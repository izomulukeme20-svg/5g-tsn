//
// OAFSModule.h - Online Adaptive Flow Scheduling for TSN
// 实时在线闭环动态调整算法
//

#ifndef OAFSMODULE_H
#define OAFSMODULE_H

#include <omnetpp.h>
#include <vector>
#include <deque>
#include <map>
#include <queue>
#include <algorithm>
#include <limits>
#include <random>
#include "../messages/FlowPacket_m.h"

using namespace omnetpp;

// 流状态
enum FlowState {
    FLOW_PENDING,
    FLOW_SCHEDULED,
    FLOW_TRANSMITTING,
    FLOW_COMPLETED,
    FLOW_TIMEOUT
};

// 在线流信息
struct OnlineFlow {
    int id;
    int src;
    int dst;
    int size;
    int pcp;
    double pdb;
    double arrivalTime;
    double deadline;
    double actualDelay;
    int hopCount;
    FlowState state;
    
    // 调度信息
    int assignedSlot;
    int assignedQueue;
    int assignedSwitch;
    int assignedPort;
    double scheduledTime;
    
    // 路径信息
    std::vector<int> path;
    std::vector<int> slotSequence;
    
    // 性能追踪
    double currentDelay;
    bool isSatisfied;
    
    OnlineFlow() : id(-1), src(-1), dst(-1), size(0), pcp(4), pdb(0.004),
                   arrivalTime(0), deadline(0), actualDelay(0), hopCount(0),
                   state(FLOW_PENDING), assignedSlot(-1), assignedQueue(-1),
                   assignedSwitch(-1), assignedPort(-1), scheduledTime(0),
                   currentDelay(0), isSatisfied(false) {}
};

// 时间槽状态
struct OnlineTimeSlot {
    int id;
    int switchId;
    int portId;
    int queueId;
    double startTime;
    double endTime;
    int capacity;
    int remainingCapacity;
    std::vector<int> flowIds;
    bool isOccupied;
    double utilization;
    
    // 动态调整
    double predictedLoad;
    double actualLoad;
    double adjustmentFactor;
    
    OnlineTimeSlot() : id(-1), switchId(-1), portId(-1), queueId(-1),
                       startTime(0), endTime(0), capacity(1500), remainingCapacity(1500),
                       isOccupied(false), utilization(0), predictedLoad(0),
                       actualLoad(0), adjustmentFactor(1.0) {}
};

// 在线调度决策
struct OnlineSchedulingDecision {
    int flowId;
    std::vector<int> slotSequence;
    std::vector<int> queueSequence;
    std::vector<int> switchSequence;
    double expectedDelay;
    double confidence;
    simtime_t decisionTime;
    
    // 在线调整
    bool needsAdjustment;
    double adjustmentReason;
    
    // MFM/MTM信息
    bool isMFM;
    bool isMTM;
    std::vector<int> mergedFlowIds;
    int numMergedSlots;
    
    OnlineSchedulingDecision() : flowId(-1), expectedDelay(0), confidence(0),
                                  needsAdjustment(false), adjustmentReason(0),
                                  isMFM(false), isMTM(false), numMergedSlots(1) {}
};

// 网络监控数据
struct NetworkMonitorData {
    double currentTime;
    int currentTimeSlot;
    std::vector<double> queueUtilization;
    std::vector<double> slotUtilization;
    double avgDelay;
    double maxDelay;
    double satisfactionRatio;
    double throughput;
    
    // 趋势数据
    std::deque<double> delayTrend;
    std::deque<double> satisfactionTrend;
    
    NetworkMonitorData() : currentTime(0), currentTimeSlot(0), avgDelay(0),
                            maxDelay(0), satisfactionRatio(0), throughput(0) {}
};

// OAFS核心算法类
class OAFSCore {
private:
    // 网络参数
    int numSwitches;
    int numPortsPerSwitch;
    int numQueues;
    int numTimeSlots;
    double timeSlotSize;
    double hyperPeriod;
    double linkRate;
    
    // 在线状态
    std::map<int, OnlineFlow> activeFlows;
    std::map<int, OnlineFlow> completedFlows;
    std::queue<OnlineFlow> pendingQueue;
    
    // 时间槽管理
    std::vector<std::vector<std::vector<OnlineTimeSlot>>> timeSlots; // [switch][port][slot]
    
    // 监控数据
    NetworkMonitorData monitorData;
    std::deque<NetworkMonitorData> monitorHistory;
    
    // 在线调整参数
    double adaptationRate;
    double predictionWindow;
    double adjustmentThreshold;
    
    // 统计
    int flowCounter;
    double totalDelay;
    int satisfiedCount;
    int totalCount;
    
    // 随机数生成器
    std::mt19937 rng;
    
    // 内部方法
    void initializeTimeSlots();
    void updateTimeSlotUtilization();
    double predictSlotLoad(int switchId, int portId, int slotId);
    int findOptimalSlot(const OnlineFlow& flow, int switchId, int portId);
    bool canMergeFlows(const OnlineTimeSlot& slot, const OnlineFlow& flow);
    void adjustSchedule(int flowId);
    double calculateExpectedDelay(const OnlineFlow& flow, const OnlineSchedulingDecision& decision);
    
    // MFM (Multi-Flow Merging) 多流合并
    bool canApplyMFM(const OnlineFlow& flow);
    std::vector<int> findMergeableFlows(const OnlineFlow& flow, int targetSlot);
    OnlineSchedulingDecision applyMFM(OnlineFlow& flow, const std::vector<int>& mergeableFlows, int targetSlot);
    bool checkMFMConstraints(const OnlineFlow& flow1, const OnlineFlow& flow2);
    double calculateMFMEfficiency(const std::vector<int>& flowIds);
    
    // MTM (Multi-Time-slot Merging) 多时隙合并
    bool canApplyMTM(const OnlineFlow& flow);
    std::vector<int> findConsecutiveSlots(int startSlot, int numSlots, int switchId, int portId);
    OnlineSchedulingDecision applyMTM(OnlineFlow& flow, const std::vector<int>& slots);
    bool checkMTMConstraints(const OnlineFlow& flow, const std::vector<int>& slots);
    double calculateMTMEfficiency(const OnlineFlow& flow, int numSlots);
    
    // Algorithm 1: 时隙路径映射
    OnlineSchedulingDecision slotPathMapping(const OnlineFlow& flow);
    int selectBestSlot(const OnlineFlow& flow, const std::vector<int>& candidateSlots);
    double calculateSlotScore(int slotId, const OnlineFlow& flow);
    void updateSlotResources(int slotId, int switchId, int portId, int flowSize);
    
public:
    OAFSCore();
    ~OAFSCore();
    
    // 初始化
    void initialize(int numSwitches, int numPortsPerSwitch, int numQueues,
                   int numTimeSlots, double timeSlotSize, double hyperPeriod, double linkRate);
    
    // 在线调度接口
    OnlineSchedulingDecision scheduleFlow(OnlineFlow& flow);
    void updateFlowStatus(int flowId, FlowState state, double currentDelay = 0);
    void receiveFeedback(int flowId, double actualDelay, bool satisfied);
    
    // 在线调整接口
    void adjustTimeSlots();
    void rebalanceFlows();
    void handleOverload(int switchId, int portId);
    
    // 监控接口
    void updateMonitorData(const NetworkMonitorData& data);
    NetworkMonitorData getMonitorData() const { return monitorData; }
    
    // 统计接口
    double getSatisfactionRatio() const;
    double getAverageDelay() const;
    double getThroughput() const;
    int getActiveFlowCount() const { return activeFlows.size(); }
    int getPendingFlowCount() const { return pendingQueue.size(); }
    
    // 调试接口
    void printStatistics();
    void printTimeSlotStatus(int switchId, int portId);
};

//
// OAFS OMNeT++ Module - 实时在线闭环模块
//
class OAFSModule : public cSimpleModule
{
private:
    OAFSCore* oafsCore;
    
    // 定时器
    cMessage* schedulingTimer;
    cMessage* adjustmentTimer;
    cMessage* monitorTimer;
    
    // 参数
    int numTimeSlots;
    int numQueues;
    int numSwitches;
    double timeSlotSize;
    double hyperPeriod;
    double linkRate;
    double schedulingInterval;
    double adjustmentInterval;
    double monitorInterval;
    bool enableAdaptive;
    
    // 状态
    NetworkMonitorData currentMonitorData;
    std::map<int, FlowPacket*> pendingFlows;
    int flowCounter;
    
    // 统计信号
    simsignal_t flowScheduledSignal;
    simsignal_t flowCompletedSignal;
    simsignal_t satisfactionSignal;
    simsignal_t delaySignal;
    simsignal_t throughputSignal;
    simsignal_t adjustmentSignal;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    // 处理不同类型的事件
    void handleSchedulingTimer();
    void handleAdjustmentTimer();
    void handleMonitorTimer();
    void handleFlowPacket(FlowPacket* pkt);
    void handleFeedback(cMessage* msg);
    
    // 辅助方法
    void collectMonitorData();
    void sendSchedulingDecision(const OnlineSchedulingDecision& decision);
    void processOnlineAdjustment();
    
public:
    OAFSModule();
    virtual ~OAFSModule();
    
    // 外部接口
    void submitFlow(FlowPacket* flow);
    void reportFeedback(int flowId, double delay, bool satisfied);
    void triggerAdjustment();
};

Define_Module(OAFSModule);

#endif
