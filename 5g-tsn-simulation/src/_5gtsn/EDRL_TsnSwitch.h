#ifndef EDRL_TSNSWITCH_H_
#define EDRL_TSNSWITCH_H_

#include <omnetpp.h>
#include "TSNSwitch.h"
#include "TorchEDRLAgent.h"
#include "OAFSModule.h"
#include "GCLRecorder.h"
#include "EDRLController.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>

struct GCLChangeRecord {
    double timestamp;
    int slotId;
    int portId;
    int queueId;
    int oldState;
    int newState;
    std::string source;
};

class EDRL_TsnSwitch : public TSNSwitch
{
private:
    TorchEDRLAgent* edrlAgent;
    OAFSCore* oafsCore;
    
    int bridgeId;
    
    bool enableEDRL;
    bool enableOAFS;
    bool enableTimeAwareShaper;
    bool enableGCLRecording;
    
    cMessage* trainingTimer;
    double trainingInterval;
    
    int trainingCounter;
    int checkpointInterval;
    std::string modelSavePath;
    bool loadModelOnStart;
    std::string modelLoadPath;
    
    double totalDelay;
    int satisfiedPackets;
    std::map<int, double> flowArrivalTimes;
    std::map<int, double> flowDeadlines;
    
    std::vector<GCLChangeRecord> gclChanges;
    std::vector<std::vector<std::vector<int>>> lastGCL;
    int gclRecordInterval;
    int gclRecordCounter;
    std::ofstream gclChangeLog;
    
    simsignal_t satisfactionSignal;
    simsignal_t trainingStepSignal;
    simsignal_t gclChangeSignal;
    
protected:
    virtual void initialize(int stage) override;
    virtual void handleLowerPacket(Packet *packet) override;
    virtual void finish() override;
    
    void handleTraining();
    
    double calculateDelay(int flowId);
    bool checkDeadlineSatisfaction(int flowId);
    void recordTrainingStats();
    
    void recordGCLChange(int slot, int port, int queue, int oldState, int newState, const std::string& source);
    void recordGCLSnapshot();
    void saveGCLHistory();
    void saveSummaryReport();
    
public:
    EDRL_TsnSwitch();
    virtual ~EDRL_TsnSwitch();
    
    double getSatisfactionRatio() const;
    
    void updateGCLFromEDRL(const std::vector<std::vector<int>>& newGCL);
    const std::vector<GCLChangeRecord>& getGCLChanges() const { return gclChanges; }
};

#endif