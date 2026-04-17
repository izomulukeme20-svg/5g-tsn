#include "FlowGenerator.h"
#include "../messages/FlowPacket_m.h"
#include <sstream>

Define_Module(FlowGenerator);

FlowGenerator::FlowGenerator() : nodeId(0), numDestinations(1), isActive(false), flowIdCounter(0) {
    std::random_device rd;
    rng.seed(rd());
}

FlowGenerator::~FlowGenerator() {
    for (auto& pair : flowTimers) {
        cancelAndDelete(pair.second);
    }
}

void FlowGenerator::initialize() {
    if (hasPar("nodeId")) nodeId = par("nodeId");
    if (hasPar("numDestinations")) numDestinations = par("numDestinations");
    if (hasPar("isActive")) isActive = par("isActive");
    
    if (numDestinations < 1) numDestinations = 1;
    destDist = std::uniform_int_distribution<int>(0, numDestinations - 1);
    jitterDist = std::normal_distribution<double>(0.0, 0.0001);
    
    flowGeneratedSignal = registerSignal("flowGenerated");
    flowSizeSignal = registerSignal("flowSize");
    
    addFlowConfig(NETWORK_CONTROL, 7, 86, 500, 0.002, 0.002, "NetworkControl");
    addFlowConfig(ISOCHRONOUS, 6, 83, 100, 0.004, 0.004, "Isochronous");
    addFlowConfig(CYCLIC, 5, 85, 255, 0.008, 0.008, "Cyclic");
    addFlowConfig(VIDEO, 4, 82, 255, 0.016, 0.016, "Video");
    
    EV << "FlowGenerator " << nodeId << " initialized with " << flowConfigs.size() 
       << " flow types, isActive=" << isActive << std::endl;
    
    if (isActive) {
        startGeneration();
    }
}

void FlowGenerator::handleMessage(cMessage* msg) {
    for (auto& pair : flowTimers) {
        if (msg == pair.second) {
            int flowTypeIndex = pair.first;
            const FlowConfig& config = flowConfigs[flowTypeIndex];
            
            int destination = destDist(rng);
            FlowPacket* packet = createFlowPacket(config, destination);
            sendFlowPacket(packet);
            
            flowCounters[flowTypeIndex]++;
            emit(flowGeneratedSignal, 1);
            emit(flowSizeSignal, config.size);
            
            EV << "FlowGenerator " << nodeId << " sent " << config.name 
               << " flow, flowId=" << packet->getFlowId() 
               << ", pcp=" << config.pcp 
               << ", size=" << config.size 
               << ", dest=" << destination << std::endl;
            
            double nextTime = simTime().dbl() + config.period + jitterDist(rng);
            scheduleAt(nextTime, msg);
            return;
        }
    }
    
    delete msg;
}

void FlowGenerator::finish() {
    EV << "FlowGenerator " << nodeId << " generated " << getNumGeneratedFlows() << " flows" << std::endl;
}

void FlowGenerator::addFlowConfig(FlowType type, int pcp, int qfi, int size, 
                                   double period, double pdb, const std::string& name) {
    FlowConfig config;
    config.type = type;
    config.pcp = pcp;
    config.qfi = qfi;
    config.size = size;
    config.period = period;
    config.pdb = pdb;
    config.name = name;
    flowConfigs.push_back(config);
}

void FlowGenerator::startGeneration() {
    isActive = true;
    
    EV << "FlowGenerator " << nodeId << " starting generation of " << flowConfigs.size() << " flow types" << std::endl;
    
    for (size_t i = 0; i < flowConfigs.size(); i++) {
        std::stringstream ss;
        ss << "FlowTimer_" << i << "_" << flowConfigs[i].name;
        cMessage* timer = new cMessage(ss.str().c_str());
        flowTimers[i] = timer;
        flowCounters[i] = 0;
        
        double startTime = simTime().dbl() + flowConfigs[i].period * ((double)rand() / RAND_MAX);
        scheduleAt(startTime, timer);
        
        EV << "  Scheduled " << flowConfigs[i].name << " flow, period=" << flowConfigs[i].period * 1000 
           << "ms, pcp=" << flowConfigs[i].pcp << ", size=" << flowConfigs[i].size << std::endl;
    }
}

void FlowGenerator::stopGeneration() {
    isActive = false;
    
    for (auto& pair : flowTimers) {
        cancelEvent(pair.second);
    }
}

FlowPacket* FlowGenerator::createFlowPacket(const FlowConfig& config, int destination) {
    FlowPacket* packet = new FlowPacket("FlowPacket");
    
    packet->setFlowId(flowIdCounter++);
    packet->setSrcNode(nodeId);
    packet->setDstNode(destination);
    packet->setPcp(config.pcp);
    packet->setQfi(config.qfi);
    packet->setSize(config.size);
    packet->setTstart(simTime().dbl());
    packet->setPdb(config.pdb);
    packet->setArrivalTime(simTime().dbl());
    packet->setDeadline(simTime().dbl() + config.pdb);
    packet->setHopCount(0);
    packet->setIsScheduled(false);
    packet->setAssignedQueue(config.pcp % 8);
    packet->setAssignedSlot(-1);
    
    packet->setByteLength(config.size);
    
    return packet;
}

void FlowGenerator::sendFlowPacket(cMessage* packet) {
    send(packet, "socketOut");
}

int FlowGenerator::getNumGeneratedFlows() const {
    int total = 0;
    for (const auto& pair : flowCounters) {
        total += pair.second;
    }
    return total;
}

double FlowGenerator::getAverageFlowSize() const {
    if (flowConfigs.empty()) return 0.0;
    
    double totalSize = 0.0;
    for (const auto& config : flowConfigs) {
        totalSize += config.size;
    }
    return totalSize / flowConfigs.size();
}
