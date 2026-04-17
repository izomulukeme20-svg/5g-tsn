#ifndef _5GTSNGATEWAY_H_
#define _5GTSNGATEWAY_H_

#include <omnetpp.h>
#include "TorchEDRLAgent.h"
#include "OAFSModule.h"
#include <vector>
#include <map>
#include <queue>
#include <fstream>

using namespace omnetpp;

struct GCLTimeSlot {
    int slotId;
    double startTime;
    double endTime;
    bool gateOpen[8];
    int occupiedBytes[8];
};

struct PathSlotDecision {
    int switchId;
    int portId;
    int queueId;
    int slotId;
    double transmissionTime;
    double waitingTime;
};

struct TrafficFlowRecord {
    double timestamp;
    int flowId;
    int srcNode;
    int dstNode;
    int packetSize;
    int priority;
    int pcp;
    double tstart;
    double pdb;
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
    std::vector<PathSlotDecision> slotPath;
};

class _5GTSNGateway : public cSimpleModule {
private:
    TorchEDRLAgent* edrlAgent;
    OAFSCore* oafsCore;
    
    int gatewayId;
    int numTimeSlots;
    int numQueues;
    int numSwitches;
    int stateSize;
    double timeSlotSize;
    double hyperPeriod;
    double linkRate;
    
    cMessage* trainingTimer;
    cMessage* gclUpdateTimer;
    double trainingInterval;
    double gclUpdateInterval;
    
    bool enableEDRL;
    bool enableOAFS;
    bool isTraining;
    bool enableMFM; // Multi-flow Merging
    bool enableMTM; // Multi-time-slot Merging
    
    int totalProcessedFlows;
    int totalSatisfiedFlows;
    double totalDelay;
    
    std::ofstream trafficLog;
    std::ofstream gclSnapshotLog;
    std::ofstream trainingLogLog;
    std::ofstream gclChangesLog;
    
    simsignal_t flowProcessedSignal;
    simsignal_t delaySignal;
    simsignal_t satisfactionSignal;
    simsignal_t gclGeneratedSignal;
    simsignal_t jitterCompensationSignal;
    
    std::vector<std::vector<GCLTimeSlot>> switchGCLs;
    std::map<int, double> flowJitterHistory;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void process5GFlow(cMessage* msg);
    void processTSNFlow(cMessage* msg);
    
    void trainEDRL();
    void updateGCL();
    void distributeGCLToSwitches();
    
    std::vector<PathSlotDecision> findOptimalSlotPath(FlowPacket* pkt);
    double calculatePathDelay(const std::vector<PathSlotDecision>& path);
    bool checkSatisfiability(const std::vector<PathSlotDecision>& path, double deadline);
    void mergeMultipleFlows(std::vector<FlowPacket*>& flows);
    void mergeMultipleTimeSlots(std::vector<GCLTimeSlot>& slots);
    void compensateJitter(FlowPacket* pkt, std::vector<PathSlotDecision>& path);
    
    void printStatistics();
    void recordTrafficFlow(const TrafficFlowRecord& record);
    void recordGCLSnapshot();
    void recordTrainingLog(int step, double reward, double loss, double avgDelay, double satisfaction, double epsilon);
    void recordGCLChange(int slot, int port, int queue, int oldState, int newState, const std::string& source);
    
public:
    _5GTSNGateway();
    virtual ~_5GTSNGateway();
    
    double getAverageDelay() const;
    double getSatisfactionRatio() const;
    int getTotalProcessedFlows() const { return totalProcessedFlows; }
};

#endif
