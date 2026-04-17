//
// OAFSModule.cc - Online Adaptive Flow Scheduling for TSN
// 实时在线闭环动态调整算法实现
//

#include "OAFSModule.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

// ==================== OAFSCore Implementation ====================

OAFSCore::OAFSCore()
    : numSwitches(0), numPortsPerSwitch(0), numQueues(8), numTimeSlots(1024),
      timeSlotSize(0.000015625), hyperPeriod(0.016), linkRate(100e6),
      adaptationRate(0.1), predictionWindow(0.01), adjustmentThreshold(0.8),
      flowCounter(0), totalDelay(0), satisfiedCount(0), totalCount(0) {
    
    std::random_device rd;
    rng.seed(rd());
}

OAFSCore::~OAFSCore() {}

void OAFSCore::initialize(int numSwitches, int numPortsPerSwitch, int numQueues,
                          int numTimeSlots, double timeSlotSize, double hyperPeriod, double linkRate) {
    this->numSwitches = numSwitches;
    this->numPortsPerSwitch = numPortsPerSwitch;
    this->numQueues = numQueues;
    this->numTimeSlots = numTimeSlots;
    this->timeSlotSize = timeSlotSize;
    this->hyperPeriod = hyperPeriod;
    this->linkRate = linkRate;
    
    initializeTimeSlots();
}

void OAFSCore::initializeTimeSlots() {
    timeSlots.resize(numSwitches);
    for (int s = 0; s < numSwitches; s++) {
        timeSlots[s].resize(numPortsPerSwitch);
        for (int p = 0; p < numPortsPerSwitch; p++) {
            timeSlots[s][p].resize(numTimeSlots);
            for (int t = 0; t < numTimeSlots; t++) {
                OnlineTimeSlot& slot = timeSlots[s][p][t];
                slot.id = t;
                slot.switchId = s;
                slot.portId = p;
                slot.startTime = t * timeSlotSize;
                slot.endTime = (t + 1) * timeSlotSize;
                slot.capacity = 1500;
                slot.remainingCapacity = slot.capacity;
            }
        }
    }
}

OnlineSchedulingDecision OAFSCore::scheduleFlow(OnlineFlow& flow) {
    OnlineSchedulingDecision decision;
    decision.flowId = flow.id;
    decision.decisionTime = simTime();
    
    // 在线调度算法
    // 1. 分析当前网络状态
    double currentLoad = 0;
    for (const auto& switchSlots : timeSlots) {
        for (const auto& portSlots : switchSlots) {
            for (const auto& slot : portSlots) {
                currentLoad += slot.utilization;
            }
        }
    }
    currentLoad /= (numSwitches * numPortsPerSwitch * numTimeSlots);
    
    // 2. 预测未来负载
    double predictedLoad = predictSlotLoad(0, 0, monitorData.currentTimeSlot);
    
    // 3. 选择最优时间槽序列
    int bestSlot = -1;
    int bestQueue = -1;
    double bestScore = -1;
    
    for (int s = 0; s < numSwitches; s++) {
        for (int p = 0; p < numPortsPerSwitch; p++) {
            int slot = findOptimalSlot(flow, s, p);
            if (slot >= 0) {
                double score = timeSlots[s][p][slot].remainingCapacity - 
                               timeSlots[s][p][slot].predictedLoad * adaptationRate;
                if (score > bestScore) {
                    bestScore = score;
                    bestSlot = slot;
                    bestQueue = flow.pcp % numQueues;
                    decision.switchSequence.clear();
                    decision.switchSequence.push_back(s);
                }
            }
        }
    }
    
    if (bestSlot >= 0) {
        decision.slotSequence.push_back(bestSlot);
        decision.queueSequence.push_back(bestQueue);
        
        // 更新时间槽状态
        int sw = decision.switchSequence[0];
        int port = 0;
        timeSlots[sw][port][bestSlot].flowIds.push_back(flow.id);
        timeSlots[sw][port][bestSlot].remainingCapacity -= flow.size;
        timeSlots[sw][port][bestSlot].utilization = 
            1.0 - (double)timeSlots[sw][port][bestSlot].remainingCapacity / 
            timeSlots[sw][port][bestSlot].capacity;
        
        // 更新流信息
        flow.assignedSlot = bestSlot;
        flow.assignedQueue = bestQueue;
        flow.assignedSwitch = sw;
        flow.scheduledTime = simTime().dbl();
        flow.state = FLOW_SCHEDULED;
        
        // 计算预期延迟
        decision.expectedDelay = calculateExpectedDelay(flow, decision);
        decision.confidence = 1.0 - (currentLoad + predictedLoad) / 2.0;
        
        // 检查是否需要调整
        decision.needsAdjustment = (currentLoad > adjustmentThreshold);
        decision.adjustmentReason = currentLoad;
    }
    
    // 存储活动流
    activeFlows[flow.id] = flow;
    
    return decision;
}

void OAFSCore::updateFlowStatus(int flowId, FlowState state, double currentDelay) {
    auto it = activeFlows.find(flowId);
    if (it != activeFlows.end()) {
        it->second.state = state;
        it->second.currentDelay = currentDelay;
        
        if (state == FLOW_COMPLETED || state == FLOW_TIMEOUT) {
            // 移动到完成队列
            completedFlows[flowId] = it->second;
            activeFlows.erase(it);
        }
    }
}

void OAFSCore::receiveFeedback(int flowId, double actualDelay, bool satisfied) {
    auto it = activeFlows.find(flowId);
    if (it == activeFlows.end()) {
        it = completedFlows.find(flowId);
    }
    
    if (it != activeFlows.end() || it != completedFlows.end()) {
        OnlineFlow& flow = (it != activeFlows.end()) ? it->second : completedFlows[flowId];
        flow.actualDelay = actualDelay;
        flow.isSatisfied = satisfied;
        
        // 更新统计
        totalDelay += actualDelay;
        totalCount++;
        if (satisfied) satisfiedCount++;
        
        // 更新时间槽的实际负载
        if (flow.assignedSlot >= 0 && flow.assignedSwitch >= 0) {
            int sw = flow.assignedSwitch;
            int port = 0;
            int slot = flow.assignedSlot;
            
            timeSlots[sw][port][slot].actualLoad += flow.size;
            
            // 动态调整
            double error = actualDelay - flow.pdb;
            if (error > 0) {
                // 延迟超标，需要调整
                adjustSchedule(flowId);
            }
        }
    }
}

void OAFSCore::adjustSchedule(int flowId) {
    auto it = activeFlows.find(flowId);
    if (it == activeFlows.end()) return;
    
    OnlineFlow& flow = it->second;
    
    // 寻找更好的时间槽
    int newSlot = findOptimalSlot(flow, flow.assignedSwitch, 0);
    if (newSlot >= 0 && newSlot != flow.assignedSlot) {
        // 释放旧时间槽
        int oldSlot = flow.assignedSlot;
        int sw = flow.assignedSwitch;
        int port = 0;
        
        auto& flowIds = timeSlots[sw][port][oldSlot].flowIds;
        flowIds.erase(std::remove(flowIds.begin(), flowIds.end(), flowId), flowIds.end());
        timeSlots[sw][port][oldSlot].remainingCapacity += flow.size;
        
        // 分配新时间槽
        flow.assignedSlot = newSlot;
        timeSlots[sw][port][newSlot].flowIds.push_back(flowId);
        timeSlots[sw][port][newSlot].remainingCapacity -= flow.size;
    }
}

void OAFSCore::adjustTimeSlots() {
    // 在线调整时间槽分配
    for (int s = 0; s < numSwitches; s++) {
        for (int p = 0; p < numPortsPerSwitch; p++) {
            for (int t = 0; t < numTimeSlots; t++) {
                OnlineTimeSlot& slot = timeSlots[s][p][t];
                
                // 根据实际负载调整
                if (slot.actualLoad > slot.predictedLoad * 1.2) {
                    // 过载，增加调整因子
                    slot.adjustmentFactor *= 1.1;
                } else if (slot.actualLoad < slot.predictedLoad * 0.8) {
                    // 欠载，减少调整因子
                    slot.adjustmentFactor *= 0.9;
                }
                
                // 更新预测负载
                slot.predictedLoad = slot.actualLoad * slot.adjustmentFactor;
            }
        }
    }
}

void OAFSCore::rebalanceFlows() {
    // 重新平衡流分配
    for (auto& pair : activeFlows) {
        OnlineFlow& flow = pair.second;
        
        // 检查是否需要重新调度
        if (flow.currentDelay > flow.pdb * 0.8) {
            adjustSchedule(flow.id);
        }
    }
}

void OAFSCore::handleOverload(int switchId, int portId) {
    // 处理过载情况
    std::vector<int> flowsToReschedule;
    
    for (int t = 0; t < numTimeSlots; t++) {
        OnlineTimeSlot& slot = timeSlots[switchId][portId][t];
        if (slot.utilization > adjustmentThreshold) {
            // 收集需要重新调度的流
            for (int flowId : slot.flowIds) {
                flowsToReschedule.push_back(flowId);
            }
        }
    }
    
    // 重新调度
    for (int flowId : flowsToReschedule) {
        adjustSchedule(flowId);
    }
}

double OAFSCore::predictSlotLoad(int switchId, int portId, int slotId) {
    // 基于历史数据预测负载
    double predicted = 0;
    int historySize = std::min((int)monitorHistory.size(), 10);
    
    if (historySize > 0) {
        double sum = 0;
        int count = 0;
        
        for (int i = monitorHistory.size() - historySize; i < monitorHistory.size(); i++) {
            if (slotId < monitorHistory[i].slotUtilization.size()) {
                sum += monitorHistory[i].slotUtilization[slotId];
                count++;
            }
        }
        
        if (count > 0) {
            predicted = sum / count;
        }
    }
    
    return predicted;
}

int OAFSCore::findOptimalSlot(const OnlineFlow& flow, int switchId, int portId) {
    int bestSlot = -1;
    double bestScore = -1;
    
    int arrivalSlot = (int)(flow.arrivalTime / timeSlotSize) % numTimeSlots;
    
    for (int t = 0; t < numTimeSlots; t++) {
        int slot = (arrivalSlot + t) % numTimeSlots;
        OnlineTimeSlot& ts = timeSlots[switchId][portId][slot];
        
        // 检查容量
        if (ts.remainingCapacity >= flow.size) {
            // 计算得分
            double score = ts.remainingCapacity - ts.predictedLoad * adaptationRate;
            score += (1.0 - ts.utilization) * 10; // 偏好低利用率
            
            // 考虑延迟约束
            double slotDelay = t * timeSlotSize;
            if (slotDelay <= flow.pdb) {
                score += (flow.pdb - slotDelay) * 100;
            }
            
            if (score > bestScore) {
                bestScore = score;
                bestSlot = slot;
            }
        }
    }
    
    return bestSlot;
}

bool OAFSCore::canMergeFlows(const OnlineTimeSlot& slot, const OnlineFlow& flow) {
    if (slot.remainingCapacity < flow.size) return false;
    
    for (int existingFlowId : slot.flowIds) {
        auto it = activeFlows.find(existingFlowId);
        if (it != activeFlows.end()) {
            if (std::abs(it->second.pdb - flow.pdb) > flow.pdb * 0.5) {
                return false;
            }
        }
    }
    
    return true;
}

double OAFSCore::calculateExpectedDelay(const OnlineFlow& flow, 
                                         const OnlineSchedulingDecision& decision) {
    double delay = 0;
    
    // 传输延迟
    delay += flow.size * 8.0 / linkRate;
    
    // 排队延迟
    if (!decision.slotSequence.empty()) {
        int slot = decision.slotSequence[0];
        delay += slot * timeSlotSize;
    }
    
    // 处理延迟
    delay += decision.switchSequence.size() * 0.0001;
    
    return delay;
}

void OAFSCore::updateMonitorData(const NetworkMonitorData& data) {
    monitorData = data;
    monitorHistory.push_back(data);
    
    // 保持历史记录在合理范围内
    if (monitorHistory.size() > 100) {
        monitorHistory.pop_front();
    }
    
    // 触发在线调整
    if (data.satisfactionRatio < 0.9) {
        rebalanceFlows();
    }
}

double OAFSCore::getSatisfactionRatio() const {
    if (totalCount == 0) return 1.0;
    return (double)satisfiedCount / totalCount;
}

double OAFSCore::getAverageDelay() const {
    if (totalCount == 0) return 0.0;
    return totalDelay / totalCount;
}

double OAFSCore::getThroughput() const {
    return monitorData.throughput;
}

void OAFSCore::printStatistics() {
    EV << "=== OAFS Statistics ===" << std::endl;
    EV << "Active Flows: " << activeFlows.size() << std::endl;
    EV << "Completed Flows: " << completedFlows.size() << std::endl;
    EV << "Pending Flows: " << pendingQueue.size() << std::endl;
    EV << "Average Delay: " << getAverageDelay() * 1000 << " ms" << std::endl;
    EV << "Satisfaction Ratio: " << getSatisfactionRatio() * 100 << "%" << std::endl;
    EV << "Throughput: " << getThroughput() / 1e6 << " Mbps" << std::endl;
    EV << "======================" << std::endl;
}

void OAFSCore::printTimeSlotStatus(int switchId, int portId) {
    EV << "Time Slot Status for Switch " << switchId << " Port " << portId << std::endl;
    for (int t = 0; t < numTimeSlots; t++) {
        OnlineTimeSlot& slot = timeSlots[switchId][portId][t];
        EV << "  Slot " << t << ": Util=" << slot.utilization * 100 << "%"
           << " Flows=" << slot.flowIds.size() << std::endl;
    }
}

// ==================== OAFSModule Implementation ====================

OAFSModule::OAFSModule()
    : oafsCore(nullptr), schedulingTimer(nullptr), adjustmentTimer(nullptr),
      monitorTimer(nullptr), flowCounter(0) {}

OAFSModule::~OAFSModule() {
    delete oafsCore;
    cancelAndDelete(schedulingTimer);
    cancelAndDelete(adjustmentTimer);
    cancelAndDelete(monitorTimer);
}

void OAFSModule::initialize() {
    // 获取参数
    numTimeSlots = par("numTimeSlots");
    numQueues = par("numQueues");
    numSwitches = par("numSwitches");
    timeSlotSize = par("timeSlotSize");
    hyperPeriod = par("hyperPeriod");
    linkRate = par("linkRate");
    schedulingInterval = timeSlotSize;
    adjustmentInterval = hyperPeriod / 4;
    monitorInterval = timeSlotSize;
    enableAdaptive = par("enableAdaptive");
    
    // 创建OAFS核心
    oafsCore = new OAFSCore();
    oafsCore->initialize(numSwitches, 4, numQueues, numTimeSlots, 
                         timeSlotSize, hyperPeriod, linkRate);
    
    // 注册信号
    flowScheduledSignal = registerSignal("oafsFlowScheduled");
    flowCompletedSignal = registerSignal("oafsFlowCompleted");
    satisfactionSignal = registerSignal("oafsSatisfaction");
    delaySignal = registerSignal("oafsDelay");
    throughputSignal = registerSignal("oafsThroughput");
    adjustmentSignal = registerSignal("oafsAdjustment");
    
    // 启动定时器
    schedulingTimer = new cMessage("SchedulingTimer");
    adjustmentTimer = new cMessage("AdjustmentTimer");
    monitorTimer = new cMessage("MonitorTimer");
    
    scheduleAt(simTime() + schedulingInterval, schedulingTimer);
    scheduleAt(simTime() + adjustmentInterval, adjustmentTimer);
    scheduleAt(simTime() + monitorInterval, monitorTimer);
}

void OAFSModule::handleMessage(cMessage* msg) {
    if (msg == schedulingTimer) {
        handleSchedulingTimer();
    } else if (msg == adjustmentTimer) {
        handleAdjustmentTimer();
    } else if (msg == monitorTimer) {
        handleMonitorTimer();
    } else if (FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg)) {
        handleFlowPacket(pkt);
    } else {
        handleFeedback(msg);
    }
}

void OAFSModule::handleSchedulingTimer() {
    // 处理待调度队列
    while (!pendingFlows.empty()) {
        auto it = pendingFlows.begin();
        FlowPacket* pkt = it->second;
        
        // 创建在线流
        OnlineFlow flow;
        flow.id = pkt->getFlowId();
        flow.src = pkt->getSrcNode();
        flow.dst = pkt->getDstNode();
        flow.size = pkt->getSize();
        flow.pcp = pkt->getPcp();
        flow.pdb = pkt->getPdb();
        flow.arrivalTime = simTime().dbl();
        flow.deadline = flow.arrivalTime + flow.pdb;
        
        // 在线调度
        OnlineSchedulingDecision decision = oafsCore->scheduleFlow(flow);
        sendSchedulingDecision(decision);
        
        // 发送信号
        emit(flowScheduledSignal, flow.id);
        
        pendingFlows.erase(it);
        delete pkt;
    }
    
    scheduleAt(simTime() + schedulingInterval, schedulingTimer);
}

void OAFSModule::handleAdjustmentTimer() {
    // 在线调整
    if (enableAdaptive) {
        processOnlineAdjustment();
    }
    
    scheduleAt(simTime() + adjustmentInterval, adjustmentTimer);
}

void OAFSModule::handleMonitorTimer() {
    // 收集监控数据
    collectMonitorData();
    
    // 发送统计信号
    emit(satisfactionSignal, oafsCore->getSatisfactionRatio());
    emit(delaySignal, oafsCore->getAverageDelay());
    emit(throughputSignal, oafsCore->getThroughput());
    
    // 打印统计
    if (simTime().dbl() >= 1.0) {
        oafsCore->printStatistics();
    }
    
    scheduleAt(simTime() + monitorInterval, monitorTimer);
}

void OAFSModule::handleFlowPacket(FlowPacket* pkt) {
    // 添加到待调度队列
    pendingFlows[pkt->getFlowId()] = pkt;
}

void OAFSModule::handleFeedback(cMessage* msg) {
    // 解析反馈
    int flowId = msg->par("flowId");
    double delay = msg->par("delay");
    bool satisfied = msg->par("satisfied");
    
    // 报告反馈
    oafsCore->receiveFeedback(flowId, delay, satisfied);
    
    // 发送信号
    emit(flowCompletedSignal, flowId);
    
    delete msg;
}

void OAFSModule::collectMonitorData() {
    NetworkMonitorData data;
    data.currentTime = simTime().dbl();
    data.currentTimeSlot = (int)(simTime().dbl() / timeSlotSize) % numTimeSlots;
    data.avgDelay = oafsCore->getAverageDelay();
    data.satisfactionRatio = oafsCore->getSatisfactionRatio();
    
    oafsCore->updateMonitorData(data);
}

void OAFSModule::sendSchedulingDecision(const OnlineSchedulingDecision& decision) {
    cMessage* decisionMsg = new cMessage("OAFSDecision");
    decisionMsg->addPar("flowId") = decision.flowId;
    decisionMsg->addPar("expectedDelay") = decision.expectedDelay;
    decisionMsg->addPar("confidence") = decision.confidence;
    
    if (!decision.slotSequence.empty()) {
        decisionMsg->addPar("slot") = decision.slotSequence[0];
    }
    if (!decision.queueSequence.empty()) {
        decisionMsg->addPar("queue") = decision.queueSequence[0];
    }
    if (!decision.switchSequence.empty()) {
        decisionMsg->addPar("switch") = decision.switchSequence[0];
    }
    
    send(decisionMsg, "decisionOut");
}

void OAFSModule::processOnlineAdjustment() {
    // 执行在线调整
    oafsCore->adjustTimeSlots();
    oafsCore->rebalanceFlows();
    
    // 发送调整信号
    emit(adjustmentSignal, 1);
}

void OAFSModule::submitFlow(FlowPacket* flow) {
    handleFlowPacket(flow);
}

void OAFSModule::reportFeedback(int flowId, double delay, bool satisfied) {
    oafsCore->receiveFeedback(flowId, delay, satisfied);
}

void OAFSModule::triggerAdjustment() {
    processOnlineAdjustment();
}

void OAFSModule::finish() {
    oafsCore->printStatistics();
}

// ==================== MFM (Multi-Flow Merging) Implementation ====================

bool OAFSCore::canApplyMFM(const OnlineFlow& flow) {
    // 小流检测: 流大小小于平均时隙容量的30%
    double avgSlotCapacity = linkRate * timeSlotSize / 8.0;
    return flow.size < avgSlotCapacity * 0.3;
}

std::vector<int> OAFSCore::findMergeableFlows(const OnlineFlow& flow, int targetSlot) {
    std::vector<int> mergeable;
    
    for (int s = 0; s < numSwitches; s++) {
        for (int p = 0; p < numPortsPerSwitch; p++) {
            OnlineTimeSlot& slot = timeSlots[s][p][targetSlot];
            
            for (int flowId : slot.flowIds) {
                auto it = activeFlows.find(flowId);
                if (it != activeFlows.end()) {
                    if (checkMFMConstraints(flow, it->second)) {
                        mergeable.push_back(flowId);
                    }
                }
            }
        }
    }
    
    return mergeable;
}

OnlineSchedulingDecision OAFSCore::applyMFM(OnlineFlow& flow, const std::vector<int>& mergeableFlows, int targetSlot) {
    OnlineSchedulingDecision decision;
    decision.flowId = flow.id;
    decision.isMFM = true;
    decision.mergedFlowIds = mergeableFlows;
    
    // 计算合并后的时隙分配
    decision.slotSequence.push_back(targetSlot);
    decision.queueSequence.push_back(flow.pcp % numQueues);
    decision.switchSequence.push_back(0);
    
    // 计算预期延迟
    decision.expectedDelay = calculateExpectedDelay(flow, decision);
    decision.confidence = calculateMFMEfficiency(mergeableFlows);
    
    // 更新时隙状态
    for (int s = 0; s < numSwitches; s++) {
        for (int p = 0; p < numPortsPerSwitch; p++) {
            timeSlots[s][p][targetSlot].flowIds.push_back(flow.id);
            timeSlots[s][p][targetSlot].remainingCapacity -= flow.size;
        }
    }
    
    return decision;
}

bool OAFSCore::checkMFMConstraints(const OnlineFlow& flow1, const OnlineFlow& flow2) {
    // 检查延迟约束兼容性
    double pdbDiff = std::abs(flow1.pdb - flow2.pdb);
    if (pdbDiff > std::min(flow1.pdb, flow2.pdb) * 0.5) {
        return false;
    }
    
    // 检查优先级兼容性
    if (std::abs(flow1.pcp - flow2.pcp) > 2) {
        return false;
    }
    
    // 检查时隙容量
    double totalSize = flow1.size + flow2.size;
    double slotCapacity = linkRate * timeSlotSize / 8.0;
    if (totalSize > slotCapacity * 0.8) {
        return false;
    }
    
    return true;
}

double OAFSCore::calculateMFMEfficiency(const std::vector<int>& flowIds) {
    if (flowIds.empty()) return 1.0;
    
    // 计算资源利用率提升
    double totalSize = 0;
    for (int id : flowIds) {
        auto it = activeFlows.find(id);
        if (it != activeFlows.end()) {
            totalSize += it->second.size;
        }
    }
    
    double slotCapacity = linkRate * timeSlotSize / 8.0;
    return std::min(1.0, totalSize / slotCapacity);
}

// ==================== MTM (Multi-Time-slot Merging) Implementation ====================

bool OAFSCore::canApplyMTM(const OnlineFlow& flow) {
    // 大流检测: 流大小大于平均时隙容量的80%
    double avgSlotCapacity = linkRate * timeSlotSize / 8.0;
    return flow.size > avgSlotCapacity * 0.8;
}

std::vector<int> OAFSCore::findConsecutiveSlots(int startSlot, int numSlots, int switchId, int portId) {
    std::vector<int> slots;
    
    for (int i = 0; i < numSlots && slots.size() < (size_t)numSlots; i++) {
        int slot = (startSlot + i) % this->numTimeSlots;
        OnlineTimeSlot& ts = timeSlots[switchId][portId][slot];
        
        // 检查时隙是否可用
        if (ts.utilization < adjustmentThreshold) {
            slots.push_back(slot);
        }
    }
    
    return slots;
}

OnlineSchedulingDecision OAFSCore::applyMTM(OnlineFlow& flow, const std::vector<int>& slots) {
    OnlineSchedulingDecision decision;
    decision.flowId = flow.id;
    decision.isMTM = true;
    decision.numMergedSlots = slots.size();
    
    // 分配连续时隙
    int remainingSize = flow.size;
    double slotCapacity = linkRate * timeSlotSize / 8.0;
    
    for (size_t i = 0; i < slots.size(); i++) {
        int slot = slots[i];
        decision.slotSequence.push_back(slot);
        decision.queueSequence.push_back(flow.pcp % numQueues);
        decision.switchSequence.push_back(0);
        
        // 更新时隙状态
        int allocSize = std::min(remainingSize, (int)slotCapacity);
        for (int s = 0; s < numSwitches; s++) {
            for (int p = 0; p < numPortsPerSwitch; p++) {
                timeSlots[s][p][slot].flowIds.push_back(flow.id);
                timeSlots[s][p][slot].remainingCapacity -= allocSize;
            }
        }
        remainingSize -= allocSize;
        
        if (remainingSize <= 0) break;
    }
    
    flow.assignedSlot = slots[0];
    decision.expectedDelay = calculateExpectedDelay(flow, decision);
    decision.confidence = calculateMTMEfficiency(flow, slots.size());
    
    return decision;
}

bool OAFSCore::checkMTMConstraints(const OnlineFlow& flow, const std::vector<int>& slots) {
    // 检查延迟约束
    double totalDelay = slots.size() * timeSlotSize;
    if (totalDelay > flow.pdb) {
        return false;
    }
    
    // 检查时隙连续性
    for (size_t i = 1; i < slots.size(); i++) {
        int expected = (slots[i-1] + 1) % numTimeSlots;
        if (slots[i] != expected) {
            return false;
        }
    }
    
    return true;
}

double OAFSCore::calculateMTMEfficiency(const OnlineFlow& flow, int numSlots) {
    double slotCapacity = linkRate * timeSlotSize / 8.0;
    double idealSlots = std::ceil(flow.size / slotCapacity);
    
    if (idealSlots <= 0) return 1.0;
    return idealSlots / numSlots;
}

// ==================== Algorithm 1: Slot-Path Mapping (论文完整实现) ====================

OnlineSchedulingDecision OAFSCore::slotPathMapping(const OnlineFlow& flow) {
    OnlineSchedulingDecision decision;
    decision.flowId = flow.id;
    decision.decisionTime = simTime();
    
    EV << "OAFS Algorithm 1: Slot-Path Mapping for Flow " << flow.id << std::endl;
    EV << "  Flow Size: " << flow.size << " bytes" << std::endl;
    EV << "  PDB: " << flow.pdb * 1000 << " ms" << std::endl;
    EV << "  Priority (PCP): " << flow.pcp << std::endl;
    
    // Step 1: 检查是否可以应用MFM (论文Section IV-B.1)
    if (canApplyMFM(flow)) {
        EV << "  Checking MFM applicability..." << std::endl;
        int arrivalSlot = (int)(flow.arrivalTime / timeSlotSize) % numTimeSlots;
        auto mergeable = findMergeableFlows(flow, arrivalSlot);
        
        if (!mergeable.empty()) {
            EV << "  MFM: Found " << mergeable.size() << " mergeable flows" << std::endl;
            OnlineFlow mutableFlow = flow;
            decision = applyMFM(mutableFlow, mergeable, arrivalSlot);
            decision.isMFM = true;
            return decision;
        }
    }
    
    // Step 2: 检查是否可以应用MTM (论文Section IV-B.2)
    if (canApplyMTM(flow)) {
        EV << "  Checking MTM applicability..." << std::endl;
        int arrivalSlot = (int)(flow.arrivalTime / timeSlotSize) % numTimeSlots;
        double slotCapacity = linkRate * timeSlotSize / 8.0;
        int neededSlots = (int)std::ceil(flow.size / slotCapacity);
        
        auto consecutiveSlots = findConsecutiveSlots(arrivalSlot, neededSlots, 0, 0);
        
        if ((int)consecutiveSlots.size() >= neededSlots) {
            EV << "  MTM: Found " << consecutiveSlots.size() << " consecutive slots" << std::endl;
            OnlineFlow mutableFlow = flow;
            decision = applyMTM(mutableFlow, consecutiveSlots);
            decision.isMTM = true;
            return decision;
        }
    }
    
    // Step 3: 论文Algorithm 1 - 时隙路径映射
    EV << "  Applying standard slot-path mapping (Algorithm 1)..." << std::endl;
    
    // 3.1 计算到达时隙
    int arrivalSlot = (int)(flow.arrivalTime / timeSlotSize) % numTimeSlots;
    
    // 3.2 遍历所有交换机端口 (论文Algorithm 1 Line 2-8)
    struct PortSlotInfo {
        int switchId;
        int portId;
        int slotId;
        double score;
        int totalSlots;
    };
    std::vector<PortSlotInfo> portSlotCandidates;
    
    for (int sw = 0; sw < numSwitches; sw++) {
        for (int port = 0; port < numPortsPerSwitch; port++) {
            // 3.3 对每个优先级计算总时隙数 (论文Algorithm 1 Line 3-5)
            for (int queue = 0; queue < numQueues; queue++) {
                // 计算该优先级队列的可用时隙
                std::vector<int> availableSlots;
                
                for (int t = 0; t < numTimeSlots; t++) {
                    int slot = (arrivalSlot + t) % numTimeSlots;
                    double slotDelay = t * timeSlotSize;
                    
                    // 检查延迟约束
                    if (slotDelay > flow.pdb) break;
                    
                    OnlineTimeSlot& ts = timeSlots[sw][port][slot];
                    
                    // 检查容量约束
                    if (ts.remainingCapacity >= flow.size) {
                        availableSlots.push_back(slot);
                    }
                }
                
                if (!availableSlots.empty()) {
                    // 计算该端口的得分
                    double portScore = 0;
                    for (int slot : availableSlots) {
                        portScore += calculateSlotScore(slot, flow);
                    }
                    portScore /= availableSlots.size();
                    
                    // 考虑优先级匹配度
                    int priorityMatch = (queue == flow.pcp % numQueues) ? 50 : 0;
                    portScore += priorityMatch;
                    
                    portSlotCandidates.push_back({sw, port, availableSlots[0], portScore, (int)availableSlots.size()});
                }
            }
        }
    }
    
    // 3.4 选择最优端口和时隙 (论文Algorithm 1 Line 6-8)
    if (!portSlotCandidates.empty()) {
        // 按得分排序
        std::sort(portSlotCandidates.begin(), portSlotCandidates.end(),
                  [](const PortSlotInfo& a, const PortSlotInfo& b) {
                      return a.score > b.score;
                  });
        
        PortSlotInfo& best = portSlotCandidates[0];
        
        decision.slotSequence.push_back(best.slotId);
        decision.queueSequence.push_back(flow.pcp % numQueues);
        decision.switchSequence.push_back(best.switchId);
        
        // 计算预期延迟
        int slotDiff = (best.slotId - arrivalSlot + numTimeSlots) % numTimeSlots;
        decision.expectedDelay = slotDiff * timeSlotSize + flow.size * 8.0 / linkRate;
        decision.confidence = best.score / 200.0;  // 归一化
        
        // 更新时隙资源
        updateSlotResources(best.slotId, best.switchId, best.portId, flow.size);
        
        EV << "  Selected: Switch=" << best.switchId << " Port=" << best.portId 
           << " Slot=" << best.slotId << " Score=" << best.score << std::endl;
    } else {
        // 没有找到合适的时隙，选择最早可用的
        EV << "  Warning: No optimal slot found, using fallback strategy" << std::endl;
        int bestSlot = findOptimalSlot(flow, 0, 0);
        if (bestSlot >= 0) {
            decision.slotSequence.push_back(bestSlot);
            decision.queueSequence.push_back(flow.pcp % numQueues);
            decision.switchSequence.push_back(0);
            decision.expectedDelay = calculateExpectedDelay(flow, decision);
            decision.confidence = 0.5;
            updateSlotResources(bestSlot, 0, 0, flow.size);
        }
    }
    
    return decision;
}

int OAFSCore::selectBestSlot(const OnlineFlow& flow, const std::vector<int>& candidateSlots) {
    if (candidateSlots.empty()) return -1;
    
    int bestSlot = candidateSlots[0];
    double bestScore = calculateSlotScore(candidateSlots[0], flow);
    
    for (size_t i = 1; i < candidateSlots.size(); i++) {
        double score = calculateSlotScore(candidateSlots[i], flow);
        if (score > bestScore) {
            bestScore = score;
            bestSlot = candidateSlots[i];
        }
    }
    
    return bestSlot;
}

double OAFSCore::calculateSlotScore(int slotId, const OnlineFlow& flow) {
    double score = 0;
    
    // 因素1: 时隙利用率 (偏好低利用率)
    double utilization = 0;
    for (int s = 0; s < numSwitches; s++) {
        for (int p = 0; p < numPortsPerSwitch; p++) {
            utilization += timeSlots[s][p][slotId].utilization;
        }
    }
    utilization /= (numSwitches * numPortsPerSwitch);
    score += (1.0 - utilization) * 100;
    
    // 因素2: 延迟裕度 (偏好有足够延迟裕度的时隙)
    int arrivalSlot = (int)(flow.arrivalTime / timeSlotSize) % numTimeSlots;
    int slotDiff = (slotId - arrivalSlot + numTimeSlots) % numTimeSlots;
    double slotDelay = slotDiff * timeSlotSize;
    double delayMargin = flow.pdb - slotDelay;
    score += delayMargin * 1000;
    
    // 因素3: 剩余容量 (偏好有足够容量的时隙)
    double remainingCapacity = 0;
    for (int s = 0; s < numSwitches; s++) {
        for (int p = 0; p < numPortsPerSwitch; p++) {
            remainingCapacity += timeSlots[s][p][slotId].remainingCapacity;
        }
    }
    score += remainingCapacity / 1000.0;
    
    return score;
}

void OAFSCore::updateSlotResources(int slotId, int switchId, int portId, int flowSize) {
    timeSlots[switchId][portId][slotId].flowIds.push_back(flowCounter);
    timeSlots[switchId][portId][slotId].remainingCapacity -= flowSize;
    timeSlots[switchId][portId][slotId].actualLoad += flowSize;
    timeSlots[switchId][portId][slotId].utilization = 
        1.0 - (double)timeSlots[switchId][portId][slotId].remainingCapacity / 1500.0;
}

Define_Module(OAFSModule);
