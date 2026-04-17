#ifndef OAFSSCHEDULER_H_
#define OAFSSCHEDULER_H_

#include <omnetpp.h>
#include "OAFSModule.h"

using namespace omnetpp;

class OAFSScheduler : public cSimpleModule {
private:
    int numTimeSlots;
    int numQueues;
    double timeSlotSize;
    
    OAFSCore* oafsCore;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    
public:
    OAFSScheduler();
    virtual ~OAFSScheduler();
    
    int assignSlot(int flowId, int priority);
    OAFSCore* getOAFSCore() { return oafsCore; }
};

#endif
