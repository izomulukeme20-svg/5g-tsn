#ifndef FLOWGENERATOR_H
#define FLOWGENERATOR_H

#include <omnetpp.h>
#include <vector>
#include <random>

using namespace omnetpp;

class FlowPacket;

enum FlowType {
    NETWORK_CONTROL = 0,
    ISOCHRONOUS = 1,
    CYCLIC = 2,
    VIDEO = 3
};

struct FlowConfig {
    FlowType type;
    int pcp;
    int qfi;
    int size;
    double period;
    double pdb;
    std::string name;
};

class FlowGenerator : public cSimpleModule {
private:
    int nodeId;
    int numDestinations;
    bool isActive;
    
    std::vector<FlowConfig> flowConfigs;
    std::map<int, cMessage*> flowTimers;
    std::map<int, int> flowCounters;
    
    std::mt19937 rng;
    std::uniform_int_distribution<int> destDist;
    std::normal_distribution<double> jitterDist;
    
    int flowIdCounter;
    
    simsignal_t flowGeneratedSignal;
    simsignal_t flowSizeSignal;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
public:
    FlowGenerator();
    virtual ~FlowGenerator();
    
    void addFlowConfig(FlowType type, int pcp, int qfi, int size, double period, double pdb, const std::string& name);
    void startGeneration();
    void stopGeneration();
    
    FlowPacket* createFlowPacket(const FlowConfig& config, int destination);
    void sendFlowPacket(cMessage* packet);
    
    int getNumGeneratedFlows() const;
    double getAverageFlowSize() const;
    
    void setNodeId(int id) { nodeId = id; }
    void setNumDestinations(int num) { numDestinations = num; destDist = std::uniform_int_distribution<int>(0, numDestinations - 1); }
};

#endif
