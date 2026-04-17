#ifndef GDEVICE_H
#define GDEVICE_H

#include <omnetpp.h>
#include <vector>
#include <random>
#include <map>
#include <queue>
#include "FlowGenerator.h"

using namespace omnetpp;

struct QoSRule {
    int qfi;
    int srcMac;
    int dstMac;
    int ethertype;
    int vlanId;
    int priority;
};

struct DRBMapping {
    int qfi;
    int drbId;
};

struct AccessLayerQueue {
    int drbId;
    std::queue<cMessage*> packets;
    int bufferSize;
    int maxBufferSize;
};

class GDevice : public cSimpleModule {
private:
    int deviceId;
    int gnbId;
    int numFlowTypes;
    bool isActive;
    
    FlowGenerator* flowGenerator;
    
    std::mt19937 rng;
    std::uniform_int_distribution<int> flowTypeDist;
    
    std::vector<double> flowIntervals;
    std::vector<int> flowSizes;
    std::vector<int> flowPriorities;
    std::vector<double> flowDelayBounds;
    
    cMessage* activityTimer;
    double activityInterval;
    
    simsignal_t packetSentSignal;
    simsignal_t packetReceivedSignal;
    simsignal_t qosClassificationSignal;
    simsignal_t bufferStatusSignal;
    
    int totalSentPackets;
    int totalReceivedPackets;
    
    std::vector<QoSRule> qosRules;
    std::vector<DRBMapping> drbMappings;
    std::map<int, AccessLayerQueue> accessLayerQueues;
    
    cMessage* bsrTimer;
    double bsrInterval;
    
    int nextQfi;
    int nextDrbId;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void generateTraffic();
    void sendPacket(cMessage* msg);
    void receivePacket(cMessage* msg);
    
    int classifyPacket(cMessage* msg);
    int mapQosFlowToDrb(int qfi);
    void enqueueToAccessLayer(cMessage* msg, int drbId);
    void sendBufferStatusReport();
    void initializeQosRules();
    void initializeDrbMappings();
    void initializeAccessLayerQueues();
    
public:
    GDevice();
    virtual ~GDevice();
    
    void setFlowConfig(FlowType type, int size, int priority, double interval, double delayBound);
    void startTraffic();
    void stopTraffic();
    
    int getTotalSentPackets() const { return totalSentPackets; }
    int getTotalReceivedPackets() const { return totalReceivedPackets; }
    double getThroughput() const;
    
    void addQoSRule(const QoSRule& rule);
    void addDrbMapping(const DRBMapping& mapping);
};

#endif
