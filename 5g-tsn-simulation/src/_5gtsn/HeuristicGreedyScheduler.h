//
// HeuristicGreedyScheduler.h - 启发式贪心调度算法头文件
// 用于5G-TSN协同优化中的流调度和GCL生成
// 支持任意网络流量到达情况
//

#ifndef HEURISTICGREEDYSCHEDULER_H_
#define HEURISTICGREEDYSCHEDULER_H_

#include <omnetpp.h>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <queue>

using namespace omnetpp;

// 业务流信息结构
struct FlowScheduleInfo {
    int flowId;
    double arrivalTime;    // t_arr,i: 预期到达时间 (μs)
    double transmissionTime; // d_i: 传输耗时 (μs)
    int priority;           // P_i: 优先级 (数值越小优先级越高)
    int assignedQueue;      // 分配的队列 (0-7)
    
    // 调度结果
    double openTime;        // O_i: 开门偏移量 (μs)
    double closeTime;       // C_i: 关门时间 (μs)
    double queuingDelay;    // 排队时延 (μs)
    bool isScheduled;       // 是否已调度
    
    FlowScheduleInfo() 
        : flowId(0), arrivalTime(0), transmissionTime(0), priority(0),
          assignedQueue(0), openTime(0), closeTime(0), queuingDelay(0),
          isScheduled(false) {}
};

// GCL指令结构
struct GCLInstruction {
    int instructionId;
    double startTime;       // 时间起点 (μs)
    double duration;        // 持续时间 (μs)
    uint8_t gateStateMask;  // 门状态掩码 (8 bits, Q7...Q0)
    std::string description; // 硬件行为解释
    
    GCLInstruction() 
        : instructionId(0), startTime(0), duration(0), gateStateMask(0) {}
};

// GCL配置结构
struct GCLConfiguration {
    double cycleTime;       // GCL总周期 (μs)
    std::vector<GCLInstruction> instructions;
    int numQueues;
    double generationTime;  // GCL生成时间戳
    
    GCLConfiguration() : cycleTime(50.0), numQueues(8), generationTime(0) {}
};

// 时间轴上的已占用区间
struct TimeInterval {
    double startTime;
    double endTime;
    int flowId;
    int queueId;
    
    TimeInterval(double s = 0, double e = 0, int f = 0, int q = 0)
        : startTime(s), endTime(e), flowId(f), queueId(q) {}
};

class HeuristicGreedyScheduler {
private:
    // 调度参数
    double cycleTime;       // GCL周期 (μs)
    int numQueues;          // 队列数量
    double currentTime;     // 当前时间 (μs)
    
    // 流管理
    std::map<int, FlowScheduleInfo> allFlows;          // 所有流（通过flowId索引）
    std::vector<FlowScheduleInfo> pendingFlows;         // 待调度的流
    std::vector<FlowScheduleInfo> scheduledFlows;       // 已调度的流
    std::vector<TimeInterval> timeIntervals;            // 时间轴上的已占用区间
    
    // GCL配置
    GCLConfiguration gclConfig;
    
    // 统计信息
    int totalFlowsScheduled;
    int totalFlowsDropped;
    double totalQueuingDelay;
    
public:
    HeuristicGreedyScheduler();
    virtual ~HeuristicGreedyScheduler();
    
    // 初始化调度器
    void initialize(double cycleTime = 50.0, int numQueues = 8);
    
    // ==================== 动态流管理接口 ====================
    
    // 添加单个流到待调度队列
    void addFlow(const FlowScheduleInfo& flow);
    
    // 批量添加流
    void addFlows(const std::vector<FlowScheduleInfo>& flows);
    
    // 移除流
    void removeFlow(int flowId);
    
    // 更新流信息
    void updateFlow(int flowId, const FlowScheduleInfo& updatedFlow);
    
    // 获取当前流数量
    int getPendingFlowCount() const { return pendingFlows.size(); }
    int getScheduledFlowCount() const { return scheduledFlows.size(); }
    int getTotalFlowCount() const { return allFlows.size(); }
    
    // ==================== 调度执行接口 ====================
    
    // 执行完整调度（调度所有待处理流）
    void scheduleAllPendingFlows();
    
    // 增量调度（调度新到达的流）
    void scheduleIncremental();
    
    // 主调度函数（保留向后兼容）
    void scheduleFlows(const std::vector<FlowScheduleInfo>& flows);
    
    // ==================== 时间管理接口 ====================
    
    // 更新当前时间
    void setCurrentTime(double time);
    double getCurrentTime() const { return currentTime; }
    
    // 清理已过期的流（在当前时间之前完成传输的流）
    void cleanupExpiredFlows();
    
    // 检查时间区间是否可用
    bool isTimeIntervalAvailable(double startTime, double duration) const;
    
    // ==================== GCL相关接口 ====================
    
    // 生成GCL（基于当前所有已调度流）
    void generateGCL();
    
    // 获取GCL配置
    const GCLConfiguration& getGCLConfiguration() const { return gclConfig; }
    
    // ==================== 实时门控状态查询接口 ====================
    
    // 根据当前仿真时间（秒），获取当前哪些队列应该开门
    // 返回一个vector，大小为numQueues，true表示开门，false表示关门
    std::vector<bool> getGateStatesAtTime(double simulationTimeSeconds);
    
    // 根据当前仿真时间（秒），获取当前GCL指令
    const GCLInstruction* getCurrentInstruction(double simulationTimeSeconds);
    
    // 检查特定队列在给定时间是否开门
    bool isQueueOpenAtTime(int queueId, double simulationTimeSeconds);
    
    // ==================== 结果获取接口 ====================
    
    // 获取所有已调度流
    const std::vector<FlowScheduleInfo>& getScheduledFlows() const { return scheduledFlows; }
    
    // 获取特定流的信息
    bool getFlowInfo(int flowId, FlowScheduleInfo& info) const;
    
    // ==================== 统计和调试接口 ====================
    
    // 获取统计信息
    int getTotalScheduled() const { return totalFlowsScheduled; }
    int getTotalDropped() const { return totalFlowsDropped; }
    double getAverageQueuingDelay() const;
    
    // 辅助函数: 打印调度结果
    void printScheduleResult();
    void printGCLTable();
    void printTimeIntervals();
    
    // 示例演示函数: 3流冲突场景（保留）
    void demo3FlowScheduling();
    
    // 随机流生成演示
    void demoRandomFlows(int numFlows, double maxArrivalTime = 100.0);
    
private:
    // ==================== 内部调度算法 ====================
    
    // 步骤1: 构建调度队列 (排序)
    void sortFlows(std::vector<FlowScheduleInfo>& flows);
    
    // 步骤2: 时间窗分配
    void allocateTimeWindows(std::vector<FlowScheduleInfo>& flows);
    
    // 为单个流分配时间窗
    bool allocateTimeWindowForFlow(FlowScheduleInfo& flow);
    
    // 更新时间轴占用信息
    void updateTimeIntervals();
    
    // 查找最早可用的时间窗
    double findEarliestAvailableTime(double desiredStartTime, double duration) const;
    
    // ==================== 辅助函数 ====================
    
    // 队列优先级映射
    int priorityToQueue(int priority);
    
    // 生成门状态掩码
    uint8_t generateGateMask(int queueId);
    
    // 合并相邻的GCL指令（优化）
    void mergeAdjacentGCLInstructions();
};

#endif
