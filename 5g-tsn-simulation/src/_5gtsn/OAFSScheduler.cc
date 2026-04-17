#include "OAFSScheduler.h"
#include "OAFSModule.h"
#include "../messages/FlowPacket_m.h"

Define_Module(OAFSScheduler);

OAFSScheduler::OAFSScheduler() : oafsCore(nullptr) {}

OAFSScheduler::~OAFSScheduler() {
    delete oafsCore;
}

void OAFSScheduler::initialize() {
    numTimeSlots = par("numTimeSlots");
    numQueues = par("numQueues");
    timeSlotSize = par("timeSlotSize");
    
    oafsCore = new OAFSCore();
    oafsCore->initialize(3, 4, numQueues, numTimeSlots, timeSlotSize, 0.016, 100e6);
    
    EV << "OAFSScheduler initialized with " << numTimeSlots << " time slots" << std::endl;
}

void OAFSScheduler::handleMessage(cMessage* msg) {
    FlowPacket* flowPkt = dynamic_cast<FlowPacket*>(msg);
    
    if (flowPkt && oafsCore) {
        OnlineFlow flow;
        flow.id = flowPkt->getFlowId();
        flow.src = flowPkt->getSrcNode();
        flow.dst = flowPkt->getDstNode();
        flow.size = flowPkt->getSize();
        flow.pcp = flowPkt->getPcp();
        flow.pdb = flowPkt->getPdb();
        flow.deadline = flowPkt->getDeadline();
        flow.arrivalTime = simTime().dbl();
        
        OnlineSchedulingDecision decision = oafsCore->scheduleFlow(flow);
        
        if (!decision.slotSequence.empty()) {
            flowPkt->setAssignedSlot(decision.slotSequence[0]);
        }
        if (!decision.queueSequence.empty()) {
            flowPkt->setAssignedQueue(decision.queueSequence[0]);
        }
        
        EV << "OAFSScheduler assigned slot sequence size " << decision.slotSequence.size() 
           << " for flow " << flowPkt->getFlowId() 
           << " expected delay " << decision.expectedDelay * 1000 << " ms" << std::endl;
    }
    
    send(msg, "scheduling$o");
}

int OAFSScheduler::assignSlot(int flowId, int priority) {
    if (oafsCore) {
        OnlineFlow flow;
        flow.id = flowId;
        flow.pcp = priority;
        flow.arrivalTime = simTime().dbl();
        
        OnlineSchedulingDecision decision = oafsCore->scheduleFlow(flow);
        if (!decision.slotSequence.empty()) {
            return decision.slotSequence[0];
        }
    }
    return -1;
}
