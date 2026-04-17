#ifndef GNB_H
#define GNB_H

#include <omnetpp.h>
#include <vector>
#include <map>
#include <queue>
#include <random>

using namespace omnetpp;

class FlowPacket;

struct UeInfo {
    int ueId;
    double lastContactTime;
    int numPackets;
    int bufferStatus;
};

struct WirelessChannel {
    int channelId;
    double frequency;
    double bandwidth;
    double txPower;
    double noiseFigure;
    double pathLossExponent;
    double shadowingStd;
    
    double calculatePathLoss(double distance) const;
    double calculateSNR(double distance, double txPower) const;
    double calculateDelay(double distance, int packetSize) const;
    double calculateJakesFading(double time, double dopplerShift) const;
};

struct RBSchedulingDecision {
    int ueId;
    int drbId;
    int numRBs;
    int mcs;
    double startTime;
    double duration;
};

struct HARQProcess {
    int processId;
    bool active;
    int retransmissionCount;
    int maxRetransmissions;
    double lastTransmissionTime;
};

class GNB : public cSimpleModule {
private:
    int gnbId;
    int numUes;
    double frequency;
    double bandwidth;
    double txPower;
    double antennaGain;
    double noiseFigure;
    
    std::vector<UeInfo> connectedUes;
    std::map<int, WirelessChannel> channels;
    std::map<int, std::queue<cMessage*>> uplinkQueues;
    std::map<int, std::queue<cMessage*>> downlinkQueues;
    
    cMessage* processingTimer;
    cMessage* schedulingTimer;
    double processingInterval;
    double schedulingInterval;
    
    double jitterMean;
    double jitterStd;
    double ueVelocity;
    double dopplerShift;
    std::mt19937 rng;
    std::normal_distribution<double> jitterDist;
    
    simsignal_t uplinkDelaySignal;
    simsignal_t downlinkDelaySignal;
    simsignal_t channelQualitySignal;
    simsignal_t rbUtilizationSignal;
    
    int totalUplinkPackets;
    int totalDownlinkPackets;
    double totalUplinkDelay;
    double totalDownlinkDelay;
    
    int totalResourceBlocks;
    int usedResourceBlocks;
    
    std::map<int, std::vector<HARQProcess>> harqProcesses;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void processUplinkPacket(cMessage* msg);
    void processDownlinkPacket(cMessage* msg);
    void forwardToCore(cMessage* msg);
    void forwardToUe(cMessage* msg, int ueId);
    
    double calculateWirelessDelay(int ueId, int packetSize);
    double addJitter(double baseDelay);
    
    void performRBScheduling();
    RBSchedulingDecision selectUeForScheduling();
    int selectMCS(double snr);
    void manageHARQ(int ueId, int drbId, bool success);
    
public:
    GNB();
    virtual ~GNB();
    
    void connectUe(int ueId);
    void disconnectUe(int ueId);
    
    int getNumConnectedUes() const;
    double getAverageUplinkDelay() const;
    double getAverageDownlinkDelay() const;
    double getRBUtilization() const;
    
    void printStatistics() const;
};

#endif
