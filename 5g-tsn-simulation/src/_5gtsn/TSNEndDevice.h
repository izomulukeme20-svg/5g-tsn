#ifndef TSNENDDEVICE_H
#define TSNENDDEVICE_H

#include <omnetpp.h>
#include <vector>
#include <map>

using namespace omnetpp;

class TSNEndDevice : public cSimpleModule {
private:
    int deviceId;
    bool isReceiver;
    
    std::map<int, double> flowDelays;
    std::map<int, double> flowStartTimes;
    
    int totalReceivedPackets;
    int totalSentPackets;
    double totalDelay;
    int satisfiedFlows;
    int totalFlows;
    
    simsignal_t endToEndDelaySignal;
    simsignal_t satisfactionSignal;
    simsignal_t throughputSignal;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void processReceivedPacket(cMessage* msg);
    void sendResponse(cMessage* msg);
    
public:
    TSNEndDevice();
    virtual ~TSNEndDevice();
    
    double getAverageDelay() const;
    double getSatisfactionRatio() const;
    double getThroughput() const;
    
    void printStatistics() const;
};

#endif
