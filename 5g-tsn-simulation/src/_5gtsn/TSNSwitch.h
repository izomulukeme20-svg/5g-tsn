#ifndef TSNSWITCH_H_
#define TSNSWITCH_H_

#include <omnetpp.h>
#include "inet/linklayer/ethernet/common/MacRelayUnit.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/linklayer/ethernet/contract/IMacForwardingTable.h"
#include <vector>
#include <map>
#include <queue>
#include <fstream>

using namespace omnetpp;
using namespace inet;

struct TrafficFlowRecord {
    double timestamp;
    int flowId;
    int packetSize;
    int priority;
    int pcp;
    int assignedQueue;
    int assignedSlot;
    int inPort;
    int outPort;
    double arrivalTime;
    double departureTime;
    double delay;
    double deadline;
    bool satisfied;
    std::string flowType;
    std::string decisionSource;
};

class FlowPacket;

struct QueueEntry {
    Packet* packet;
    double arrivalTime;
    int flowId;
    int priority;
    int size;
};

struct GateControlEntry {
    std::vector<int> gateStates;
    double duration;
};

class TSNSwitch : public MacRelayUnit {
protected:
    int switchId;
    int numQueues;
    int numTimeSlots;
    double timeSlotSize;
    double hyperPeriod;
    double processingDelay;
    double linkRate;
    
    std::vector<std::vector<std::vector<int>>> gcl;
    std::vector<std::vector<std::queue<QueueEntry>>> portQueues;
    std::vector<std::vector<int>> queueLengths;
    std::vector<std::vector<double>> queueDelays;
    
    cMessage* gclTimer;
    int currentTimeSlot;
    
    simsignal_t delaySignal;
    simsignal_t queueLengthSignal;
    simsignal_t throughputSignal;
    simsignal_t droppedPacketsSignal;
    
    int totalProcessedPackets;
    int totalDroppedPackets;
    double totalDelay;
    
    std::ofstream trafficLog;
    std::ofstream gclSnapshotLog;
    std::ofstream trainingLogLog;
    std::ofstream gclChangesLog;
    
    bool enableTrafficRecording;
    int trafficRecordLimit;
    std::map<int, TrafficFlowRecord> pendingFlows;
    std::vector<TrafficFlowRecord> trafficRecords;
    int genericFlowIdCounter;
    
protected:
    virtual void initialize(int stage) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void finish() override;
    
public:
    TSNSwitch();
    virtual ~TSNSwitch();
    
    void loadGCL(const std::vector<std::vector<int>>& newGCL);
    
    int getCurrentTimeSlot() const { return currentTimeSlot; }
    std::vector<int> getOpenQueues(int portId) const;
    bool isGateOpen(int portId, int queueId) const;
    
    void updateGCL();
    
    double getQueueingDelay(int portId, int queueId) const;
    int getQueueLength(int portId, int queueId) const;
    double getAverageDelay() const;
    
    void printStatistics() const;
    void recordTrafficFlow(const TrafficFlowRecord& record);
    void recordGCLSnapshot();
};

#endif