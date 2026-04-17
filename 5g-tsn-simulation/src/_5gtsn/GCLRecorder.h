#ifndef GCLRECORDER_H_
#define GCLRECORDER_H_

#include <omnetpp.h>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>

using namespace omnetpp;

struct GCLChangeEntry {
    double timestamp;
    int slot;
    int port;
    int queue;
    int oldState;
    int newState;
    std::string source;
    std::string recordTime;
};

class GCLRecorder : public cSimpleModule {
private:
    std::string outputPath;
    int recordInterval;
    std::ofstream outputFile;
    int recordCounter;
    std::vector<GCLChangeEntry> pendingRecords;
    static std::mutex fileMutex;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
public:
    GCLRecorder();
    virtual ~GCLRecorder();
    
    void recordGCLChange(int slot, int port, int queue, int oldState, int newState, const std::string& source);
    void flushRecords();
    static GCLRecorder* getInstance();
    
private:
    static GCLRecorder* instance;
    std::string getCurrentTimeString();
};

#endif
