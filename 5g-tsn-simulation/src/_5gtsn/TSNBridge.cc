#include "TSNBridge.h"
#include "../messages/FlowPacket_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/common/Ethernet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/ieee8021q/Ieee8021qTagHeader_m.h"
#include <iostream>
#include <cmath>

Define_Module(TSNBridge);

TSNBridge::TSNBridge() : edrlAgent(nullptr), oafsCore(nullptr), greedyScheduler(nullptr),
                         bridgeId(0), numPorts(4), numQueues(8), numTimeSlots(1024),
                         timeSlotSize(0.000015625), hyperPeriod(0.016), linkRate(100e6),
                         enableEDRL(true), enableOAFS(true), enableTimeAwareShaper(true), enableGreedyScheduler(true),
                         gclUpdateTimer(nullptr), trainingTimer(nullptr), statsTimer(nullptr),
                         gclUpdateInterval(0.001), trainingInterval(0.01), statsInterval(0.1),
                         nextFlowId(1), currentSlot(0), slotStartTime(0),
                         totalPacketsProcessed(0), totalPacketsForwarded(0), totalPacketsDropped(0),
                         totalDelay(0), satisfiedPackets(0),
                         trainingCounter(0), checkpointInterval(100), modelSavePath("") {}

TSNBridge::~TSNBridge() {
    delete edrlAgent;
    delete oafsCore;
    delete greedyScheduler;
    cancelAndDelete(gclUpdateTimer);
    cancelAndDelete(trainingTimer);
    cancelAndDelete(statsTimer);
    for (auto& queue : queues) {
        while (!queue.packetQueue.empty()) {
            delete queue.packetQueue.front();
            queue.packetQueue.pop_front();
        }
    }
    for (int i = 0; i < (int)outputQueues.size(); i++) {
        while (!outputQueues[i].empty()) {
            delete outputQueues[i].front();
            outputQueues[i].pop_front();
        }
    }
    for (auto& timer : outputTimers) {
        cancelAndDelete(timer);
    }
}

void TSNBridge::initialize() {
    bridgeId = par("bridgeId");
    numPorts = par("numPorts");
    numQueues = par("numQueues");
    numTimeSlots = par("numTimeSlots");
    timeSlotSize = par("timeSlotSize");
    hyperPeriod = par("hyperPeriod");
    linkRate = par("linkRate");
    enableEDRL = par("enableEDRL");
    enableOAFS = par("enableOAFS");
    enableTimeAwareShaper = par("enableTimeAwareShaper");
    enableGreedyScheduler = par("enableGreedyScheduler");
    gclUpdateInterval = par("gclUpdateInterval");
    trainingInterval = par("trainingInterval");
    
    queues.resize(numQueues);
    for (int i = 0; i < numQueues; i++) {
        queues[i].queueId = i;
        queues[i].backlogBytes = 0;
        queues[i].packetCount = 0;
        queues[i].lastDequeueTime = 0;
        queues[i].avgDelay = 0;
    }
    
    outputQueues.resize(numPorts);
    outputTimers.resize(numPorts, nullptr);
    for (int i = 0; i < numPorts; i++) {
        outputTimers[i] = new cMessage(("OutputTimer_" + std::to_string(i)).c_str());
    }
    
    timeSlots.resize(numTimeSlots);
    for (int i = 0; i < numTimeSlots; i++) {
        timeSlots[i].slotId = i;
        timeSlots[i].isActive = true;
        timeSlots[i].assignedQueue = i % numQueues;
        timeSlots[i].utilization = 0;
        timeSlots[i].packetCount = 0;
    }
    
    if (enableEDRL) {
        edrlAgent = new TorchEDRLAgent();
        edrlAgent->initialize(numTimeSlots, numQueues, 1, timeSlotSize, hyperPeriod);
    }
    
    if (enableOAFS) {
        oafsCore = new OAFSCore();
        oafsCore->initialize(1, numPorts, numQueues, numTimeSlots, timeSlotSize, hyperPeriod, linkRate);
    }
    
    if (enableGreedyScheduler) {
        greedyScheduler = new HeuristicGreedyScheduler();
        greedyScheduler->initialize(hyperPeriod * 1e6, numQueues); // 转换为微秒
        EV << "========================================" << endl;
        EV << "  启发式贪心调度算法已初始化!   " << endl;
        EV << "  Heuristic Greedy Scheduler Ready!   " << endl;
        EV << "========================================" << endl;
        // 运行3流冲突场景演示
        greedyScheduler->demo3FlowScheduling();
    }
    
    gclUpdateTimer = new cMessage("GCLUpdateTimer");
    trainingTimer = new cMessage("TrainingTimer");
    statsTimer = new cMessage("StatsTimer");
    
    scheduleAt(simTime() + gclUpdateInterval, gclUpdateTimer);
    if (enableEDRL) scheduleAt(simTime() + trainingInterval, trainingTimer);
    scheduleAt(simTime() + statsInterval, statsTimer);
    
    packetProcessedSignal = registerSignal("packetProcessed");
    packetForwardedSignal = registerSignal("packetForwarded");
    packetDroppedSignal = registerSignal("packetDropped");
    delaySignal = registerSignal("delay");
    queueLengthSignal = registerSignal("queueLength");
    satisfactionSignal = registerSignal("satisfaction");
    throughputSignal = registerSignal("throughput");
    slotUtilizationSignal = registerSignal("slotUtilization");
}

void TSNBridge::handleMessage(cMessage* msg) {
    if (msg == gclUpdateTimer) {
        handleGCLUpdate();
        scheduleAt(simTime() + gclUpdateInterval, gclUpdateTimer);
        return;
    }
    if (msg == trainingTimer) {
        handleTraining();
        scheduleAt(simTime() + trainingInterval, trainingTimer);
        return;
    }
    if (msg == statsTimer) {
        handleStatsUpdate();
        scheduleAt(simTime() + statsInterval, statsTimer);
        return;
    }
    for (int i = 0; i < numPorts; i++) {
        if (msg == outputTimers[i]) {
            sendFromOutputQueue(i);
            return;
        }
    }
    processPacket(msg);
}

void TSNBridge::processPacket(cMessage* msg) {
    totalPacketsProcessed++;
    emit(packetProcessedSignal, 1L);
    
    FlowPacket* flowPkt = dynamic_cast<FlowPacket*>(msg);
    inet::Packet* ethernetPkt = flowPkt == nullptr ? dynamic_cast<inet::Packet*>(msg) : nullptr;
    
    FlowInfo flowInfo;
    flowInfo.flowId = flowPkt ? flowPkt->getFlowId() : nextFlowId++;
    flowInfo.arrivalTime = simTime().dbl();
    flowInfo.pcp = flowPkt ? flowPkt->getPcp() : 0;
    flowInfo.size = flowPkt ? flowPkt->getSize() : 100;
    flowInfo.pdb = flowPkt ? flowPkt->getPdb() : 0.01;
    flowInfo.deadline = flowInfo.arrivalTime + flowInfo.pdb;
    flowInfo.srcAddr = flowPkt ? flowPkt->getSrcNode() : 0;
    flowInfo.dstAddr = flowPkt ? flowPkt->getDstNode() : 0;

    if (ethernetPkt != nullptr) {
        flowInfo.size = ethernetPkt->getByteLength();
        const auto& ethHeader = ethernetPkt->peekAtFront<inet::EthernetMacHeader>();
        if (ethHeader != nullptr && ethHeader->getTypeOrLength() == inet::ETHERTYPE_8021Q_TAG) {
            const auto& vlanTag = ethernetPkt->peekDataAt<inet::Ieee8021qTagEpdHeader>(inet::B(inet::ETHER_MAC_HEADER_BYTES));
            if (vlanTag != nullptr)
                flowInfo.pcp = vlanTag->getPcp();
        }
    }
    
    int queueId = selectQueue(flowInfo.flowId, flowInfo.pcp, flowInfo.deadline);
    int slotId = selectTimeSlot(flowInfo.flowId, queueId, flowInfo.deadline);
    
    flowInfo.assignedQueue = queueId;
    flowInfo.assignedSlot = slotId;
    activeFlows[flowInfo.flowId] = flowInfo;
    
    if (enableEDRL && edrlAgent) {
        NetworkState state;
        state.currentTimeSlot = currentSlot;
        for (int i = 0; i < numQueues; i++) {
            state.queueLengths.push_back(queues[i].packetCount);
        }
        // 填充时隙占用矩阵 (1024时隙 x 8队列)
        for (int s = 0; s < numTimeSlots; s++) {
            std::vector<int> slotRow;
            for (int q = 0; q < numQueues; q++) {
                // 使用时隙的packetCount作为占用指标
                slotRow.push_back(timeSlots[s].packetCount);
            }
            state.slotOccupancy.push_back(slotRow);
        }
        state.avgDelay = getAverageDelay();
        state.timestamp = simTime().dbl();
        SchedulingDecision decision = edrlAgent->makeDecision(state, flowPkt);
        if (decision.assignedQueue >= 0 && decision.assignedQueue < numQueues) {
            queueId = decision.assignedQueue;
        }
        if (decision.assignedSlot >= 0 && decision.assignedSlot < numTimeSlots) {
            slotId = decision.assignedSlot;
            timeSlots[slotId].assignedQueue = queueId;
        }
    }
    
    if (enableOAFS && oafsCore) {
        OnlineFlow oflow;
        oflow.id = flowInfo.flowId;
        oflow.src = flowInfo.srcAddr;
        oflow.dst = flowInfo.dstAddr;
        oflow.size = flowInfo.size;
        oflow.pcp = flowInfo.pcp;
        oflow.pdb = flowInfo.pdb;
        oflow.arrivalTime = flowInfo.arrivalTime;
        oflow.deadline = flowInfo.deadline;
        OnlineSchedulingDecision odecision = oafsCore->scheduleFlow(oflow);
        if (!odecision.queueSequence.empty()) queueId = odecision.queueSequence[0];
        if (!odecision.slotSequence.empty()) slotId = odecision.slotSequence[0];
    }
    
    enqueuePacket(msg, queueId, slotId);
    updateQueueState(queueId);
    updateTimeSlotState(slotId);
    emit(queueLengthSignal, queues[queueId].packetCount);
}

void TSNBridge::handleGCLUpdate() {
    currentSlot = (currentSlot + 1) % numTimeSlots;
    slotStartTime = simTime().dbl();
    int packetsForwardedThisSlot = 0;
    int maxPacketsPerSlot = 10;
    double currentSimTime = simTime().dbl();
    
    if (enableGreedyScheduler && greedyScheduler) {
        // ==================== 使用启发式贪心调度器控制门控 ====================
        
        // 获取当前哪些队列应该开门
        std::vector<bool> gateStates = greedyScheduler->getGateStatesAtTime(currentSimTime);
        
        // 从高优先级队列到低优先级队列检查
        for (int q = numQueues - 1; q >= 0 && packetsForwardedThisSlot < maxPacketsPerSlot; q--) {
            // 只有当对应队列的门打开，且队列有数据包时，才转发
            if (gateStates[q] && !queues[q].packetQueue.empty()) {
                cMessage* pkt = queues[q].packetQueue.front();
                queues[q].packetQueue.pop_front();
                int arrivalGateIndex = -1;
                if (pkt->getArrivalGate()) {
                    std::string gateName = pkt->getArrivalGate()->getBaseName();
                    if (gateName == "port") {
                        arrivalGateIndex = pkt->getArrivalGate()->getIndex();
                    }
                }
                int outputPort = -1;
                for (int p = 0; p < numPorts; p++) {
                    if (p != arrivalGateIndex && hasGate("port$o", p) && gate("port$o", p)->isConnected()) {
                        outputPort = p;
                        break;
                    }
                }
                if (outputPort >= 0) {
                    forwardPacket(pkt, outputPort);
                    packetsForwardedThisSlot++;
                } else {
                    delete pkt;
                }
                dequeuePacket(q);
            }
        }
    } else if (enableTimeAwareShaper) {
        // ==================== 使用传统时间感知整形器 ====================
        for (int q = numQueues - 1; q >= 0 && packetsForwardedThisSlot < maxPacketsPerSlot; q--) {
            if (!queues[q].packetQueue.empty()) {
                cMessage* pkt = queues[q].packetQueue.front();
                queues[q].packetQueue.pop_front();
                int arrivalGateIndex = -1;
                if (pkt->getArrivalGate()) {
                    std::string gateName = pkt->getArrivalGate()->getBaseName();
                    if (gateName == "port") {
                        arrivalGateIndex = pkt->getArrivalGate()->getIndex();
                    }
                }
                int outputPort = -1;
                for (int p = 0; p < numPorts; p++) {
                    if (p != arrivalGateIndex && hasGate("port$o", p) && gate("port$o", p)->isConnected()) {
                        outputPort = p;
                        break;
                    }
                }
                if (outputPort >= 0) {
                    forwardPacket(pkt, outputPort);
                    packetsForwardedThisSlot++;
                } else {
                    delete pkt;
                }
                dequeuePacket(q);
            }
        }
    }
    
    if (enableOAFS && oafsCore) oafsCore->adjustTimeSlots();
}

void TSNBridge::handleTraining() {
    if (!enableEDRL || !edrlAgent) return;
    
    edrlAgent->train();
    
    trainingCounter++;
    
    if (trainingCounter % checkpointInterval == 0) {
        std::string checkpointPath = modelSavePath + "edrl_checkpoint";
        edrlAgent->saveModel(checkpointPath);
        EV << "=== Checkpoint saved at " << simTime() << "s ===" << std::endl;
        EV << "  Path: " << checkpointPath << std::endl;
        EV << "  Training count: " << trainingCounter << std::endl;
        EV << "  Avg Delay: " << getAverageDelay() * 1000 << " ms" << std::endl;
        EV << "  Satisfaction: " << getSatisfactionRatio() * 100 << "%" << std::endl;
    }
}

void TSNBridge::handleStatsUpdate() { recordStatistics(); }

int TSNBridge::selectQueue(int flowId, int pcp, double deadline) {
    double urgency = deadline - simTime().dbl();
    if (urgency < 0.001) return 7;
    else if (urgency < 0.005) return 6;
    else if (urgency < 0.01) return 5;
    else return pcp % numQueues;
}

int TSNBridge::selectTimeSlot(int flowId, int queueId, double deadline) {
    double timeToDeadline = deadline - simTime().dbl();
    int slotsToDeadline = (int)std::ceil(timeToDeadline / timeSlotSize);
    slotsToDeadline = std::min(slotsToDeadline, numTimeSlots / 2);
    int targetSlot = (currentSlot + slotsToDeadline) % numTimeSlots;
    while (!timeSlots[targetSlot].isActive && slotsToDeadline < numTimeSlots) {
        targetSlot = (targetSlot + 1) % numTimeSlots;
        slotsToDeadline++;
    }
    return targetSlot;
}

void TSNBridge::enqueuePacket(cMessage* msg, int queueId, int slotId) {
    if (queueId < 0 || queueId >= numQueues) queueId = 0;
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    int pktSize = pkt ? pkt->getSize() : 100;
    queues[queueId].packetQueue.push_back(msg);
    queues[queueId].backlogBytes += pktSize;
    queues[queueId].packetCount++;
    timeSlots[slotId].packetCount++;
    timeSlots[slotId].utilization = (double)timeSlots[slotId].packetCount / 100.0;
}

void TSNBridge::dequeuePacket(int queueId) {
    if (queueId < 0 || queueId >= numQueues) return;
    if (!queues[queueId].packetQueue.empty()) {
        FlowPacket* pkt = dynamic_cast<FlowPacket*>(queues[queueId].packetQueue.front());
        int pktSize = pkt ? pkt->getSize() : 100;
        queues[queueId].backlogBytes -= pktSize;
        queues[queueId].packetCount--;
        queues[queueId].lastDequeueTime = simTime().dbl();
    }
}

void TSNBridge::forwardPacket(cMessage* msg, int outputPort) {
    totalPacketsForwarded++;
    emit(packetForwardedSignal, 1L);
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    if (pkt) {
        double delay = calculateDelay(pkt->getFlowId());
        totalDelay += delay;
        bool satisfied = checkDeadlineSatisfaction(pkt->getFlowId());
        if (satisfied) satisfiedPackets++;
        emit(delaySignal, delay);
        emit(satisfactionSignal, satisfied ? 1.0 : 0.0);
        if (enableEDRL && edrlAgent) edrlAgent->receiveFeedback(pkt->getFlowId(), delay, satisfied);
        if (enableOAFS && oafsCore) oafsCore->receiveFeedback(pkt->getFlowId(), delay, satisfied);
        activeFlows.erase(pkt->getFlowId());
    }
    if (outputPort < 0 || outputPort >= numPorts) {
        outputPort = 0;
    }
    outputQueues[outputPort].push_back(msg);
    sendFromOutputQueue(outputPort);
}

void TSNBridge::sendFromOutputQueue(int port) {
    if (outputQueues[port].empty()) return;
    cChannel *txChannel = gate("port$o", port)->getTransmissionChannel();
    if (txChannel->isBusy()) {
        simtime_t finishTime = txChannel->getTransmissionFinishTime();
        if (!outputTimers[port]->isScheduled()) {
            scheduleAt(finishTime, outputTimers[port]);
        }
        return;
    }
    cMessage* msg = outputQueues[port].front();
    outputQueues[port].pop_front();
    send(msg, "port$o", port);
    if (!outputQueues[port].empty()) {
        cChannel *newTxChannel = gate("port$o", port)->getTransmissionChannel();
        if (newTxChannel->isBusy()) {
            simtime_t finishTime = newTxChannel->getTransmissionFinishTime();
            if (!outputTimers[port]->isScheduled()) {
                scheduleAt(finishTime, outputTimers[port]);
            }
        } else {
            if (!outputTimers[port]->isScheduled()) {
                scheduleAt(simTime(), outputTimers[port]);
            }
        }
    }
}

void TSNBridge::updateQueueState(int queueId) {
    if (queueId < 0 || queueId >= numQueues) return;
    double totalQueueDelay = 0;
    int count = 0;
    for (auto& pair : activeFlows) {
        if (pair.second.assignedQueue == queueId) {
            totalQueueDelay += simTime().dbl() - pair.second.arrivalTime;
            count++;
        }
    }
    if (count > 0) queues[queueId].avgDelay = totalQueueDelay / count;
}

void TSNBridge::updateTimeSlotState(int slotId) {
    if (slotId < 0 || slotId >= numTimeSlots) return;
    double totalBytes = 0;
    for (auto& pair : activeFlows) {
        if (pair.second.assignedSlot == slotId) totalBytes += pair.second.size;
    }
    timeSlots[slotId].utilization = totalBytes / (linkRate * timeSlotSize);
}

double TSNBridge::calculateDelay(int flowId) {
    auto it = activeFlows.find(flowId);
    if (it != activeFlows.end()) return simTime().dbl() - it->second.arrivalTime;
    return 0;
}

bool TSNBridge::checkDeadlineSatisfaction(int flowId) {
    auto it = activeFlows.find(flowId);
    if (it != activeFlows.end()) {
        double delay = simTime().dbl() - it->second.arrivalTime;
        return delay <= it->second.pdb;
    }
    return true;
}

void TSNBridge::recordStatistics() {
    double throughput = totalPacketsForwarded / (simTime().dbl() + 0.001);
    emit(throughputSignal, throughput);
}

void TSNBridge::finish() {
    recordStatistics();
    EV << "=== TSNBridge " << bridgeId << " Statistics ===" << std::endl;
    EV << "Processed: " << totalPacketsProcessed << " Forwarded: " << totalPacketsForwarded << std::endl;
    EV << "Avg Delay: " << getAverageDelay() * 1000 << " ms" << std::endl;
    EV << "Satisfaction: " << getSatisfactionRatio() * 100 << "%" << std::endl;
    EV << "Total Training Steps: " << trainingCounter << std::endl;
    
    if (enableEDRL && edrlAgent) {
        std::string finalModelPath = modelSavePath + "final_edrl_model_" + 
                                     std::to_string((int)simTime().dbl()) + "s.txt";
        edrlAgent->saveModel(finalModelPath);
        EV << "=== Final model saved ===" << std::endl;
        EV << "  Path: " << finalModelPath << std::endl;
        
        edrlAgent->saveTrainingLog(modelSavePath + "training_log.csv");
        EV << "  Training log: " << modelSavePath << "training_log.csv" << std::endl;
    }
}

double TSNBridge::getAverageDelay() const {
    return totalPacketsForwarded > 0 ? totalDelay / totalPacketsForwarded : 0;
}

double TSNBridge::getSatisfactionRatio() const {
    return totalPacketsForwarded > 0 ? (double)satisfiedPackets / totalPacketsForwarded : 1.0;
}

int TSNBridge::getQueueLength(int queueId) const {
    if (queueId >= 0 && queueId < numQueues) return queues[queueId].packetCount;
    return 0;
}

double TSNBridge::getSlotUtilization(int slotId) const {
    if (slotId >= 0 && slotId < numTimeSlots) return timeSlots[slotId].utilization;
    return 0;
}
