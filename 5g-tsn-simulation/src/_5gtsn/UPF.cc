#include "UPF.h"
#include "../messages/FlowPacket_m.h"

Define_Module(UPF);

UPF::UPF() : upfId(0), numUes(2), numGateways(1),
             processingTimer(nullptr), processingInterval(0.0001),
             nextTunnelId(1) {
}

UPF::~UPF() {
    cancelAndDelete(processingTimer);
    
    for (auto& pair : uplinkQueues) {
        while (!pair.second.empty()) {
            delete pair.second.front();
            pair.second.pop();
        }
    }
    for (auto& pair : downlinkQueues) {
        while (!pair.second.empty()) {
            delete pair.second.front();
            pair.second.pop();
        }
    }
}

void UPF::initialize() {
    upfId = par("upfId");
    numUes = par("numUes");
    numGateways = par("numGateways");
    processingInterval = par("processingInterval");
    
    for (int i = 0; i < numUes; i++) {
        uplinkQueues[i] = std::queue<cMessage*>();
    }
    for (int i = 0; i < numGateways; i++) {
        downlinkQueues[i] = std::queue<cMessage*>();
    }
    
    processingTimer = new cMessage("ProcessingTimer");
    scheduleAt(simTime() + processingInterval, processingTimer);
    
    packetForwardedSignal = registerSignal("packetForwarded");
    tunnelCreatedSignal = registerSignal("tunnelCreated");
    trafficStatsSignal = registerSignal("trafficStats");
    
    EV << "UPF " << upfId << " initialized with user plane tunnel support" << std::endl;
    EV << "  Number of UEs: " << numUes << std::endl;
    EV << "  Number of Gateways: " << numGateways << std::endl;
}

void UPF::handleMessage(cMessage* msg) {
    if (msg == processingTimer) {
        for (int i = 0; i < numUes; i++) {
            if (!uplinkQueues[i].empty()) {
                cMessage* pkt = uplinkQueues[i].front();
                uplinkQueues[i].pop();
                processUplinkPacket(pkt);
            }
        }
        for (int i = 0; i < numGateways; i++) {
            if (!downlinkQueues[i].empty()) {
                cMessage* pkt = downlinkQueues[i].front();
                downlinkQueues[i].pop();
                processDownlinkPacket(pkt);
            }
        }
        scheduleAt(simTime() + processingInterval, processingTimer);
        return;
    }
    
    if (msg->arrivedOn("gncIn")) {
        int ueId = msg->getArrivalGate()->getIndex();
        if (ueId >= 0 && ueId < numUes) {
            uplinkQueues[ueId].push(msg);
        } else {
            delete msg;
        }
    } else if (msg->arrivedOn("gatewayIn")) {
        int gatewayId = msg->getArrivalGate()->getIndex();
        if (gatewayId >= 0 && gatewayId < numGateways) {
            downlinkQueues[gatewayId].push(msg);
        } else {
            delete msg;
        }
    } else {
        delete msg;
    }
}

void UPF::finish() {
    printStatistics();
}

void UPF::processUplinkPacket(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    if (!pkt) {
        delete msg;
        return;
    }
    
    int ueId = pkt->getSrcNode();
    int qfi = pkt->getQfi();
    
    auto it = tunnels.find(qfi);
    int tunnelId;
    if (it == tunnels.end()) {
        tunnelId = createTunnel(ueId, 0, qfi);
    } else {
        tunnelId = it->second.tunnelId;
    }
    
    double processingDelay = simTime().dbl() - pkt->getArrivalTime();
    updateTrafficStats(qfi, pkt->getSize(), processingDelay);
    
    emit(packetForwardedSignal, 1);
    
    forwardToGateway(msg);
}

void UPF::processDownlinkPacket(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    if (!pkt) {
        delete msg;
        return;
    }
    
    forwardToGNB(msg);
}

void UPF::forwardToGateway(cMessage* msg) {
    send(msg, "gatewayOut", 0);
}

void UPF::forwardToGNB(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    int ueId = pkt ? pkt->getDstNode() : 0;
    if (ueId >= 0 && ueId < numUes) {
        send(msg, "gnbOut", ueId);
    } else {
        delete msg;
    }
}

int UPF::createTunnel(int srcUeId, int dstGatewayId, int qfi) {
    TunnelInfo tunnel;
    tunnel.tunnelId = nextTunnelId++;
    tunnel.srcUeId = srcUeId;
    tunnel.dstGatewayId = dstGatewayId;
    tunnel.creationTime = simTime().dbl();
    tunnel.qfi = qfi;
    
    tunnels[qfi] = tunnel;
    
    emit(tunnelCreatedSignal, tunnel.tunnelId);
    
    EV << "UPF created tunnel " << tunnel.tunnelId 
       << " for QFI " << qfi 
       << " (UE " << srcUeId << " -> Gateway " << dstGatewayId << ")" << std::endl;
    
    return tunnel.tunnelId;
}

void UPF::updateTrafficStats(int qfi, int packetSize, double delay) {
    auto it = flowStats.find(qfi);
    if (it == flowStats.end()) {
        TrafficStats stats;
        stats.totalPackets = 0;
        stats.totalBytes = 0;
        stats.totalDelay = 0.0;
        stats.maxDelay = 0.0;
        flowStats[qfi] = stats;
        it = flowStats.find(qfi);
    }
    
    it->second.totalPackets++;
    it->second.totalBytes += packetSize;
    it->second.totalDelay += delay;
    if (delay > it->second.maxDelay) {
        it->second.maxDelay = delay;
    }
    
    emit(trafficStatsSignal, it->second.totalPackets);
}

int UPF::getTunnelCount() const {
    return tunnels.size();
}

void UPF::printStatistics() const {
    EV << "=== UPF " << upfId << " Statistics ===" << std::endl;
    EV << "Number of active tunnels: " << getTunnelCount() << std::endl;
    
    for (const auto& pair : flowStats) {
        int qfi = pair.first;
        const TrafficStats& stats = pair.second;
        double avgDelay = stats.totalPackets > 0 ? stats.totalDelay / stats.totalPackets : 0.0;
        
        EV << "QFI " << qfi << ":" << std::endl;
        EV << "  Total packets: " << stats.totalPackets << std::endl;
        EV << "  Total bytes: " << stats.totalBytes << std::endl;
        EV << "  Average delay: " << avgDelay * 1000 << " ms" << std::endl;
        EV << "  Max delay: " << stats.maxDelay * 1000 << " ms" << std::endl;
    }
    
    EV << "==========================" << std::endl;
}
