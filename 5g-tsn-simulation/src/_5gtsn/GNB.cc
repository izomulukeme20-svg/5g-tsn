#include "GNB.h"
#include "../messages/FlowPacket_m.h"
#include <cmath>

Define_Module(GNB);

double WirelessChannel::calculatePathLoss(double distance) const {
    const double referenceDistance = 1.0;
    const double referencePathLoss = 28.0 + 22.0 * std::log10(frequency / 1e9);
    
    return referencePathLoss + 10.0 * pathLossExponent * std::log10(distance / referenceDistance);
}

double WirelessChannel::calculateSNR(double distance, double txPower) const {
    const double k_B = 1.38e-23;
    const double T = 290.0;
    const double thermalNoise = k_B * T * bandwidth;
    
    double pathLoss = calculatePathLoss(distance);
    double receivedPower = txPower - pathLoss;
    double noise = thermalNoise + noiseFigure;
    
    return receivedPower - noise;
}

double WirelessChannel::calculateDelay(double distance, int packetSize) const {
    double snr = calculateSNR(distance, txPower);
    double spectralEfficiency = std::log2(1.0 + std::pow(10.0, snr / 10.0));
    double transmissionTime = (packetSize * 8.0) / (bandwidth * spectralEfficiency);
    
    const double propagationSpeed = 3e8;
    double propagationDelay = distance / propagationSpeed;
    
    return transmissionTime + propagationDelay;
}

double WirelessChannel::calculateJakesFading(double time, double dopplerShift) const {
    const int numWaves = 8;
    double fading = 0.0;
    
    for (int n = 0; n < numWaves; n++) {
        double phase = 2.0 * M_PI * n / numWaves;
        double angle = 2.0 * M_PI * dopplerShift * time * std::cos(phase);
        fading += std::cos(angle + phase);
    }
    
    return std::abs(fading) / numWaves;
}

GNB::GNB() : gnbId(0), numUes(0), frequency(5.9e9), bandwidth(100e6), 
             txPower(46.0), antennaGain(10.0), noiseFigure(5.0),
             processingTimer(nullptr), schedulingTimer(nullptr),
             processingInterval(0.0001), schedulingInterval(0.001),
             jitterMean(0.0), jitterStd(0.0005),
             ueVelocity(3.0), dopplerShift(0.0),
             totalUplinkPackets(0), totalDownlinkPackets(0),
             totalUplinkDelay(0.0), totalDownlinkDelay(0.0),
             totalResourceBlocks(100), usedResourceBlocks(0) {
    std::random_device rd;
    rng.seed(rd());
    jitterDist = std::normal_distribution<double>(jitterMean, jitterStd);
}

GNB::~GNB() {
    cancelAndDelete(processingTimer);
    cancelAndDelete(schedulingTimer);
    
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

void GNB::initialize() {
    gnbId = par("gnbId");
    numUes = par("numUes");
    frequency = par("frequency");
    bandwidth = par("bandwidth");
    txPower = par("txPower");
    antennaGain = par("antennaGain");
    noiseFigure = par("noiseFigure");
    jitterStd = par("jitterStd");
    ueVelocity = par("ueVelocity");
    
    const double c = 3e8;
    dopplerShift = ueVelocity * frequency / c;
    
    jitterDist = std::normal_distribution<double>(jitterMean, jitterStd);
    
    WirelessChannel defaultChannel;
    defaultChannel.channelId = 0;
    defaultChannel.frequency = frequency;
    defaultChannel.bandwidth = bandwidth;
    defaultChannel.txPower = txPower;
    defaultChannel.noiseFigure = noiseFigure;
    defaultChannel.pathLossExponent = 3.5;
    defaultChannel.shadowingStd = 4.0;
    channels[0] = defaultChannel;
    
    connectedUes.reserve(numUes);
    for (int i = 0; i < numUes; i++) {
        UeInfo ue;
        ue.ueId = i;
        ue.lastContactTime = 0.0;
        ue.numPackets = 0;
        ue.bufferStatus = 0;
        connectedUes.push_back(ue);
        
        uplinkQueues[i] = std::queue<cMessage*>();
        downlinkQueues[i] = std::queue<cMessage*>();
        
        std::vector<HARQProcess> ueHarq;
        for (int j = 0; j < 8; j++) {
            HARQProcess hp;
            hp.processId = j;
            hp.active = false;
            hp.retransmissionCount = 0;
            hp.maxRetransmissions = 3;
            hp.lastTransmissionTime = 0.0;
            ueHarq.push_back(hp);
        }
        harqProcesses[i] = ueHarq;
    }
    
    processingTimer = new cMessage("ProcessingTimer");
    schedulingTimer = new cMessage("SchedulingTimer");
    scheduleAt(simTime() + processingInterval, processingTimer);
    scheduleAt(simTime() + schedulingInterval, schedulingTimer);
    
    uplinkDelaySignal = registerSignal("uplinkDelay");
    downlinkDelaySignal = registerSignal("downlinkDelay");
    channelQualitySignal = registerSignal("channelQuality");
    rbUtilizationSignal = registerSignal("rbUtilization");
    
    EV << "gNB " << gnbId << " initialized with enhanced RB scheduling" << std::endl;
    EV << "  Frequency: " << frequency / 1e9 << " GHz" << std::endl;
    EV << "  Doppler shift: " << dopplerShift << " Hz" << std::endl;
    EV << "  UE velocity: " << ueVelocity << " m/s" << std::endl;
    EV << "  Total RBs: " << totalResourceBlocks << std::endl;
}

void GNB::handleMessage(cMessage* msg) {
    if (msg == processingTimer) {
        for (int i = 0; i < numUes; i++) {
            if (!uplinkQueues[i].empty()) {
                cMessage* pkt = uplinkQueues[i].front();
                uplinkQueues[i].pop();
                processUplinkPacket(pkt);
            }
            if (!downlinkQueues[i].empty()) {
                cMessage* pkt = downlinkQueues[i].front();
                downlinkQueues[i].pop();
                processDownlinkPacket(pkt);
            }
        }
        scheduleAt(simTime() + processingInterval, processingTimer);
        return;
    }
    
    if (msg == schedulingTimer) {
        performRBScheduling();
        scheduleAt(simTime() + schedulingInterval, schedulingTimer);
        return;
    }
    
    if (msg->arrivedOn("ueIn")) {
        int ueId = msg->getArrivalGate()->getIndex();
        uplinkQueues[ueId].push(msg);
    } else if (msg->arrivedOn("coreIn")) {
        FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
        int ueId = pkt ? pkt->getDstNode() : 0;
        if (ueId >= 0 && ueId < numUes) {
            downlinkQueues[ueId].push(msg);
        } else {
            delete msg;
        }
    } else {
        delete msg;
    }
}

void GNB::finish() {
    printStatistics();
}

void GNB::processUplinkPacket(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    int ueId = pkt ? pkt->getSrcNode() : 0;
    int packetSize = pkt ? pkt->getSize() : 100;
    
    double wirelessDelay = calculateWirelessDelay(ueId, packetSize);
    wirelessDelay = addJitter(wirelessDelay);
    
    if (pkt) {
        pkt->setWirelessDelay(wirelessDelay);
        pkt->setJitter(wirelessDelay - (wirelessDelay - jitterMean));
        pkt->setArrivalTime(simTime().dbl());
    }
    
    totalUplinkPackets++;
    totalUplinkDelay += wirelessDelay;
    
    emit(uplinkDelaySignal, wirelessDelay);
    
    manageHARQ(ueId, pkt ? pkt->getDrbId() : 1, true);
    
    forwardToCore(msg);
}

void GNB::processDownlinkPacket(cMessage* msg) {
    FlowPacket* pkt = dynamic_cast<FlowPacket*>(msg);
    int ueId = pkt ? pkt->getDstNode() : 0;
    int packetSize = pkt ? pkt->getSize() : 100;
    
    double wirelessDelay = calculateWirelessDelay(ueId, packetSize);
    wirelessDelay = addJitter(wirelessDelay);
    
    totalDownlinkPackets++;
    totalDownlinkDelay += wirelessDelay;
    
    emit(downlinkDelaySignal, wirelessDelay);
    
    forwardToUe(msg, ueId);
}

void GNB::forwardToCore(cMessage* msg) {
    send(msg, "coreOut");
}

void GNB::forwardToUe(cMessage* msg, int ueId) {
    if (ueId >= 0 && ueId < numUes) {
        send(msg, "ueOut", ueId);
    } else {
        delete msg;
    }
}

double GNB::calculateWirelessDelay(int ueId, int packetSize) {
    if (channels.empty()) return 0.001;
    
    const WirelessChannel& channel = channels.begin()->second;
    double distance = 100.0 + (ueId % 10) * 50.0;
    
    double baseDelay = channel.calculateDelay(distance, packetSize);
    
    double fadingFactor = channel.calculateJakesFading(simTime().dbl(), dopplerShift);
    double fadingDelay = baseDelay * (1.0 + fadingFactor);
    
    return fadingDelay;
}

double GNB::addJitter(double baseDelay) {
    double jitter = jitterDist(rng);
    return std::max(0.0, baseDelay + jitter);
}

void GNB::performRBScheduling() {
    usedResourceBlocks = 0;
    
    while (usedResourceBlocks < totalResourceBlocks) {
        RBSchedulingDecision decision = selectUeForScheduling();
        
        if (decision.ueId < 0) break;
        
        usedResourceBlocks += decision.numRBs;
        
        if (decision.ueId >= 0 && decision.ueId < numUes) {
            connectedUes[decision.ueId].lastContactTime = simTime().dbl();
        }
    }
    
    double utilization = static_cast<double>(usedResourceBlocks) / totalResourceBlocks;
    emit(rbUtilizationSignal, utilization);
}

RBSchedulingDecision GNB::selectUeForScheduling() {
    RBSchedulingDecision decision;
    decision.ueId = -1;
    decision.drbId = 1;
    decision.numRBs = 0;
    decision.mcs = 0;
    decision.startTime = simTime().dbl();
    decision.duration = 0.001;
    
    int maxPriority = -1;
    int selectedUe = -1;
    
    for (int i = 0; i < numUes; i++) {
        if (!uplinkQueues[i].empty() || !downlinkQueues[i].empty()) {
            cMessage* frontMsg = !uplinkQueues[i].empty() ? uplinkQueues[i].front() : downlinkQueues[i].front();
            FlowPacket* pkt = dynamic_cast<FlowPacket*>(frontMsg);
            int priority = pkt ? pkt->getPcp() : 0;
            
            if (priority > maxPriority) {
                maxPriority = priority;
                selectedUe = i;
            }
        }
    }
    
    if (selectedUe >= 0) {
        decision.ueId = selectedUe;
        
        double distance = 100.0 + (selectedUe % 10) * 50.0;
        if (!channels.empty()) {
            const WirelessChannel& channel = channels.begin()->second;
            double snr = channel.calculateSNR(distance, txPower);
            decision.mcs = selectMCS(snr);
            emit(channelQualitySignal, snr);
        }
        
        decision.numRBs = 10;
    }
    
    return decision;
}

int GNB::selectMCS(double snr) {
    if (snr > 25) return 28;
    if (snr > 20) return 24;
    if (snr > 15) return 20;
    if (snr > 10) return 16;
    if (snr > 5) return 12;
    if (snr > 0) return 8;
    return 4;
}

void GNB::manageHARQ(int ueId, int drbId, bool success) {
    if (ueId < 0 || ueId >= numUes) return;
    
    auto& ueHarq = harqProcesses[ueId];
    for (auto& hp : ueHarq) {
        if (!hp.active) {
            hp.active = true;
            hp.retransmissionCount = 0;
            hp.lastTransmissionTime = simTime().dbl();
            break;
        }
    }
}

void GNB::connectUe(int ueId) {
    if (ueId >= 0 && ueId < (int)connectedUes.size()) {
        connectedUes[ueId].lastContactTime = simTime().dbl();
    }
}

void GNB::disconnectUe(int ueId) {
    if (ueId >= 0 && ueId < (int)connectedUes.size()) {
        connectedUes[ueId].lastContactTime = 0.0;
    }
}

int GNB::getNumConnectedUes() const {
    int count = 0;
    for (const auto& ue : connectedUes) {
        if (ue.lastContactTime > 0) count++;
    }
    return count;
}

double GNB::getAverageUplinkDelay() const {
    if (totalUplinkPackets == 0) return 0.0;
    return totalUplinkDelay / totalUplinkPackets;
}

double GNB::getAverageDownlinkDelay() const {
    if (totalDownlinkPackets == 0) return 0.0;
    return totalDownlinkDelay / totalDownlinkPackets;
}

double GNB::getRBUtilization() const {
    return static_cast<double>(usedResourceBlocks) / totalResourceBlocks;
}

void GNB::printStatistics() const {
    EV << "=== gNB " << gnbId << " Statistics ===" << std::endl;
    EV << "Connected UEs: " << getNumConnectedUes() << std::endl;
    EV << "Total uplink packets: " << totalUplinkPackets << std::endl;
    EV << "Total downlink packets: " << totalDownlinkPackets << std::endl;
    EV << "Average uplink delay: " << getAverageUplinkDelay() * 1000 << " ms" << std::endl;
    EV << "Average downlink delay: " << getAverageDownlinkDelay() * 1000 << " ms" << std::endl;
    EV << "RB Utilization: " << getRBUtilization() * 100 << "%" << std::endl;
    EV << "==========================" << std::endl;
}
