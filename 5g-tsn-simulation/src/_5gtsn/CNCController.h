//
// CNCController.h - Centralized Network Configuration Controller for TSN
// 论文: 集中式GCL配置管理和分发
//

#ifndef CNCCONTROLLER_H_
#define CNCCONTROLLER_H_

#include <omnetpp.h>
#include <vector>
#include <map>
#include <deque>
#include "TorchEDRLAgent.h"
#include "HeuristicGreedyScheduler.h"

using namespace omnetpp;

// 网络配置
struct NetworkConfiguration {
    int configId;
    double timestamp;
    GCLMatrix gcl;
    std::map<int, int> flowToQueue;
    std::map<int, int> flowToSlot;
    double expectedMaxDelay;
    double expectedAvgDelay;
    
    NetworkConfiguration() : configId(0), timestamp(0), 
                               expectedMaxDelay(0), expectedAvgDelay(0) {}
};

// 网络状态报告
struct NetworkStatusReport {
    double timestamp;
    int reportId;
    
    // 延迟统计
    double avgDelay;
    double maxDelay;
    double minDelay;
    double jitter;
    
    // 满足率统计
    double satisfactionRatio;
    int satisfiedFlows;
    int totalFlows;
    
    // 吞吐量统计
    double throughput;
    double utilization;
    
    // 队列状态
    std::vector<int> queueLengths;
    std::vector<double> queueDelays;
    
    // 时隙状态
    std::vector<double> slotUtilizations;
    
    NetworkStatusReport() : timestamp(0), reportId(0), avgDelay(0), maxDelay(0),
                             minDelay(0), jitter(0), satisfactionRatio(0),
                             satisfiedFlows(0), totalFlows(0), throughput(0),
                             utilization(0) {}
};

// CNC配置中心模块
class CNCController : public cSimpleModule {
private:
    // EDRL Agent引用
    TorchEDRLAgent* edrlAgent;
    
    // 启发式贪心调度器
    HeuristicGreedyScheduler* greedyScheduler;
    
    // 调度模式
    bool enableGreedyScheduler;
    
    // 配置管理
    std::deque<NetworkConfiguration> configHistory;
    NetworkConfiguration currentConfig;
    int configVersion;
    
    // 状态监控
    std::deque<NetworkStatusReport> statusHistory;
    NetworkStatusReport latestStatus;
    
    // 定时器
    cMessage* configUpdateTimer;
    cMessage* statusCollectTimer;
    cMessage* optimizationTimer;
    
    // 参数
    double configUpdateInterval;
    double statusCollectInterval;
    double optimizationInterval;
    int historySize;
    
    // 网络参数
    int numSwitches;
    int numQueues;
    int numTimeSlots;
    double timeSlotSize;
    double hyperPeriod;
    
    // 优化目标
    double targetSatisfaction;
    double targetMaxDelay;
    
    // 统计信号
    simsignal_t configUpdateSignal;
    simsignal_t satisfactionSignal;
    simsignal_t avgDelaySignal;
    simsignal_t throughputSignal;
    
    // 内部方法
    void generateNewConfiguration();
    void distributeConfiguration();
    void analyzeStatusReports();
    void optimizeConfiguration();
    bool checkConstraints(const NetworkConfiguration& config);
    double evaluateConfiguration(const NetworkConfiguration& config);
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    void handleConfigUpdate();
    void handleStatusCollect();
    void handleOptimization();
    
public:
    CNCController();
    virtual ~CNCController();
    
    // 外部接口
    void receiveStatusReport(const NetworkStatusReport& report);
    void triggerReconfiguration();
    const NetworkConfiguration& getCurrentConfig() const { return currentConfig; }
    int getConfigVersion() const { return configVersion; }
};

Define_Module(CNCController);

#endif
