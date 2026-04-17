#include "5GTSNGateway.h"
#include "../messages/FlowPacket_m.h"
#include <iostream>
#include <fstream>

Define_Module(_5GTSNGateway);

_5GTSNGateway::_5GTSNGateway() : edrlAgent(nullptr), oafsCore(nullptr),
                                  gatewayId(0), numTimeSlots(1024), numQueues(8), numSwitches(2),
                                  timeSlotSize(0.000015625), hyperPeriod(0.016), linkRate(100e6),
                                  trainingTimer(nullptr), gclUpdateTimer(nullptr),
                                  trainingInterval(0.1), gclUpdateInterval(0.016),
                                  enableEDRL(true), enableOAFS(true), isTraining(false),
                                  enableMFM(true), enableMTM(true),
                                  totalProcessedFlows(0), totalSatisfiedFlows(0), totalDelay(0.0) {}

_5GTSNGateway::~_5GTSNGateway() {
    delete edrlAgent;
    delete oafsCore;
    cancelAndDelete(trainingTimer);
    cancelAndDelete(gclUpdateTimer);
}

void _5GTSNGateway::initialize() {
    gatewayId = par("gatewayId");
    numTimeSlots = par("numTimeSlots");
    numQueues = par("numQueues");
    numSwitches = par("numSwitches");
    timeSlotSize = par("timeSlotSize");
    hyperPeriod = par("hyperPeriod");
    linkRate = par("linkRate");
    enableEDRL = par("enableEDRL");
    enableOAFS = par("enableOAFS");
    enableMFM = par("enableMFM");
    enableMTM = par("enableMTM");
    trainingInterval = par("trainingInterval");
    
    if (enableEDRL) {
        edrlAgent = new TorchEDRLAgent();
        edrlAgent->initialize(numTimeSlots, numQueues, numSwitches, 
                              timeSlotSize, hyperPeriod);
    }
    
    if (enableOAFS) {
        oafsCore = new OAFSCore();
        oafsCore->initialize(numSwitches, 4, numQueues, numTimeSlots, 
                             timeSlotSize, hyperPeriod, linkRate);
    }
    
    trainingTimer = new cMessage("TrainingTimer");
    gclUpdateTimer = new cMessage("GCLUpdateTimer");
    
    if (enableEDRL) {
        isTraining = true;
        scheduleAt(simTime() + trainingInterval, trainingTimer);
    }
    scheduleAt(simTime() + gclUpdateInterval, gclUpdateTimer);
    
    flowProcessedSignal = registerSignal("flowProcessed");
    delaySignal = registerSignal("delay");
    satisfactionSignal = registerSignal("satisfaction");
    gclGeneratedSignal = registerSignal("gclGenerated");
    
    trafficLog.open("traffic_flows.csv", std::ios::out | std::ios::trunc);
    if (trafficLog.is_open()) {
        trafficLog << "timestamp,flow_id,packet_size,priority,pcp,assigned_queue,assigned_slot,in_port,out_port,arrival_time,departure_time,delay,deadline,satisfied,flow_type,decision_source,record_time\n";
        EV << "Successfully opened traffic_flows.csv" << std::endl;
    } else {
        EV << "ERROR: Failed to open traffic_flows.csv" << std::endl;
    }
    
    gclSnapshotLog.open("gcl_snapshot.csv", std::ios::out | std::ios::trunc);
    if (gclSnapshotLog.is_open()) {
        gclSnapshotLog << "# GCL Snapshot Log\n";
        EV << "Successfully opened gcl_snapshot.csv" << std::endl;
    } else {
        EV << "ERROR: Failed to open gcl_snapshot.csv" << std::endl;
    }
    
    trainingLogLog.open("training_log.csv", std::ios::out | std::ios::trunc);
    if (trainingLogLog.is_open()) {
        trainingLogLog << "step,reward,loss,avg_delay,satisfaction,epsilon,timestamp\n";
        EV << "Successfully opened training_log.csv" << std::endl;
    } else {
        EV << "ERROR: Failed to open training_log.csv" << std::endl;
    }
    
    gclChangesLog.open("gcl_changes.csv", std::ios::out | std::ios::trunc);
    if (gclChangesLog.is_open()) {
        gclChangesLog << "timestamp,slot,port,queue,oldState,newState,source,record_time\n";
        EV << "Successfully opened gcl_changes.csv" << std::endl;
    } else {
        EV << "ERROR: Failed to open gcl_changes.csv" << std::endl;
    }
}

void _5GTSNGateway::handleMessage(cMessage* msg) {
    if (msg == trainingTimer) {
        trainEDRL();
        scheduleAt(simTime() + trainingInterval, trainingTimer);
        return;
    }
    
    if (msg == gclUpdateTimer) {
        updateGCL();
        scheduleAt(simTime() + gclUpdateInterval, gclUpdateTimer);
        return;
    }
    
    EV << "=== _5GTSNGateway::handleMessage ===" << std::endl;
    EV << "  Message arrived on gate: " << msg->getArrivalGate()->getName() << std::endl;
    EV << "  Message name: " << msg->getName() << std::endl;
    
    std::string gateName = msg->getArrivalGate()->getName();
    
    if (gateName.find("fiveGIn") != std::string::npos) {
        EV << "  Processing as 5G flow (from fiveGIn)" << std::endl;
        process5GFlow(msg);
    } else if (gateName.find("tsnIn") != std::string::npos) {
        EV << "  Processing as TSN flow (from tsnIn)" << std::endl;
        processTSNFlow(msg);
    } else {
        EV << "  Forwarding to tsnOut (default)" << std::endl;
        send(msg, "tsnOut");
    }
}

void _5GTSNGateway::finish() {
    printStatistics();
    if (edrlAgent) {
        edrlAgent->saveModel("edrl_model.txt");
    }
    
    if (trafficLog.is_open()) trafficLog.close();
    if (gclSnapshotLog.is_open()) gclSnapshotLog.close();
    if (trainingLogLog.is_open()) trainingLogLog.close();
    if (gclChangesLog.is_open()) gclChangesLog.close();
}

void _5GTSNGateway::process5GFlow(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    if (!pkt) {
        delete msg;
        return;
    }
    
    double arrivalTime = simTime().dbl();
    pkt->setArrivalTime(arrivalTime);
    
    OnlineFlow flow;
    flow.id = pkt->getFlowId();
    flow.src = pkt->getSrcNode();
    flow.dst = pkt->getDstNode();
    flow.size = pkt->getSize();
    flow.pcp = pkt->getPcp();
    flow.pdb = pkt->getPdb();
    flow.arrivalTime = arrivalTime;
    flow.deadline = arrivalTime + flow.pdb;
    
    OnlineSchedulingDecision decision;
    std::vector<PathSlotDecision> slotPath;
    
    if (enableOAFS && oafsCore) {
        decision = oafsCore->scheduleFlow(flow);
    }
    
    slotPath = findOptimalSlotPath(pkt);
    
    if (enableMFM || enableMTM) {
        compensateJitter(pkt, slotPath);
    }
    
    double pathDelay = calculatePathDelay(slotPath);
    bool satisfied = checkSatisfiability(slotPath, flow.deadline);
    
    if (satisfied && enableMTM) {
        std::vector<GCLTimeSlot> slots;
        mergeMultipleTimeSlots(slots);
    }
    
    TrafficFlowRecord record;
    record.timestamp = simTime().dbl();
    record.flowId = flow.id;
    record.srcNode = flow.src;
    record.dstNode = flow.dst;
    record.packetSize = flow.size;
    record.priority = flow.pcp;
    record.pcp = flow.pcp;
    record.tstart = pkt->getTstart();
    record.pdb = flow.pdb;
    record.assignedQueue = decision.queueSequence.empty() ? 0 : decision.queueSequence[0];
    record.assignedSlot = decision.slotSequence.empty() ? -1 : decision.slotSequence[0];
    record.inPort = 0;
    record.outPort = 0;
    record.arrivalTime = flow.arrivalTime;
    record.departureTime = simTime().dbl() + pathDelay;
    record.delay = pathDelay;
    record.deadline = flow.deadline;
    record.satisfied = satisfied;
    record.flowType = "Ethernet";
    record.decisionSource = "OAFS+GCL";
    record.slotPath = slotPath;
    
    recordTrafficFlow(record);
    
    pkt->setHopCount(pkt->getHopCount() + 1);
    pkt->setIsScheduled(true);
    pkt->setAssignedQueue(record.assignedQueue);
    pkt->setAssignedSlot(record.assignedSlot);
    
    send(msg, "tsnOut$o");
    
    totalProcessedFlows++;
    totalDelay += pathDelay;
    if (satisfied) totalSatisfiedFlows++;
    
    emit(flowProcessedSignal, totalProcessedFlows);
    emit(delaySignal, pathDelay);
    emit(satisfactionSignal, satisfied ? 1.0 : 0.0);
}

void _5GTSNGateway::processTSNFlow(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    
    double delay = pkt ? (simTime().dbl() - pkt->getArrivalTime()) : 0;
    bool satisfied = delay < (pkt ? pkt->getPdb() : 0.004);
    
    if (enableEDRL && edrlAgent && pkt) {
        edrlAgent->receiveFeedback(pkt->getFlowId(), delay, satisfied);
    }
    
    if (enableOAFS && oafsCore && pkt) {
        oafsCore->receiveFeedback(pkt->getFlowId(), delay, satisfied);
    }
    
    totalDelay += delay;
    if (satisfied) totalSatisfiedFlows++;
    
    emit(delaySignal, delay);
    emit(satisfactionSignal, satisfied ? 1.0 : 0.0);
    
    send(msg, "fiveGIn$o");
}

void _5GTSNGateway::trainEDRL() {
    if (!enableEDRL || !edrlAgent) return;
    
    edrlAgent->train();
    
    EV << "EDRL Training - Epsilon: " << edrlAgent->getEpsilon() 
       << ", Avg Reward: " << edrlAgent->getAverageReward() << std::endl;
}

void _5GTSNGateway::updateGCL() {
    if (enableEDRL && edrlAgent) {
        edrlAgent->train();
        EV << "EDRL Training completed at gateway" << std::endl;
    }
    
    if (enableOAFS && oafsCore) {
        oafsCore->adjustTimeSlots();
        oafsCore->rebalanceFlows();
    }
    
    distributeGCLToSwitches();
    
    emit(gclGeneratedSignal, 1);
}

void _5GTSNGateway::distributeGCLToSwitches() {
    EV << "Gateway GCL update completed - TSN switches have their own EDRL/OAFS instances" << std::endl;
    EV << "  Number of switches: " << numSwitches << std::endl;
    EV << "  Each switch operates independently with EDRL/OAFS" << std::endl;
}

void _5GTSNGateway::printStatistics() {
    EV << "=== 5G-TSN Gateway Statistics ===" << std::endl;
    EV << "Total Processed Flows: " << totalProcessedFlows << std::endl;
    EV << "Total Satisfied Flows: " << totalSatisfiedFlows << std::endl;
    EV << "Average Delay: " << (totalProcessedFlows > 0 ? totalDelay / totalProcessedFlows * 1000 : 0) << " ms" << std::endl;
    EV << "Satisfaction Ratio: " << (totalProcessedFlows > 0 ? (double)totalSatisfiedFlows / totalProcessedFlows * 100 : 100) << "%" << std::endl;
    
    if (enableEDRL && edrlAgent) {
        EV << "--- EDRL Statistics ---" << std::endl;
        edrlAgent->printStatistics();
    }
    
    if (enableOAFS && oafsCore) {
        EV << "--- OAFS Statistics ---" << std::endl;
        oafsCore->printStatistics();
    }
    
    EV << "=================================" << std::endl;
}

double _5GTSNGateway::getAverageDelay() const {
    return totalProcessedFlows > 0 ? totalDelay / totalProcessedFlows : 0;
}

double _5GTSNGateway::getSatisfactionRatio() const {
    return totalProcessedFlows > 0 ? (double)totalSatisfiedFlows / totalProcessedFlows : 1.0;
}

std::vector<PathSlotDecision> _5GTSNGateway::findOptimalSlotPath(FlowPacket* pkt) {
    std::vector<PathSlotDecision> path;
    
    if (!pkt) return path;
    
    int pcp = pkt->getPcp();
    int packetSize = pkt->getSize();
    double arrivalTime = pkt->getArrivalTime();
    
    for (int switchId = 0; switchId < numSwitches; switchId++) {
        PathSlotDecision decision;
        decision.switchId = switchId;
        decision.portId = 0;
        decision.queueId = pcp;
        decision.slotId = static_cast<int>(arrivalTime / timeSlotSize) + switchId;
        decision.transmissionTime = static_cast<double>(packetSize * 8) / linkRate;
        decision.waitingTime = 0.0;
        
        path.push_back(decision);
    }
    
    return path;
}

double _5GTSNGateway::calculatePathDelay(const std::vector<PathSlotDecision>& path) {
    double totalDelay = 0.0;
    for (const auto& d : path) {
        totalDelay += d.waitingTime + d.transmissionTime;
    }
    return totalDelay;
}

bool _5GTSNGateway::checkSatisfiability(const std::vector<PathSlotDecision>& path, double deadline) {
    double totalDelay = calculatePathDelay(path);
    return simTime().dbl() + totalDelay <= deadline;
}

void _5GTSNGateway::mergeMultipleFlows(std::vector<FlowPacket*>& flows) {
    if (!enableMFM || flows.empty()) return;
    
    int totalSize = 0;
    for (auto pkt : flows) {
        totalSize += pkt->getSize();
    }
}

void _5GTSNGateway::mergeMultipleTimeSlots(std::vector<GCLTimeSlot>& slots) {
    if (!enableMTM || slots.size() < 2) return;
    
    for (size_t i = 0; i < slots.size() - 1; i++) {
        bool canMerge = true;
        for (int q = 0; q < numQueues; q++) {
            if (slots[i].gateOpen[q] != slots[i+1].gateOpen[q]) {
                canMerge = false;
                break;
            }
        }
        if (canMerge) {
            slots[i].endTime = slots[i+1].endTime;
            for (int q = 0; q < numQueues; q++) {
                slots[i].occupiedBytes[q] += slots[i+1].occupiedBytes[q];
            }
            slots.erase(slots.begin() + i + 1);
            i--;
        }
    }
}

void _5GTSNGateway::compensateJitter(FlowPacket* pkt, std::vector<PathSlotDecision>& path) {
    if (!pkt) return;
    
    double jitter = pkt->getJitter();
    int flowId = pkt->getFlowId();
    
    auto it = flowJitterHistory.find(flowId);
    if (it != flowJitterHistory.end()) {
        double avgJitter = (it->second + jitter) / 2.0;
        flowJitterHistory[flowId] = avgJitter;
    } else {
        flowJitterHistory[flowId] = jitter;
    }
    
    for (auto& d : path) {
        d.waitingTime += jitter * 0.5;
    }
    
    emit(jitterCompensationSignal, jitter);
}

void _5GTSNGateway::recordTrafficFlow(const TrafficFlowRecord& record) {
    if (trafficLog.is_open()) {
        std::time_t now = std::time(nullptr);
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        trafficLog << std::fixed << std::setprecision(6)
                     << record.timestamp << ","
                     << record.flowId << ","
                     << record.packetSize << ","
                     << record.priority << ","
                     << record.pcp << ","
                     << record.assignedQueue << ","
                     << record.assignedSlot << ","
                     << record.inPort << ","
                     << record.outPort << ","
                     << record.arrivalTime << ","
                     << record.departureTime << ","
                     << record.delay << ","
                     << record.deadline << ","
                     << (record.satisfied ? 1 : 0) << ","
                     << record.flowType << ","
                     << record.decisionSource << ","
                     << timeBuffer << "\n";
        trafficLog.flush();
    }
}

void _5GTSNGateway::recordGCLSnapshot() {
    if (gclSnapshotLog.is_open()) {
        gclSnapshotLog << "# GCL Snapshot at t=" << simTime() << "s\n";
        gclSnapshotLog << "# Training steps: 0\n";
        gclSnapshotLog << "# Avg Delay: " << getAverageDelay() * 1000 << " ms\n";
        gclSnapshotLog << "# Satisfaction: " << getSatisfactionRatio() * 100 << "%\n";
        gclSnapshotLog << "# Format: time_slot,port,queue,gate_state\n";
        gclSnapshotLog.flush();
    }
}

void _5GTSNGateway::recordTrainingLog(int step, double reward, double loss, double avgDelay, double satisfaction, double epsilon) {
    if (trainingLogLog.is_open()) {
        std::time_t now = std::time(nullptr);
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        trainingLogLog << step << ","
                       << reward << ","
                       << loss << ","
                       << avgDelay << ","
                       << satisfaction << ","
                       << epsilon << ","
                       << timeBuffer << "\n";
        trainingLogLog.flush();
    }
}

void _5GTSNGateway::recordGCLChange(int slot, int port, int queue, int oldState, int newState, const std::string& source) {
    if (gclChangesLog.is_open()) {
        std::time_t now = std::time(nullptr);
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        gclChangesLog << simTime().dbl() << ","
                      << slot << ","
                      << port << ","
                      << queue << ","
                      << oldState << ","
                      << newState << ","
                      << source << ","
                      << timeBuffer << "\n";
        gclChangesLog.flush();
    }
}
