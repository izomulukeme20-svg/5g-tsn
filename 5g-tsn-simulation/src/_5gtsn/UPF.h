#ifndef UPF_H
#define UPF_H

#include <omnetpp.h>
#include <vector>
#include <map>
#include <queue>

using namespace omnetpp;

class FlowPacket;

struct TunnelInfo {
    int tunnelId;
    int srcUeId;
    int dstGatewayId;
    double creationTime;
    int qfi;
};

struct TrafficStats {
    int totalPackets;
    int totalBytes;
    double totalDelay;
    double maxDelay;
};

class UPF : public cSimpleModule {
private:
    int upfId;
    int numUes;
    int numGateways;
    
    std::map<int, TunnelInfo> tunnels;
    std::map<int, std::queue<cMessage*>> uplinkQueues;
    std::map<int, std::queue<cMessage*>> downlinkQueues;
    
    cMessage* processingTimer;
    double processingInterval;
    
    simsignal_t packetForwardedSignal;
    simsignal_t tunnelCreatedSignal;
    simsignal_t trafficStatsSignal;
    
    std::map<int, TrafficStats> flowStats;
    
    int nextTunnelId;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void processUplinkPacket(cMessage* msg);
    void processDownlinkPacket(cMessage* msg);
    void forwardToGateway(cMessage* msg);
    void forwardToGNB(cMessage* msg);
    
    int createTunnel(int srcUeId, int dstGatewayId, int qfi);
    void updateTrafficStats(int qfi, int packetSize, double delay);
    
public:
    UPF();
    virtual ~UPF();
    
    int getTunnelCount() const;
    void printStatistics() const;
};

#endif
