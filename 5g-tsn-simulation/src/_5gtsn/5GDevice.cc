/**
 * @file 5GDevice.cc
 * @brief 5G设备模块实现文件 - 增强版
 * 
 * 本文件实现了GDevice类，该类模拟5G网络中的用户设备(UE/CPE)行为。
 * 增强功能包括：
 * - 业务分类器（基于MAC/VLAN/Ethertype）
 * - QoS规则管理
 * - QoS Flow到DRB的映射
 * - SDAP协议层模拟
 * - 接入层排队（PDCP/RLC/MAC）
 * - BSR（缓冲区状态报告）
 */

#include "5GDevice.h"
#include "../messages/FlowPacket_m.h"

Define_Module(GDevice);

GDevice::GDevice() : deviceId(0), gnbId(0), numFlowTypes(4), isActive(false),
                     flowGenerator(nullptr), activityTimer(nullptr), activityInterval(0.001),
                     totalSentPackets(0), totalReceivedPackets(0),
                     bsrTimer(nullptr), bsrInterval(0.001),
                     nextQfi(1), nextDrbId(1) {
    std::random_device rd;
    rng.seed(rd());
    flowTypeDist = std::uniform_int_distribution<int>(0, 3);
}

GDevice::~GDevice() {
    cancelAndDelete(activityTimer);
    cancelAndDelete(bsrTimer);
    delete flowGenerator;
    
    for (auto& pair : accessLayerQueues) {
        while (!pair.second.packets.empty()) {
            delete pair.second.packets.front();
            pair.second.packets.pop();
        }
    }
}

void GDevice::initialize() {
    deviceId = par("deviceId");
    gnbId = par("gnbId");
    numFlowTypes = par("numFlowTypes");
    isActive = par("isActive");
    activityInterval = par("activityInterval");
    bsrInterval = par("bsrInterval");
    
    flowIntervals.resize(numFlowTypes);
    flowSizes.resize(numFlowTypes);
    flowPriorities.resize(numFlowTypes);
    flowDelayBounds.resize(numFlowTypes);
    
    setFlowConfig(NETWORK_CONTROL, 500, 7, 0.002, 0.002);
    setFlowConfig(ISOCHRONOUS, 100, 6, 0.004, 0.004);
    setFlowConfig(CYCLIC, 255, 5, 0.008, 0.008);
    setFlowConfig(VIDEO, 255, 4, 0.016, 0.016);
    
    flowGenerator = new FlowGenerator();
    flowGenerator->setNodeId(deviceId);
    flowGenerator->setNumDestinations(par("numDestinations"));
    
    packetSentSignal = registerSignal("packetSent");
    packetReceivedSignal = registerSignal("packetReceived");
    qosClassificationSignal = registerSignal("qosClassification");
    bufferStatusSignal = registerSignal("bufferStatus");
    
    activityTimer = new cMessage("ActivityTimer");
    bsrTimer = new cMessage("BSRTimer");
    
    initializeQosRules();
    initializeDrbMappings();
    initializeAccessLayerQueues();
    
    if (isActive) {
        startTraffic();
        scheduleAt(simTime() + bsrInterval, bsrTimer);
    }
    
    EV << "5G Device " << deviceId << " initialized with enhanced QoS features" << std::endl;
}

void GDevice::handleMessage(cMessage* msg) {
    if (msg == activityTimer) {
        generateTraffic();
        scheduleAt(simTime() + activityInterval, activityTimer);
        return;
    }
    
    if (msg == bsrTimer) {
        sendBufferStatusReport();
        scheduleAt(simTime() + bsrInterval, bsrTimer);
        return;
    }
    
    if (msg->isSelfMessage()) {
        delete msg;
        return;
    }
    
    if (msg->arrivedOn("in")) {
        receivePacket(msg);
    } else {
        sendPacket(msg);
    }
}

void GDevice::finish() {
    EV << "5G Device " << deviceId << " statistics:" << std::endl;
    EV << "  Total sent packets: " << totalSentPackets << std::endl;
    EV << "  Total received packets: " << totalReceivedPackets << std::endl;
    EV << "  Throughput: " << getThroughput() << " bps" << std::endl;
    EV << "  QoS Rules: " << qosRules.size() << std::endl;
    EV << "  DRB Mappings: " << drbMappings.size() << std::endl;
}

void GDevice::setFlowConfig(FlowType type, int size, int priority, double interval, double delayBound) {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < numFlowTypes) {
        flowSizes[idx] = size;
        flowPriorities[idx] = priority;
        flowIntervals[idx] = interval;
        flowDelayBounds[idx] = delayBound;
    }
}

void GDevice::startTraffic() {
    isActive = true;
    scheduleAt(simTime() + activityInterval, activityTimer);
}

void GDevice::stopTraffic() {
    isActive = false;
    cancelEvent(activityTimer);
    cancelEvent(bsrTimer);
}

void GDevice::generateTraffic() {
    if (!isActive) return;
    
    int flowType = flowTypeDist(rng);
    FlowPacket* packet = new FlowPacket("FlowPacket");
    
    packet->setFlowId(totalSentPackets);
    packet->setSrcNode(deviceId);
    packet->setDstNode(0);
    packet->setPcp(flowPriorities[flowType]);
    packet->setSize(flowSizes[flowType]);
    packet->setTstart(simTime().dbl());
    packet->setPdb(flowDelayBounds[flowType]);
    packet->setArrivalTime(simTime().dbl());
    packet->setDeadline(simTime().dbl() + flowDelayBounds[flowType]);
    packet->setHopCount(0);
    packet->setIsScheduled(false);
    packet->setAssignedQueue(0);
    packet->setAssignedSlot(-1);
    
    packet->setSrcMacAddress(deviceId + 100);
    packet->setDstMacAddress(200);
    packet->setEthertype(0x0800);
    packet->setVlanId(flowPriorities[flowType]);
    packet->setWirelessDelay(0.0);
    packet->setJitter(0.0);
    
    int qfi = classifyPacket(packet);
    packet->setQfi(qfi);
    
    int drbId = mapQosFlowToDrb(qfi);
    packet->setDrbId(drbId);
    
    enqueueToAccessLayer(packet, drbId);
    
    emit(qosClassificationSignal, qfi);
    
    totalSentPackets++;
    emit(packetSentSignal, 1);
}

void GDevice::sendPacket(cMessage* msg) {
    send(msg, "out");
}

void GDevice::receivePacket(cMessage* msg) {
    totalReceivedPackets++;
    emit(packetReceivedSignal, 1);
    
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    if (pkt && pkt->getTstart() > 0) {
        double delay = simTime().dbl() - pkt->getTstart();
        EV << "Device " << deviceId << " received packet with delay: " 
           << delay * 1000 << " ms" << std::endl;
    }
    
    delete msg;
}

int GDevice::classifyPacket(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    if (!pkt) return 1;
    
    int srcMac = pkt->getSrcMacAddress();
    int dstMac = pkt->getDstMacAddress();
    int ethertype = pkt->getEthertype();
    int vlanId = pkt->getVlanId();
    int pcp = pkt->getPcp();
    
    for (const auto& rule : qosRules) {
        bool match = true;
        
        if (rule.srcMac >= 0 && rule.srcMac != srcMac) match = false;
        if (rule.dstMac >= 0 && rule.dstMac != dstMac) match = false;
        if (rule.ethertype >= 0 && rule.ethertype != ethertype) match = false;
        if (rule.vlanId >= 0 && rule.vlanId != vlanId) match = false;
        if (rule.priority >= 0 && rule.priority != pcp) match = false;
        
        if (match) {
            return rule.qfi;
        }
    }
    
    return nextQfi++;
}

int GDevice::mapQosFlowToDrb(int qfi) {
    for (const auto& mapping : drbMappings) {
        if (mapping.qfi == qfi) {
            return mapping.drbId;
        }
    }
    
    int drbId = nextDrbId++;
    DRBMapping newMapping;
    newMapping.qfi = qfi;
    newMapping.drbId = drbId;
    drbMappings.push_back(newMapping);
    
    return drbId;
}

void GDevice::enqueueToAccessLayer(cMessage* msg, int drbId) {
    if (accessLayerQueues.find(drbId) == accessLayerQueues.end()) {
        AccessLayerQueue newQueue;
        newQueue.drbId = drbId;
        newQueue.bufferSize = 0;
        newQueue.maxBufferSize = 10000;
        accessLayerQueues[drbId] = newQueue;
    }
    
    AccessLayerQueue& queue = accessLayerQueues[drbId];
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    int packetSize = pkt ? pkt->getSize() : 100;
    
    if (queue.bufferSize + packetSize <= queue.maxBufferSize) {
        queue.packets.push(msg);
        queue.bufferSize += packetSize;
    } else {
        EV << "Buffer overflow for DRB " << drbId << ", dropping packet" << std::endl;
        delete msg;
    }
}

void GDevice::sendBufferStatusReport() {
    int totalBufferSize = 0;
    for (const auto& pair : accessLayerQueues) {
        totalBufferSize += pair.second.bufferSize;
    }
    
    emit(bufferStatusSignal, totalBufferSize);
    
    for (auto& pair : accessLayerQueues) {
        AccessLayerQueue& queue = pair.second;
        while (!queue.packets.empty()) {
            cMessage* pkt = queue.packets.front();
            queue.packets.pop();
            FlowPacket* fpkt = dynamic_cast<FlowPacket*>(pkt);
            if (fpkt) {
                queue.bufferSize -= fpkt->getSize();
            }
            sendPacket(pkt);
        }
    }
}

void GDevice::initializeQosRules() {
    QoSRule rule1;
    rule1.qfi = 1;
    rule1.srcMac = -1;
    rule1.dstMac = -1;
    rule1.ethertype = -1;
    rule1.vlanId = -1;
    rule1.priority = 7;
    qosRules.push_back(rule1);
    
    QoSRule rule2;
    rule2.qfi = 2;
    rule2.srcMac = -1;
    rule2.dstMac = -1;
    rule2.ethertype = -1;
    rule2.vlanId = -1;
    rule2.priority = 6;
    qosRules.push_back(rule2);
    
    QoSRule rule3;
    rule3.qfi = 3;
    rule3.srcMac = -1;
    rule3.dstMac = -1;
    rule3.ethertype = -1;
    rule3.vlanId = -1;
    rule3.priority = 5;
    qosRules.push_back(rule3);
    
    QoSRule rule4;
    rule4.qfi = 4;
    rule4.srcMac = -1;
    rule4.dstMac = -1;
    rule4.ethertype = -1;
    rule4.vlanId = -1;
    rule4.priority = 4;
    qosRules.push_back(rule4);
}

void GDevice::initializeDrbMappings() {
    for (int i = 1; i <= 4; i++) {
        DRBMapping mapping;
        mapping.qfi = i;
        mapping.drbId = i;
        drbMappings.push_back(mapping);
    }
}

void GDevice::initializeAccessLayerQueues() {
    for (int i = 1; i <= 4; i++) {
        AccessLayerQueue queue;
        queue.drbId = i;
        queue.bufferSize = 0;
        queue.maxBufferSize = 10000;
        accessLayerQueues[i] = queue;
    }
}

double GDevice::getThroughput() const {
    double simTimeValue = simTime().dbl();
    if (simTimeValue <= 0) return 0.0;
    
    double totalBits = 0.0;
    for (int i = 0; i < numFlowTypes; i++) {
        totalBits += flowSizes[i] * 8.0;
    }
    
    return (totalSentPackets * totalBits) / simTimeValue;
}

void GDevice::addQoSRule(const QoSRule& rule) {
    qosRules.push_back(rule);
}

void GDevice::addDrbMapping(const DRBMapping& mapping) {
    drbMappings.push_back(mapping);
}
