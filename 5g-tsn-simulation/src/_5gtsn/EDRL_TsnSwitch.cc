#include "EDRL_TsnSwitch.h"
#include "FlowPacket_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/UserPriorityTag_m.h"
#include "inet/common/ProtocolTag_m.h"
#include <fstream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <ctime>

Define_Module(EDRL_TsnSwitch);

EDRL_TsnSwitch::EDRL_TsnSwitch() : edrlAgent(nullptr), oafsCore(nullptr),
                                     trainingTimer(nullptr), trainingCounter(0),
                                     checkpointInterval(100), loadModelOnStart(false),
                                     totalDelay(0), satisfiedPackets(0), gclRecordInterval(100),
                                     gclRecordCounter(0) {}

EDRL_TsnSwitch::~EDRL_TsnSwitch() {
    cancelAndDelete(trainingTimer);
    if (edrlAgent) delete edrlAgent;
    if (oafsCore) delete oafsCore;
    if (gclChangeLog.is_open()) gclChangeLog.close();
}

void EDRL_TsnSwitch::initialize(int stage) {
    TSNSwitch::initialize(stage);
    
    if (stage == INITSTAGE_LINK_LAYER) {
        bridgeId = par("bridgeId");
        enableEDRL = par("enableEDRL");
        enableOAFS = par("enableOAFS");
        enableTimeAwareShaper = par("enableTimeAwareShaper");
        enableGCLRecording = par("enableGCLRecording");
        trainingInterval = par("trainingInterval");
        modelSavePath = par("modelSavePath").stdstringValue();
        loadModelOnStart = par("loadModelOnStart");
        modelLoadPath = par("modelLoadPath").stdstringValue();
        
        if (enableEDRL) {
            edrlAgent = new TorchEDRLAgent();
            edrlAgent->initialize(numTimeSlots, 8, 1, timeSlotSize, hyperPeriod);
            EV << "=== EDRL_TsnSwitch created new TorchEDRLAgent ===" << std::endl;
            
            if (loadModelOnStart && edrlAgent) {
                edrlAgent->loadModel(modelLoadPath);
                EV << "=== Model loaded from previous training ===" << std::endl;
                EV << "  Path: " << modelLoadPath << std::endl;
            }
        }
        
        if (enableOAFS) {
            oafsCore = new OAFSCore();
            EV << "=== OAFS module initialized ===" << std::endl;
        }
        
        trainingTimer = new cMessage("TrainingTimer");
        scheduleAt(simTime() + trainingInterval, trainingTimer);
        
        satisfactionSignal = registerSignal("satisfaction");
        trainingStepSignal = registerSignal("trainingStep");
        gclChangeSignal = registerSignal("gclChange");
        
        gclChangeLog.open("gcl_changes.csv");
        gclChangeLog << "timestamp,slot,port,queue,oldState,newState,source,record_time\n";
        
        int numPorts = interfaceTable->getNumInterfaces();
        lastGCL.resize(numTimeSlots, std::vector<std::vector<int>>(numPorts, std::vector<int>(numQueues, 0)));
    }
}

void EDRL_TsnSwitch::handleLowerPacket(Packet *packet) {
    auto macAddressInd = packet->getTag<MacAddressInd>();
    auto sourceAddress = macAddressInd->getSrcAddress();
    auto destinationAddress = macAddressInd->getDestAddress();
    auto interfaceInd = packet->getTag<InterfaceInd>();
    int incomingInterfaceId = interfaceInd->getInterfaceId();
    auto incomingInterface = interfaceTable->getInterfaceById(incomingInterfaceId);
    
    int inPort = incomingInterface->getInterfaceId();
    
    int outPort = -1;
    int numPorts = interfaceTable->getNumInterfaces();
    for (int p = 0; p < numPorts; p++) {
        auto intf = interfaceTable->getInterface(p);
        if (intf && !intf->isLoopback() && intf->isBroadcast() && intf != incomingInterface) {
            outPort = p;
            break;
        }
    }
    
    if (outPort < 0) {
        TSNSwitch::handleLowerPacket(packet);
        return;
    }
    
    int queueId = 0;
    int pcp = 0;
    auto userPriorityInd = packet->findTag<UserPriorityInd>();
    if (userPriorityInd != nullptr) {
        pcp = userPriorityInd->getUserPriority();
        queueId = pcp % numQueues;
    }
    
    double arrivalTime = simTime().dbl();
    int size = packet->getByteLength();
    int flowId = genericFlowIdCounter++;
    
    std::string flowType = "Ethernet";
    std::string decisionSource = "TSN";
    
    if (enableEDRL && edrlAgent) {
        NetworkState netState;
        netState.currentTimeSlot = getCurrentTimeSlot();
        netState.queueLengths.resize(numQueues);
        for (int q = 0; q < numQueues; q++) {
            netState.queueLengths[q] = queueLengths[outPort][q];
        }
        netState.avgDelay = getAverageDelay();
        
        SchedulingDecision decision = edrlAgent->makeDecision(netState, nullptr);
        if (decision.assignedQueue >= 0 && decision.assignedQueue < numQueues) {
            queueId = decision.assignedQueue;
            decisionSource = "EDRL";
        }
    }
    
    if (enableOAFS && oafsCore) {
        OnlineFlow flow;
        flow.id = flowId;
        flow.size = size;
        flow.pcp = pcp;
        flow.arrivalTime = arrivalTime;
        flow.deadline = arrivalTime + 0.004;
        
        OnlineSchedulingDecision decision = oafsCore->scheduleFlow(flow);
        if (!decision.queueSequence.empty()) {
            queueId = decision.queueSequence[0];
            decisionSource = "OAFS";
        }
    }
    
    if (enableTrafficRecording && (int)trafficRecords.size() < trafficRecordLimit) {
        TrafficFlowRecord record;
        record.timestamp = arrivalTime;
        record.flowId = flowId;
        record.packetSize = size;
        record.priority = pcp;
        record.pcp = pcp;
        record.assignedQueue = queueId;
        record.assignedSlot = getCurrentTimeSlot();
        record.inPort = inPort;
        record.outPort = outPort;
        record.arrivalTime = arrivalTime;
        record.departureTime = -1;
        record.delay = -1;
        record.deadline = arrivalTime + 0.004;
        record.satisfied = false;
        record.flowType = flowType;
        record.decisionSource = decisionSource;
        
        recordTrafficFlow(record);
        trafficRecords.push_back(record);
    }
    
    TSNSwitch::handleLowerPacket(packet);
}

void EDRL_TsnSwitch::finish() {
    TSNSwitch::finish();
    
    if (edrlAgent) {
        edrlAgent->saveModel(modelSavePath);
        EV << "=== EDRL model saved ===" << std::endl;
        EV << "  Path: " << modelSavePath << std::endl;
        EV << "  Training steps: " << trainingCounter << std::endl;
    }
    
    saveSummaryReport();
    saveGCLHistory();
}

void EDRL_TsnSwitch::handleTraining() {
    if (!enableEDRL || !edrlAgent) return;
    
    edrlAgent->train();
    trainingCounter++;
    
    if (trainingCounter % checkpointInterval == 0) {
        edrlAgent->saveModel(modelSavePath);
        EV << "=== EDRL checkpoint saved ===" << std::endl;
        EV << "  Training step: " << trainingCounter << std::endl;
    }
    
    emit(trainingStepSignal, trainingCounter);
}

double EDRL_TsnSwitch::calculateDelay(int flowId) {
    return totalDelay;
}

bool EDRL_TsnSwitch::checkDeadlineSatisfaction(int flowId) {
    return true;
}

void EDRL_TsnSwitch::recordTrainingStats() {
}

void EDRL_TsnSwitch::recordGCLChange(int slot, int port, int queue, int oldState, int newState, const std::string& source) {
    GCLChangeRecord record;
    record.timestamp = simTime().dbl();
    record.slotId = slot;
    record.portId = port;
    record.queueId = queue;
    record.oldState = oldState;
    record.newState = newState;
    record.source = source;
    
    gclChanges.push_back(record);
    
    if (gclChangeLog.is_open()) {
        std::time_t now = std::time(nullptr);
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        gclChangeLog << std::fixed << std::setprecision(6)
                     << record.timestamp << ","
                     << record.slotId << ","
                     << record.portId << ","
                     << record.queueId << ","
                     << record.oldState << ","
                     << record.newState << ","
                     << record.source << ","
                     << timeBuffer << "\n";
        gclChangeLog.flush();
    }
    
    emit(gclChangeSignal, gclChanges.size());
}

void EDRL_TsnSwitch::recordGCLSnapshot() {
    TSNSwitch::recordGCLSnapshot();
}

void EDRL_TsnSwitch::saveGCLHistory() {
    std::ofstream gclHistory("gcl_history.csv");
    gclHistory << "time_slot,port,queue,gate_state\n";
    
    int numPorts = interfaceTable->getNumInterfaces();
    for (int t = 0; t < numTimeSlots; t++) {
        for (int p = 0; p < numPorts; p++) {
            for (int q = 0; q < numQueues; q++) {
                gclHistory << t << "," << p << "," << q << "," << gcl[t][p][q] << "\n";
            }
        }
    }
    gclHistory.close();
}

void EDRL_TsnSwitch::saveSummaryReport() {
    std::ofstream summary("summary_report.txt");
    summary << "=== EDRL TSN Switch Summary Report ===\n";
    summary << "Simulation time: " << simTime() << "s\n";
    summary << "Switch ID: " << switchId << "\n";
    summary << "Bridge ID: " << bridgeId << "\n";
    summary << "Total processed packets: " << totalProcessedPackets << "\n";
    summary << "Total dropped packets: " << totalDroppedPackets << "\n";
    summary << "Average delay: " << getAverageDelay() * 1000 << " ms\n";
    summary << "Satisfaction ratio: " << getSatisfactionRatio() * 100 << "%\n";
    summary << "Training steps: " << trainingCounter << "\n";
    summary << "Traffic records: " << trafficRecords.size() << "\n";
    summary << "GCL changes: " << gclChanges.size() << "\n";
    summary.close();
}

double EDRL_TsnSwitch::getSatisfactionRatio() const {
    return totalProcessedPackets > 0 ? (double)satisfiedPackets / totalProcessedPackets : 1.0;
}

void EDRL_TsnSwitch::updateGCLFromEDRL(const std::vector<std::vector<int>>& newGCL) {
    loadGCL(newGCL);
}
