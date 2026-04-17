//
// CNCController.cc - Centralized Network Configuration Controller for TSN
// 实现集中式GCL配置管理和分发
//

#include "CNCController.h"

Define_Module(CNCController);

CNCController::CNCController()
    : edrlAgent(nullptr), greedyScheduler(nullptr), enableGreedyScheduler(true),
      configVersion(0), configUpdateTimer(nullptr),
      statusCollectTimer(nullptr), optimizationTimer(nullptr),
      configUpdateInterval(0.01), statusCollectInterval(0.1),
      optimizationInterval(1.0), historySize(100),
      numSwitches(2), numQueues(8), numTimeSlots(1024),
      timeSlotSize(0.000015625), hyperPeriod(0.016),
      targetSatisfaction(0.95), targetMaxDelay(0.01) {}

CNCController::~CNCController() {
    delete edrlAgent;
    delete greedyScheduler;
    cancelAndDelete(configUpdateTimer);
    cancelAndDelete(statusCollectTimer);
    cancelAndDelete(optimizationTimer);
}

void CNCController::initialize() {
    // 获取参数
    numSwitches = par("numSwitches");
    numQueues = par("numQueues");
    numTimeSlots = par("numTimeSlots");
    timeSlotSize = par("timeSlotSize");
    hyperPeriod = par("hyperPeriod");
    configUpdateInterval = par("configUpdateInterval");
    statusCollectInterval = par("statusCollectInterval");
    optimizationInterval = par("optimizationInterval");
    targetSatisfaction = par("targetSatisfaction");
    targetMaxDelay = par("targetMaxDelay");
    enableGreedyScheduler = par("enableGreedyScheduler");
    
    // 初始化EDRL Agent
    edrlAgent = new TorchEDRLAgent();
    edrlAgent->initialize(numTimeSlots, numQueues, numSwitches, timeSlotSize, hyperPeriod);
    
    // 初始化启发式贪心调度器
    if (enableGreedyScheduler) {
        greedyScheduler = new HeuristicGreedyScheduler();
        greedyScheduler->initialize(hyperPeriod * 1e6, numQueues); // 转换为微秒
    }
    
    // 初始化配置
    currentConfig.configId = 0;
    currentConfig.timestamp = 0;
    currentConfig.gcl = GCLMatrix(numTimeSlots, numQueues);
    
    // 创建定时器
    configUpdateTimer = new cMessage("ConfigUpdateTimer");
    statusCollectTimer = new cMessage("StatusCollectTimer");
    optimizationTimer = new cMessage("OptimizationTimer");
    
    // 调度定时器
    scheduleAt(simTime() + configUpdateInterval, configUpdateTimer);
    scheduleAt(simTime() + statusCollectInterval, statusCollectTimer);
    scheduleAt(simTime() + optimizationInterval, optimizationTimer);
    
    // 注册信号
    configUpdateSignal = registerSignal("configUpdate");
    satisfactionSignal = registerSignal("satisfaction");
    avgDelaySignal = registerSignal("avgDelay");
    throughputSignal = registerSignal("throughput");
    
    EV << "CNC Controller initialized" << std::endl;
}

void CNCController::handleMessage(cMessage* msg) {
    if (msg == configUpdateTimer) {
        handleConfigUpdate();
        scheduleAt(simTime() + configUpdateInterval, configUpdateTimer);
    } else if (msg == statusCollectTimer) {
        handleStatusCollect();
        scheduleAt(simTime() + statusCollectInterval, statusCollectTimer);
    } else if (msg == optimizationTimer) {
        handleOptimization();
        scheduleAt(simTime() + optimizationInterval, optimizationTimer);
    } else {
        delete msg;
    }
}

void CNCController::handleConfigUpdate() {
    generateNewConfiguration();
    distributeConfiguration();
    emit(configUpdateSignal, configVersion);
}

void CNCController::handleStatusCollect() {
    analyzeStatusReports();
    
    emit(satisfactionSignal, latestStatus.satisfactionRatio);
    emit(avgDelaySignal, latestStatus.avgDelay);
    emit(throughputSignal, latestStatus.throughput);
}

void CNCController::handleOptimization() {
    if (latestStatus.satisfactionRatio < targetSatisfaction ||
        latestStatus.maxDelay > targetMaxDelay) {
        optimizeConfiguration();
        triggerReconfiguration();
    }
}

void CNCController::generateNewConfiguration() {
    NetworkConfiguration newConfig;
    newConfig.configId = ++configVersion;
    newConfig.timestamp = simTime().dbl();
    
    if (enableGreedyScheduler && greedyScheduler) {
        // 使用启发式贪心调度算法生成GCL
        EV << "========================================" << endl;
        EV << "  使用启发式贪心调度算法生成GCL  " << endl;
        EV << "  Heuristic Greedy Scheduler Active!   " << endl;
        EV << "========================================" << endl;
        
        // 演示：使用3流冲突场景作为示例
        greedyScheduler->demo3FlowScheduling();
        
        // 将启发式调度器的GCL转换为GCLMatrix格式
        const GCLConfiguration& gclConfig = greedyScheduler->getGCLConfiguration();
        
        // 填充GCL矩阵
        for (int s = 0; s < numTimeSlots; s++) {
            for (int q = 0; q < numQueues; q++) {
                newConfig.gcl.setGate(s, q, 0);
            }
        }
        
        // 根据GCL指令设置对应的时隙
        double slotDuration = timeSlotSize * 1e6; // 转换为微秒
        for (const auto& instr : gclConfig.instructions) {
            int startSlot = (int)(instr.startTime / slotDuration);
            int endSlot = (int)((instr.startTime + instr.duration) / slotDuration);
            
            for (int s = startSlot; s <= endSlot && s < numTimeSlots; s++) {
                for (int q = 0; q < numQueues; q++) {
                    uint8_t mask = (uint8_t)(1 << (7 - q));
                    if ((instr.gateStateMask & mask) != 0) {
                        newConfig.gcl.setGate(s, q, 1);
                    }
                }
            }
        }
        
        newConfig.expectedAvgDelay = 0.001; // 示例值
    } else {
        // 使用EDRL生成GCL
        NetworkState state;
        state.currentTimeSlot = (int)(simTime().dbl() / timeSlotSize) % numTimeSlots;
        state.avgDelay = latestStatus.avgDelay;
        state.satisfactionRatio = latestStatus.satisfactionRatio;
        
        newConfig.gcl = edrlAgent->generateGCL(state);
        
        // 计算预期性能
        newConfig.expectedAvgDelay = edrlAgent->getAverageDelay();
    }
    
    // 保存配置历史
    configHistory.push_back(newConfig);
    if (configHistory.size() > (size_t)historySize) {
        configHistory.pop_front();
    }
    
    currentConfig = newConfig;
    
    EV << "Generated new configuration v" << configVersion << std::endl;
}

void CNCController::distributeConfiguration() {
    // 发送配置到网络中的TSN Bridge
    cMessage* configMsg = new cMessage("GCLConfig");
    configMsg->addPar("configId") = currentConfig.configId;
    configMsg->addPar("timestamp") = currentConfig.timestamp;
    
    // 发送GCL矩阵
    for (int s = 0; s < numTimeSlots; s++) {
        for (int q = 0; q < numQueues; q++) {
            char parName[32];
            sprintf(parName, "gcl_%d_%d", s, q);
            configMsg->addPar(parName) = currentConfig.gcl.getGate(s, q);
        }
    }
    
    // 广播配置
    int gateCount = gateSize("out");
    for (int i = 0; i < gateCount; i++) {
        send(configMsg->dup(), "out", i);
    }
    delete configMsg;
    
    EV << "Distributed configuration v" << currentConfig.configId 
       << " to " << gateCount << " bridges" << std::endl;
}

void CNCController::analyzeStatusReports() {
    if (statusHistory.empty()) return;
    
    // 计算平均指标
    double sumDelay = 0;
    double sumSatisfaction = 0;
    double sumThroughput = 0;
    int count = 0;
    
    for (const auto& report : statusHistory) {
        sumDelay += report.avgDelay;
        sumSatisfaction += report.satisfactionRatio;
        sumThroughput += report.throughput;
        count++;
    }
    
    if (count > 0) {
        latestStatus.avgDelay = sumDelay / count;
        latestStatus.satisfactionRatio = sumSatisfaction / count;
        latestStatus.throughput = sumThroughput / count;
    }
    
    EV << "Status Analysis - Avg Delay: " << latestStatus.avgDelay * 1000 
       << "ms, Satisfaction: " << latestStatus.satisfactionRatio * 100 
       << "%, Throughput: " << latestStatus.throughput / 1e6 << " Mbps" << std::endl;
}

void CNCController::optimizeConfiguration() {
    EV << "Optimizing configuration due to performance degradation" << std::endl;
    
    // 触发EDRL训练
    edrlAgent->train();
    
    // 生成新的优化配置
    generateNewConfiguration();
}

bool CNCController::checkConstraints(const NetworkConfiguration& config) {
    // 检查GCL约束
    for (int s = 0; s < numTimeSlots; s++) {
        int activeQueues = 0;
        for (int q = 0; q < numQueues; q++) {
            if (config.gcl.getGate(s, q) == 1) {
                activeQueues++;
            }
        }
        // 每个时隙最多激活一个队列
        if (activeQueues > 1) {
            return false;
        }
    }
    return true;
}

double CNCController::evaluateConfiguration(const NetworkConfiguration& config) {
    double score = 0;
    
    // 评估延迟性能
    score += (targetMaxDelay - config.expectedAvgDelay) * 1000;
    
    // 评估满足率
    score += config.expectedAvgDelay * 100;
    
    return score;
}

void CNCController::receiveStatusReport(const NetworkStatusReport& report) {
    latestStatus = report;
    statusHistory.push_back(report);
    
    if (statusHistory.size() > (size_t)historySize) {
        statusHistory.pop_front();
    }
}

void CNCController::triggerReconfiguration() {
    generateNewConfiguration();
    distributeConfiguration();
}

void CNCController::finish() {
    EV << "=== CNC Controller Final Statistics ===" << std::endl;
    EV << "Total Configurations Generated: " << configVersion << std::endl;
    EV << "Final Satisfaction Ratio: " << latestStatus.satisfactionRatio * 100 << "%" << std::endl;
    EV << "Final Average Delay: " << latestStatus.avgDelay * 1000 << " ms" << std::endl;
    EV << "Final Throughput: " << latestStatus.throughput / 1e6 << " Mbps" << std::endl;
    EV << "======================================" << std::endl;
}
