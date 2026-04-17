#ifndef EDRLCONTROLLER_H_
#define EDRLCONTROLLER_H_

#include <omnetpp.h>
#include "TorchEDRLAgent.h"

using namespace omnetpp;

class EDRLController : public cSimpleModule {
private:
    int switchId;
    bool enableTraining;
    double trainingInterval;
    std::string modelPath;
    bool loadModelOnStart;
    
    TorchEDRLAgent* edrlAgent;
    cMessage* trainingTimer;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
public:
    EDRLController();
    virtual ~EDRLController();
    
    TorchEDRLAgent* getEDRLAgent() { return edrlAgent; }
};

#endif
