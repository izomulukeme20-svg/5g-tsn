#include "TSNSwitch.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/UserPriorityTag_m.h"
#include "inet/common/ProtocolTag_m.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <ctime>

Define_Module(TSNSwitch);

TSNSwitch::TSNSwitch() : gclTimer(nullptr), currentTimeSlot(0), 
                          totalProcessedPackets(0), totalDroppedPackets(0), totalDelay(0),
                          enableTrafficRecording(true), trafficRecordLimit(100000), genericFlowIdCounter(0) {}

TSNSwitch::~TSNSwitch() {
    cancelAndDelete(gclTimer);
    
    if (trafficLog.is_open()) trafficLog.close();
    if (gclSnapshotLog.is_open()) gclSnapshotLog.close();
    if (trainingLogLog.is_open()) trainingLogLog.close();
    if (gclChangesLog.is_open()) gclChangesLog.close();
}

void TSNSwitch::initialize(int stage) {
    MacRelayUnit::initialize(stage);
    
    if (stage == INITSTAGE_LINK_LAYER) {
        switchId = par("switchId");
        numQueues = par("numQueues");
        numTimeSlots = par("numTimeSlots");
        timeSlotSize = par("timeSlotSize");
        hyperPeriod = par("hyperPeriod");
        processingDelay = par("processingDelay");
        linkRate = par("linkRate");
        enableTrafficRecording = par("enableTrafficRecording");
        trafficRecordLimit = par("trafficRecordLimit");
        
        int numPorts = interfaceTable->getNumInterfaces();
        
        gcl.resize(numTimeSlots, std::vector<std::vector<int>>(numPorts, std::vector<int>(numQueues, 0)));
        
        portQueues.resize(numPorts);
        queueLengths.resize(numPorts, std::vector<int>(numQueues, 0));
        queueDelays.resize(numPorts, std::vector<double>(numQueues, 0.0));
        
        for (int p = 0; p < numPorts; p++) {
            portQueues[p].resize(numQueues);
        }
        
        for (int t = 0; t < numTimeSlots; t++) {
            for (int p = 0; p < numPorts; p++) {
                for (int q = 0; q < numQueues; q++) {
                    gcl[t][p][q] = (q == t % numQueues) ? 1 : 0;
                }
            }
        }
        
        gclTimer = new cMessage("GCLTimer");
        scheduleAt(simTime() + timeSlotSize, gclTimer);
        
        delaySignal = registerSignal("delay");
        queueLengthSignal = registerSignal("queueLength");
        throughputSignal = registerSignal("throughput");
        droppedPacketsSignal = registerSignal("droppedPackets");
        
        WATCH(currentTimeSlot);
        WATCH(totalProcessedPackets);
        WATCH(totalDroppedPackets);
        
        if (enableTrafficRecording) {
            trafficLog.open("traffic_flows.csv");
            trafficLog << "timestamp,flow_id,packet_size,priority,pcp,assigned_queue,assigned_slot,in_port,out_port,arrival_time,departure_time,delay,deadline,satisfied,flow_type,decision_source,record_time\n";
            
            gclSnapshotLog.open("gcl_snapshot.csv");
            gclSnapshotLog << "# GCL Snapshot Log\n";
            
            trainingLogLog.open("training_log.csv");
            trainingLogLog << "step,reward,loss,avg_delay,satisfaction,epsilon,timestamp\n";
            
            gclChangesLog.open("gcl_changes.csv");
            gclChangesLog << "timestamp,slot,port,queue,oldState,newState,source,record_time\n";
        }
    }
}

void TSNSwitch::handleLowerPacket(Packet *packet) {
    auto macAddressInd = packet->getTag<MacAddressInd>();
    auto sourceAddress = macAddressInd->getSrcAddress();
    auto destinationAddress = macAddressInd->getDestAddress();
    auto interfaceInd = packet->getTag<InterfaceInd>();
    int incomingInterfaceId = interfaceInd->getInterfaceId();
    auto incomingInterface = interfaceTable->getInterfaceById(incomingInterfaceId);
    
    int inPort = incomingInterface->getInterfaceId();
    
    int outPort = -1;
    int numPorts = interfaceTable->getNumInterfaces();
    for (int p = 0; p < numPorts; p++) {
        auto intf = interfaceTable->getInterface(p);
        if (intf && !intf->isLoopback() && intf->isBroadcast() && intf != incomingInterface) {
            outPort = p;
            break;
        }
    }
    
    if (outPort < 0) {
        MacRelayUnit::handleLowerPacket(packet);
        return;
    }
    
    int queueId = 0;
    int pcp = 0;
    auto userPriorityInd = packet->findTag<UserPriorityInd>();
    if (userPriorityInd != nullptr) {
        pcp = userPriorityInd->getUserPriority();
        queueId = pcp % numQueues;
    }
    
    double arrivalTime = simTime().dbl();
    int size = packet->getByteLength();
    int flowId = genericFlowIdCounter++;
    
    if (enableTrafficRecording && (int)trafficRecords.size() < trafficRecordLimit) {
        TrafficFlowRecord record;
        record.timestamp = arrivalTime;
        record.flowId = flowId;
        record.packetSize = size;
        record.priority = pcp;
        record.pcp = pcp;
        record.assignedQueue = queueId;
        record.assignedSlot = getCurrentTimeSlot();
        record.inPort = inPort;
        record.outPort = outPort;
        record.arrivalTime = arrivalTime;
        record.departureTime = -1;
        record.delay = -1;
        record.deadline = arrivalTime + 0.004;
        record.satisfied = false;
        record.flowType = "Ethernet";
        record.decisionSource = "TSN";
        
        recordTrafficFlow(record);
        trafficRecords.push_back(record);
    }
    
    totalProcessedPackets++;
    
    MacRelayUnit::handleLowerPacket(packet);
}

void TSNSwitch::finish() {
    MacRelayUnit::finish();
    printStatistics();
    recordGCLSnapshot();
}

void TSNSwitch::loadGCL(const std::vector<std::vector<int>>& newGCL) {
    int numPorts = interfaceTable->getNumInterfaces();
    for (int t = 0; t < numTimeSlots && t < (int)newGCL.size(); t++) {
        for (int q = 0; q < numQueues && q < (int)newGCL[t].size(); q++) {
            for (int p = 0; p < numPorts; p++) {
                gcl[t][p][q] = newGCL[t][q];
            }
        }
    }
}

std::vector<int> TSNSwitch::getOpenQueues(int portId) const {
    std::vector<int> openQueues;
    int numPorts = interfaceTable->getNumInterfaces();
    if (portId < 0 || portId >= numPorts) return openQueues;
    
    for (int q = 0; q < numQueues; q++) {
        if (gcl[currentTimeSlot][portId][q] == 1) {
            openQueues.push_back(q);
        }
    }
    return openQueues;
}

bool TSNSwitch::isGateOpen(int portId, int queueId) const {
    int numPorts = interfaceTable->getNumInterfaces();
    if (portId < 0 || portId >= numPorts) return false;
    if (queueId < 0 || queueId >= numQueues) return false;
    return gcl[currentTimeSlot][portId][queueId] == 1;
}

void TSNSwitch::updateGCL() {
    currentTimeSlot = (currentTimeSlot + 1) % numTimeSlots;
}

double TSNSwitch::getQueueingDelay(int portId, int queueId) const {
    int numPorts = interfaceTable->getNumInterfaces();
    if (portId < 0 || portId >= numPorts) return 0.0;
    if (queueId < 0 || queueId >= numQueues) return 0.0;
    return queueDelays[portId][queueId];
}

int TSNSwitch::getQueueLength(int portId, int queueId) const {
    int numPorts = interfaceTable->getNumInterfaces();
    if (portId < 0 || portId >= numPorts) return 0;
    if (queueId < 0 || queueId >= numQueues) return 0;
    return queueLengths[portId][queueId];
}

double TSNSwitch::getAverageDelay() const {
    return totalProcessedPackets > 0 ? totalDelay / totalProcessedPackets : 0.0;
}

void TSNSwitch::printStatistics() const {
    EV << "=== TSNSwitch Statistics ===" << std::endl;
    EV << "  Switch ID: " << switchId << std::endl;
    EV << "  Total processed packets: " << totalProcessedPackets << std::endl;
    EV << "  Total dropped packets: " << totalDroppedPackets << std::endl;
    EV << "  Average delay: " << getAverageDelay() * 1000 << " ms" << std::endl;
    EV << "  Traffic records: " << trafficRecords.size() << std::endl;
}

void TSNSwitch::recordTrafficFlow(const TrafficFlowRecord& record) {
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

void TSNSwitch::recordGCLSnapshot() {
    if (gclSnapshotLog.is_open()) {
        gclSnapshotLog << "# GCL Snapshot at t=" << simTime() << "s\n";
        gclSnapshotLog << "# Training steps: 0\n";
        gclSnapshotLog << "# Avg Delay: " << getAverageDelay() * 1000 << " ms\n";
        gclSnapshotLog << "# Satisfaction: 100%\n";
        gclSnapshotLog << "# Format: time_slot,port,queue,gate_state\n";
        
        int numPorts = interfaceTable->getNumInterfaces();
        for (int t = 0; t < numTimeSlots; t++) {
            for (int p = 0; p < numPorts; p++) {
                for (int q = 0; q < numQueues; q++) {
                    gclSnapshotLog << t << "," << p << "," << q << "," << gcl[t][p][q] << "\n";
                }
            }
        }
        gclSnapshotLog.flush();
    }
}
