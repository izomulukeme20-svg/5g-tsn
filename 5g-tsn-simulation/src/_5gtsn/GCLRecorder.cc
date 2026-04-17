#include "GCLRecorder.h"
#include <iomanip>
#include <ctime>
#include <sstream>

Define_Module(GCLRecorder);

GCLRecorder* GCLRecorder::instance = nullptr;
std::mutex GCLRecorder::fileMutex;

GCLRecorder::GCLRecorder() : recordCounter(0) {
    instance = this;
}

GCLRecorder::~GCLRecorder() {
    flushRecords();
    if (outputFile.is_open()) {
        outputFile.close();
    }
    if (instance == this) {
        instance = nullptr;
    }
}

GCLRecorder* GCLRecorder::getInstance() {
    return instance;
}

std::string GCLRecorder::getCurrentTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm* tm_info = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}

void GCLRecorder::initialize() {
    outputPath = par("outputPath").stdstringValue();
    recordInterval = par("recordInterval");
    
    if (outputPath.empty()) {
        outputPath = "gcl_changes.csv";
    }
    
    std::lock_guard<std::mutex> lock(fileMutex);
    outputFile.open(outputPath, std::ios::out | std::ios::trunc);
    if (outputFile.is_open()) {
        outputFile << "timestamp,slot,port,queue,oldState,newState,source,record_time" << std::endl;
        EV << "GCLRecorder initialized, output: " << outputPath << std::endl;
    } else {
        EV << "GCLRecorder failed to open file: " << outputPath << std::endl;
    }
}

void GCLRecorder::handleMessage(cMessage* msg) {
    delete msg;
}

void GCLRecorder::recordGCLChange(int slot, int port, int queue, int oldState, int newState, const std::string& source) {
    std::lock_guard<std::mutex> lock(fileMutex);
    
    if (!outputFile.is_open()) {
        return;
    }
    
    double timestamp = simTime().dbl();
    std::string recordTime = getCurrentTimeString();
    
    outputFile << std::fixed << std::setprecision(6)
               << timestamp << ","
               << slot << ","
               << port << ","
               << queue << ","
               << oldState << ","
               << newState << ","
               << source << ","
               << recordTime << std::endl;
    
    outputFile.flush();
    recordCounter++;
}

void GCLRecorder::flushRecords() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (outputFile.is_open()) {
        outputFile.flush();
    }
}

void GCLRecorder::finish() {
    flushRecords();
    EV << "GCLRecorder recorded " << recordCounter << " GCL changes" << std::endl;
}
