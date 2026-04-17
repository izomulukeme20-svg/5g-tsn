#ifndef TSNBRIDGE_H_
#define TSNBRIDGE_H_

#include <omnetpp.h>
#include "TorchEDRLAgent.h"
#include "OAFSModule.h"
#include "HeuristicGreedyScheduler.h"
#include <vector>
#include <map>
#include <queue>
#include <deque>

using namespace omnetpp;

struct FlowInfo {
    int flowId;
    int srcAddr;
    int dstAddr;
    int pcp;
    int qfi;
    int size;
    double arrivalTime;
    double deadline;
    double pdb;
    int hopCount;
    int assignedQueue;
    int assignedSlot;
    bool isScheduled;
};

struct QueueState {
    int queueId;
    int backlogBytes;
    int packetCount;
    double lastDequeueTime;
    double avgDelay;
    std::deque<cMessage*> packetQueue;
};

struct TimeSlotState {
    int slotId;
    bool isActive;
    int assignedQueue;
    double utilization;
    int packetCount;
};

class TSNBridge : public cSimpleModule {
private:
    TorchEDRLAgent* edrlAgent;
    OAFSCore* oafsCore;
    HeuristicGreedyScheduler* greedyScheduler;
    
    int bridgeId;
    int numPorts;
    int numQueues;
    int numTimeSlots;
    int stateSize;
    double timeSlotSize;
    double hyperPeriod;
    double linkRate;
    
    bool enableEDRL;
    bool enableOAFS;
    bool enableTimeAwareShaper;
    bool enableGreedyScheduler;
    
    cMessage* gclUpdateTimer;
    cMessage* trainingTimer;
    cMessage* statsTimer;
    double gclUpdateInterval;
    double trainingInterval;
    double statsInterval;
    
    std::vector<QueueState> queues;
    std::vector<TimeSlotState> timeSlots;
    std::map<int, FlowInfo> activeFlows;
    std::map<std::pair<int, int>, int> flowToQueue;
    
    std::vector<std::deque<cMessage*>> outputQueues;
    std::vector<cMessage*> outputTimers;
    
    int nextFlowId;
    int currentSlot;
    double slotStartTime;
    
    int totalPacketsProcessed;
    int totalPacketsForwarded;
    int totalPacketsDropped;
    double totalDelay;
    int satisfiedPackets;
    
    int trainingCounter;
    int checkpointInterval;
    std::string modelSavePath;
    
    simsignal_t packetProcessedSignal;
    simsignal_t packetForwardedSignal;
    simsignal_t packetDroppedSignal;
    simsignal_t delaySignal;
    simsignal_t queueLengthSignal;
    simsignal_t satisfactionSignal;
    simsignal_t throughputSignal;
    simsignal_t slotUtilizationSignal;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void processPacket(cMessage* msg);
    void processControlMessage(cMessage* msg);
    
    void handleGCLUpdate();
    void handleTraining();
    void handleStatsUpdate();
    
    int classifyPacket(cMessage* msg);
    int selectQueue(int flowId, int pcp, double deadline);
    int selectTimeSlot(int flowId, int queueId, double deadline);
    
    void enqueuePacket(cMessage* msg, int queueId, int slotId);
    void dequeuePacket(int queueId);
    void forwardPacket(cMessage* msg, int outputPort);
    void sendFromOutputQueue(int port);
    
    void updateQueueState(int queueId);
    void updateTimeSlotState(int slotId);
    void updateNetworkState();
    
    double calculateDelay(int flowId);
    bool checkDeadlineSatisfaction(int flowId);
    
    void applyEDRLScheduling();
    void applyOAFSScheduling();
    
    void recordStatistics();
    
public:
    TSNBridge();
    virtual ~TSNBridge();
    
    double getAverageDelay() const;
    double getSatisfactionRatio() const;
    int getQueueLength(int queueId) const;
    double getSlotUtilization(int slotId) const;
};

#endif
