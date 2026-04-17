#include "TSNEndDevice.h"
#include "../messages/FlowPacket_m.h"
#include <iostream>
#include <fstream>
#include <iomanip>

Define_Module(TSNEndDevice);

TSNEndDevice::TSNEndDevice() : deviceId(0), isReceiver(true),
                                totalReceivedPackets(0), totalSentPackets(0),
                                totalDelay(0.0), satisfiedFlows(0), totalFlows(0) {}

TSNEndDevice::~TSNEndDevice() {}

void TSNEndDevice::initialize() {
    deviceId = par("deviceId");
    isReceiver = par("isReceiver");
    
    endToEndDelaySignal = registerSignal("endToEndDelay");
    satisfactionSignal = registerSignal("satisfaction");
    throughputSignal = registerSignal("throughput");
    
    EV << "TSN End Device " << deviceId << " initialized as " 
       << (isReceiver ? "receiver" : "sender") << std::endl;
}

void TSNEndDevice::handleMessage(cMessage* msg) {
    if (msg->arrivedOn("in")) {
        processReceivedPacket(msg);
    } else {
        sendResponse(msg);
    }
}

void TSNEndDevice::finish() {
    printStatistics();
}

void TSNEndDevice::processReceivedPacket(cMessage* msg) {
    totalReceivedPackets++;
    
    FlowPacket* flowPkt = dynamic_cast<FlowPacket*>(msg);
    
    double tstart = 0.0;
    double pdb = 0.004;
    int flowId = totalReceivedPackets;
    int pcp = 0;
    int size = 0;
    
    if (flowPkt) {
        tstart = flowPkt->getTstart();
        pdb = flowPkt->getPdb();
        flowId = flowPkt->getFlowId();
        pcp = flowPkt->getPcp();
        size = flowPkt->getSize();
    } else {
        if (msg->hasPar("tstart")) tstart = msg->par("tstart").doubleValue();
        if (msg->hasPar("pdb")) pdb = msg->par("pdb").doubleValue();
        if (msg->hasPar("flowId")) flowId = (int)msg->par("flowId").longValue();
        if (msg->hasPar("pcp")) pcp = (int)msg->par("pcp").longValue();
        cPacket* pkt = dynamic_cast<cPacket*>(msg);
        if (pkt) size = pkt->getByteLength();
    }
    
    double e2eDelay = simTime().dbl() - tstart;
    
    flowDelays[flowId] = e2eDelay;
    flowStartTimes[flowId] = tstart;
    totalDelay += e2eDelay;
    totalFlows++;
    
    bool satisfied = (e2eDelay <= pdb);
    if (satisfied) {
        satisfiedFlows++;
    }
    
    emit(endToEndDelaySignal, e2eDelay);
    emit(satisfactionSignal, satisfied ? 1 : 0);
    
    EV << "TSN End Device " << deviceId 
       << " received packet flowId=" << flowId
       << " pcp=" << pcp
       << " size=" << size
       << " E2E delay: " << e2eDelay * 1000 << " ms"
       << " (deadline: " << pdb * 1000 << " ms)"
       << " - " << (satisfied ? "SATISFIED" : "VIOLATED") << std::endl;
    
    delete msg;
}

void TSNEndDevice::sendResponse(cMessage* msg) {
    totalSentPackets++;
    send(msg, "out");
}

double TSNEndDevice::getAverageDelay() const {
    if (totalFlows == 0) return 0.0;
    return totalDelay / totalFlows;
}

double TSNEndDevice::getSatisfactionRatio() const {
    if (totalFlows == 0) return 0.0;
    return static_cast<double>(satisfiedFlows) / totalFlows;
}

double TSNEndDevice::getThroughput() const {
    double simTimeValue = simTime().dbl();
    if (simTimeValue <= 0) return 0.0;
    return totalReceivedPackets / simTimeValue;
}

void TSNEndDevice::printStatistics() const {
    EV << "=== TSN End Device " << deviceId << " Statistics ===" << std::endl;
    EV << "Total received packets: " << totalReceivedPackets << std::endl;
    EV << "Total sent packets: " << totalSentPackets << std::endl;
    EV << "Average E2E delay: " << getAverageDelay() * 1000 << " ms" << std::endl;
    EV << "Satisfaction ratio: " << getSatisfactionRatio() * 100 << "%" << std::endl;
    EV << "Throughput: " << getThroughput() << " packets/s" << std::endl;
    EV << "======================================" << std::endl;
}
