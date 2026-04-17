//
// HeuristicGreedyScheduler.cc - 启发式贪心调度算法实现
// 用于5G-TSN协同优化中的流调度和GCL生成
// 支持任意网络流量到达情况
//

#include "HeuristicGreedyScheduler.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <ctime>

using namespace std;

HeuristicGreedyScheduler::HeuristicGreedyScheduler()
    : cycleTime(50.0), numQueues(8), currentTime(0.0),
      totalFlowsScheduled(0), totalFlowsDropped(0), totalQueuingDelay(0.0) {
}

HeuristicGreedyScheduler::~HeuristicGreedyScheduler() {
}

void HeuristicGreedyScheduler::initialize(double cycleTime, int numQueues) {
    this->cycleTime = cycleTime;
    this->numQueues = numQueues;
    this->currentTime = 0.0;
    
    allFlows.clear();
    pendingFlows.clear();
    scheduledFlows.clear();
    timeIntervals.clear();
    gclConfig.instructions.clear();
    gclConfig.cycleTime = cycleTime;
    gclConfig.numQueues = numQueues;
    
    totalFlowsScheduled = 0;
    totalFlowsDropped = 0;
    totalQueuingDelay = 0.0;
    
    EV << "Heuristic Greedy Scheduler initialized: cycleTime=" << cycleTime 
       << "μs, numQueues=" << numQueues << endl;
}

// ==================== 动态流管理接口 ====================

void HeuristicGreedyScheduler::addFlow(const FlowScheduleInfo& flow) {
    // 检查是否已存在该流
    if (allFlows.find(flow.flowId) != allFlows.end()) {
        EV << "Warning: Flow " << flow.flowId << " already exists, updating it" << endl;
        updateFlow(flow.flowId, flow);
        return;
    }
    
    // 添加到所有流和待调度队列
    allFlows[flow.flowId] = flow;
    pendingFlows.push_back(flow);
    
    EV << "Added flow S" << flow.flowId 
       << ": arrival=" << flow.arrivalTime 
       << "μs, duration=" << flow.transmissionTime 
       << "μs, priority=" << flow.priority << endl;
}

void HeuristicGreedyScheduler::addFlows(const std::vector<FlowScheduleInfo>& flows) {
    for (const auto& flow : flows) {
        addFlow(flow);
    }
    EV << "Added " << flows.size() << " flows in batch" << endl;
}

void HeuristicGreedyScheduler::removeFlow(int flowId) {
    auto it = allFlows.find(flowId);
    if (it != allFlows.end()) {
        allFlows.erase(it);
        
        // 从待调度队列中移除
        pendingFlows.erase(
            remove_if(pendingFlows.begin(), pendingFlows.end(),
                [flowId](const FlowScheduleInfo& f) { return f.flowId == flowId; }),
            pendingFlows.end()
        );
        
        // 从已调度队列中移除
        auto origSize = scheduledFlows.size();
        scheduledFlows.erase(
            remove_if(scheduledFlows.begin(), scheduledFlows.end(),
                [flowId](const FlowScheduleInfo& f) { return f.flowId == flowId; }),
            scheduledFlows.end()
        );
        
        if (scheduledFlows.size() != origSize) {
            // 有已调度的流被移除，需要更新时间轴和GCL
            updateTimeIntervals();
            generateGCL();
        }
        
        EV << "Removed flow S" << flowId << endl;
    } else {
        EV << "Warning: Flow " << flowId << " not found for removal" << endl;
    }
}

void HeuristicGreedyScheduler::updateFlow(int flowId, const FlowScheduleInfo& updatedFlow) {
    auto it = allFlows.find(flowId);
    if (it != allFlows.end()) {
        // 先移除旧的流（如果已调度）
        removeFlow(flowId);
        
        // 添加更新后的流
        addFlow(updatedFlow);
        
        EV << "Updated flow S" << flowId << endl;
    }
}

bool HeuristicGreedyScheduler::getFlowInfo(int flowId, FlowScheduleInfo& info) const {
    auto it = allFlows.find(flowId);
    if (it != allFlows.end()) {
        info = it->second;
        return true;
    }
    return false;
}

// ==================== 调度执行接口 ====================

void HeuristicGreedyScheduler::scheduleAllPendingFlows() {
    if (pendingFlows.empty()) {
        EV << "No pending flows to schedule" << endl;
        return;
    }
    
    EV << "Scheduling all " << pendingFlows.size() << " pending flows..." << endl;
    
    // 复制待调度流进行处理
    std::vector<FlowScheduleInfo> flowsToSchedule = pendingFlows;
    
    // 清空待调度队列
    pendingFlows.clear();
    
    // 执行调度
    sortFlows(flowsToSchedule);
    allocateTimeWindows(flowsToSchedule);
    
    EV << "Scheduling complete. Scheduled " << scheduledFlows.size() << " flows total" << endl;
}

void HeuristicGreedyScheduler::scheduleIncremental() {
    if (pendingFlows.empty()) {
        return;
    }
    
    EV << "Incremental scheduling: " << pendingFlows.size() << " new flows" << endl;
    
    // 对新到达的流进行排序
    sortFlows(pendingFlows);
    
    // 逐个调度新流
    for (auto& flow : pendingFlows) {
        if (allocateTimeWindowForFlow(flow)) {
            scheduledFlows.push_back(flow);
            allFlows[flow.flowId] = flow;
            totalFlowsScheduled++;
            totalQueuingDelay += flow.queuingDelay;
        } else {
            totalFlowsDropped++;
            EV << "Warning: Failed to schedule flow S" << flow.flowId << endl;
        }
    }
    
    pendingFlows.clear();
    updateTimeIntervals();
    generateGCL();
}

void HeuristicGreedyScheduler::scheduleFlows(const std::vector<FlowScheduleInfo>& flows) {
    // 保留向后兼容的接口
    addFlows(flows);
    scheduleAllPendingFlows();
}

// ==================== 时间管理接口 ====================

void HeuristicGreedyScheduler::setCurrentTime(double time) {
    currentTime = time;
    EV << "Current time set to " << time << "μs" << endl;
}

void HeuristicGreedyScheduler::cleanupExpiredFlows() {
    EV << "Cleaning up expired flows before " << currentTime << "μs..." << endl;
    
    int expiredCount = 0;
    
    // 移除已过期的流（closeTime < currentTime）
    auto origSize = scheduledFlows.size();
    scheduledFlows.erase(
        remove_if(scheduledFlows.begin(), scheduledFlows.end(),
            [this, &expiredCount](const FlowScheduleInfo& f) {
                if (f.closeTime < this->currentTime) {
                    expiredCount++;
                    allFlows.erase(f.flowId);
                    return true;
                }
                return false;
            }),
        scheduledFlows.end()
    );
    
    if (expiredCount > 0) {
        EV << "Cleaned up " << expiredCount << " expired flows" << endl;
        updateTimeIntervals();
        generateGCL();
    }
}

bool HeuristicGreedyScheduler::isTimeIntervalAvailable(double startTime, double duration) const {
    double endTime = startTime + duration;
    
    for (const auto& interval : timeIntervals) {
        // 检查是否有重叠
        if (!(endTime <= interval.startTime || startTime >= interval.endTime)) {
            return false;
        }
    }
    return true;
}

// ==================== 内部调度算法 ====================

void HeuristicGreedyScheduler::sortFlows(std::vector<FlowScheduleInfo>& flows) {
    // 双关键字排序：主关键字为优先级(升序)，次关键字为到达时间(升序)
    std::sort(flows.begin(), flows.end(), 
        [](const FlowScheduleInfo& a, const FlowScheduleInfo& b) {
            if (a.priority != b.priority) {
                return a.priority < b.priority;
            }
            return a.arrivalTime < b.arrivalTime;
        });
    
    EV << "Flows sorted: ";
    for (const auto& flow : flows) {
        EV << "S" << flow.flowId << "(P=" << flow.priority 
           << ",t=" << flow.arrivalTime << ") ";
    }
    EV << endl;
}

void HeuristicGreedyScheduler::allocateTimeWindows(std::vector<FlowScheduleInfo>& flows) {
    for (auto& flow : flows) {
        if (allocateTimeWindowForFlow(flow)) {
            scheduledFlows.push_back(flow);
            allFlows[flow.flowId] = flow;
            totalFlowsScheduled++;
            totalQueuingDelay += flow.queuingDelay;
            // 关键：每调度一个流后立即更新时间轴，以便下一个流能正确看到占用情况！
            updateTimeIntervals();
        } else {
            totalFlowsDropped++;
            EV << "Warning: Failed to schedule flow S" << flow.flowId << endl;
        }
    }
    
    generateGCL();
}

bool HeuristicGreedyScheduler::allocateTimeWindowForFlow(FlowScheduleInfo& flow) {
    // 分配队列
    flow.assignedQueue = priorityToQueue(flow.priority);
    
    // 找到最早可用的时间窗
    double earliestAvailable = findEarliestAvailableTime(flow.arrivalTime, flow.transmissionTime);
    
    // 检查是否在周期内可以调度
    if (earliestAvailable + flow.transmissionTime > cycleTime) {
        // 超出当前周期，尝试在下一个周期调度
        // 这里简化处理：标记为不可调度
        return false;
    }
    
    // 计算开门时间：O_i = max(t_arr,i, 最早可用时间)
    flow.openTime = std::max(flow.arrivalTime, earliestAvailable);
    
    // 计算关门时间：C_i = O_i + d_i
    flow.closeTime = flow.openTime + flow.transmissionTime;
    
    // 计算排队时延
    flow.queuingDelay = flow.openTime - flow.arrivalTime;
    flow.isScheduled = true;
    
    EV << "Scheduled flow S" << flow.flowId 
       << ": [" << flow.openTime << ", " << flow.closeTime 
       << "], queue=" << flow.assignedQueue
       << ", delay=" << flow.queuingDelay << "μs" << endl;
    
    return true;
}

void HeuristicGreedyScheduler::updateTimeIntervals() {
    timeIntervals.clear();
    
    for (const auto& flow : scheduledFlows) {
        if (flow.isScheduled) {
            timeIntervals.emplace_back(
                flow.openTime, 
                flow.closeTime, 
                flow.flowId, 
                flow.assignedQueue
            );
        }
    }
    
    // 按开始时间排序
    std::sort(timeIntervals.begin(), timeIntervals.end(),
        [](const TimeInterval& a, const TimeInterval& b) {
            return a.startTime < b.startTime;
        });
}

double HeuristicGreedyScheduler::findEarliestAvailableTime(double desiredStartTime, double duration) const {
    // 首先检查期望时间是否可用
    if (isTimeIntervalAvailable(desiredStartTime, duration)) {
        return desiredStartTime;
    }
    
    // 如果不可用，找到最早的可用时间
    double candidateTime = desiredStartTime;
    
    // 检查所有已占用区间，找间隙
    for (const auto& interval : timeIntervals) {
        if (interval.startTime > candidateTime) {
            // 检查[candidateTime, interval.startTime)是否足够
            if (interval.startTime - candidateTime >= duration) {
                return candidateTime;
            }
        }
        // 更新候选时间到当前区间结束
        candidateTime = std::max(candidateTime, interval.endTime);
    }
    
    // 最后检查周期结束前的剩余时间
    return candidateTime;
}

// ==================== GCL相关接口 ====================

void HeuristicGreedyScheduler::generateGCL() {
    gclConfig.instructions.clear();
    gclConfig.generationTime = currentTime;
    
    if (scheduledFlows.empty()) {
        // 没有流，生成全关闭的GCL
        GCLInstruction emptyInstr;
        emptyInstr.instructionId = 0;
        emptyInstr.startTime = 0.0;
        emptyInstr.duration = cycleTime;
        emptyInstr.gateStateMask = 0x00;
        emptyInstr.description = "无调度流，所有队列关闭";
        gclConfig.instructions.push_back(emptyInstr);
        return;
    }
    
    double currentTimePoint = 0.0;
    int instructionId = 0;
    
    // 确保时间区间是按时间排序的
    updateTimeIntervals();
    
    // 初始到第一个流开始前
    if (!timeIntervals.empty() && timeIntervals[0].startTime > 0) {
        GCLInstruction instr0;
        instr0.instructionId = instructionId++;
        instr0.startTime = 0.0;
        instr0.duration = timeIntervals[0].startTime;
        instr0.gateStateMask = 0x00;
        instr0.description = "初始阶段，等待流到达";
        gclConfig.instructions.push_back(instr0);
        currentTimePoint = timeIntervals[0].startTime;
    }
    
    // 为每个时间区间生成GCL指令
    for (size_t i = 0; i < timeIntervals.size(); i++) {
        const auto& interval = timeIntervals[i];
        
        // 流的指令
        GCLInstruction flowInstr;
        flowInstr.instructionId = instructionId++;
        flowInstr.startTime = interval.startTime;
        flowInstr.duration = interval.endTime - interval.startTime;
        flowInstr.gateStateMask = generateGateMask(interval.queueId);
        
        // 查找对应的流信息
        FlowScheduleInfo flowInfo;
        if (getFlowInfo(interval.flowId, flowInfo)) {
            flowInstr.description = "S" + std::to_string(interval.flowId) + 
                                    "时间窗，Q" + std::to_string(interval.queueId) + "开门放行";
        } else {
            flowInstr.description = "Q" + std::to_string(interval.queueId) + "开门";
        }
        
        gclConfig.instructions.push_back(flowInstr);
        currentTimePoint = interval.endTime;
        
        // 如果不是最后一个区间，检查间隙
        if (i < timeIntervals.size() - 1) {
            double nextStart = timeIntervals[i + 1].startTime;
            if (nextStart > currentTimePoint) {
                GCLInstruction gapInstr;
                gapInstr.instructionId = instructionId++;
                gapInstr.startTime = currentTimePoint;
                gapInstr.duration = nextStart - currentTimePoint;
                gapInstr.gateStateMask = 0x00;
                gapInstr.description = "间隙，全部关门等待";
                gclConfig.instructions.push_back(gapInstr);
                currentTimePoint = nextStart;
            }
        }
    }
    
    // 最后一个流结束到周期结束
    if (currentTimePoint < cycleTime) {
        GCLInstruction lastInstr;
        lastInstr.instructionId = instructionId++;
        lastInstr.startTime = currentTimePoint;
        lastInstr.duration = cycleTime - currentTimePoint;
        lastInstr.gateStateMask = 0x00;
        lastInstr.description = "剩余周期留给背景流";
        gclConfig.instructions.push_back(lastInstr);
    }
    
    // 优化：合并相邻的相同状态的指令
    mergeAdjacentGCLInstructions();
    
    EV << "GCL generated with " << gclConfig.instructions.size() << " instructions" << endl;
}

void HeuristicGreedyScheduler::mergeAdjacentGCLInstructions() {
    if (gclConfig.instructions.size() <= 1) {
        return;
    }
    
    std::vector<GCLInstruction> merged;
    merged.push_back(gclConfig.instructions[0]);
    
    for (size_t i = 1; i < gclConfig.instructions.size(); i++) {
        GCLInstruction& last = merged.back();
        const GCLInstruction& current = gclConfig.instructions[i];
        
        if (last.gateStateMask == current.gateStateMask) {
            // 可以合并
            last.duration += current.duration;
            last.description += " + " + current.description;
        } else {
            merged.push_back(current);
        }
    }
    
    // 重新编号
    for (size_t i = 0; i < merged.size(); i++) {
        merged[i].instructionId = i;
    }
    
    gclConfig.instructions = merged;
}

// ==================== 辅助函数 ====================

int HeuristicGreedyScheduler::priorityToQueue(int priority) {
    // 优先级映射：优先级1->队列7(最高), 优先级2->队列6, 依此类推
    if (priority == 1) {
        return 7;
    } else if (priority == 2) {
        return 6;
    } else {
        return std::max(0, 7 - priority);
    }
}

uint8_t HeuristicGreedyScheduler::generateGateMask(int queueId) {
    // 生成8位门状态掩码，queueId=7对应最高位
    if (queueId >= 0 && queueId < 8) {
        return (uint8_t)(1 << (7 - queueId));
    }
    return 0x00;
}

double HeuristicGreedyScheduler::getAverageQueuingDelay() const {
    if (totalFlowsScheduled > 0) {
        return totalQueuingDelay / totalFlowsScheduled;
    }
    return 0.0;
}

// ==================== 统计和调试接口 ====================

void HeuristicGreedyScheduler::printScheduleResult() {
    cout << endl << "========== 启发式贪心调度结果 ==========" << endl;
    cout << "总流数: " << allFlows.size() 
         << ", 已调度: " << scheduledFlows.size() 
         << ", 待调度: " << pendingFlows.size() << endl;
    
    if (!scheduledFlows.empty()) {
        cout << endl << "流ID | 到达时间 | 传输时间 | 优先级 | 开门时间 | 关门时间 | 排队时延 | 队列" << endl;
        cout << "-----|----------|----------|--------|----------|----------|----------|------" << endl;
        
        for (const auto& flow : scheduledFlows) {
            cout << " S" << setw(2) << flow.flowId 
                 << "  | " << setw(8) << flow.arrivalTime
                 << " | " << setw(8) << flow.transmissionTime
                 << " | " << setw(6) << flow.priority
                 << " | " << setw(8) << flow.openTime
                 << " | " << setw(8) << flow.closeTime
                 << " | " << setw(8) << flow.queuingDelay
                 << " | Q" << flow.assignedQueue << endl;
        }
    }
    cout << "=======================================" << endl;
}

void HeuristicGreedyScheduler::printGCLTable() {
    cout << endl << "========== GCL 门控指令表 ==========" << endl;
    cout << "指令 | 时间起点 | 持续时间 | 门状态掩码 | 硬件行为" << endl;
    cout << "-----|----------|----------|------------|----------" << endl;
    
    for (const auto& instr : gclConfig.instructions) {
        cout << " " << setw(3) << instr.instructionId
             << "  | " << setw(8) << instr.startTime
             << " | " << setw(8) << instr.duration
             << " | 0x" << hex << setw(2) << setfill('0') 
             << (int)instr.gateStateMask << dec << setfill(' ')
             << "     | " << instr.description << endl;
    }
    cout << "=====================================" << endl;
}

void HeuristicGreedyScheduler::printTimeIntervals() {
    cout << endl << "========== 时间轴占用情况 ==========" << endl;
    if (timeIntervals.empty()) {
        cout << "时间轴空闲" << endl;
    } else {
        for (const auto& interval : timeIntervals) {
            cout << "[" << interval.startTime << ", " << interval.endTime 
                 << "): S" << interval.flowId << " (Q" << interval.queueId << ")" << endl;
        }
    }
    cout << "===================================" << endl;
}

// ==================== 演示函数 ====================

void HeuristicGreedyScheduler::demo3FlowScheduling() {
    EV << endl << "========== 3流冲突场景演示 ==========" << endl;
    
    initialize(50.0, 8);
    
    // 创建3个业务流
    FlowScheduleInfo s1;
    s1.flowId = 1;
    s1.arrivalTime = 10.0;
    s1.transmissionTime = 5.0;
    s1.priority = 1;
    addFlow(s1);
    
    FlowScheduleInfo s2;
    s2.flowId = 2;
    s2.arrivalTime = 12.0;
    s2.transmissionTime = 4.0;
    s2.priority = 2;
    addFlow(s2);
    
    FlowScheduleInfo s3;
    s3.flowId = 3;
    s3.arrivalTime = 25.0;
    s3.transmissionTime = 6.0;
    s3.priority = 2;
    addFlow(s3);
    
    // 执行调度
    scheduleAllPendingFlows();
    
    // 打印结果
    printScheduleResult();
    printTimeIntervals();
    printGCLTable();
    
    EV << "========== 演示完成 ==========" << endl << endl;
}

void HeuristicGreedyScheduler::demoRandomFlows(int numFlows, double maxArrivalTime) {
    EV << endl << "========== 随机流调度演示: " << numFlows << "个流 ==========" << endl;
    
    initialize(100.0, 8);
    
    // 随机数生成器
    mt19937 rng(time(nullptr));
    uniform_real_distribution<double> arrivalDist(0, maxArrivalTime);
    uniform_real_distribution<double> durationDist(2, 10);
    uniform_int_distribution<int> priorityDist(1, 4);
    
    for (int i = 1; i <= numFlows; i++) {
        FlowScheduleInfo flow;
        flow.flowId = i;
        flow.arrivalTime = arrivalDist(rng);
        flow.transmissionTime = durationDist(rng);
        flow.priority = priorityDist(rng);
        addFlow(flow);
    }
    
    // 执行调度
    scheduleAllPendingFlows();
    
    // 打印统计
    cout << endl << "统计信息:" << endl;
    cout << "  总流数: " << numFlows << endl;
    cout << "  成功调度: " << getTotalScheduled() << endl;
    cout << "  丢弃: " << getTotalDropped() << endl;
    cout << "  平均排队时延: " << fixed << setprecision(2) 
         << getAverageQueuingDelay() << "μs" << endl;
    
    printScheduleResult();
    printGCLTable();
    
    EV << "========== 随机流演示完成 ==========" << endl << endl;
}

// ==================== 实时门控状态查询接口 ====================

std::vector<bool> HeuristicGreedyScheduler::getGateStatesAtTime(double simulationTimeSeconds) {
    // 初始化所有队列为开门（默认情况，保证终端能收到流量）
    std::vector<bool> gateStates(numQueues, true);
    
    // 如果有GCL指令，则按照GCL执行；否则保持所有队列开门
    if (!gclConfig.instructions.empty() && !scheduledFlows.empty()) {
        // 将秒转换为微秒
        double timeUs = simulationTimeSeconds * 1e6;
        
        // 在循环GCL周期内查找对应的时间点
        double cycleTime = gclConfig.cycleTime;
        double relativeTime = fmod(timeUs, cycleTime);
        if (relativeTime < 0) relativeTime += cycleTime;
        
        // 查找当前对应的GCL指令
        const GCLInstruction* currentInstr = getCurrentInstruction(simulationTimeSeconds);
        if (currentInstr) {
            // 先关闭所有队列
            for (int q = 0; q < numQueues; q++) {
                gateStates[q] = false;
            }
            
            // 根据门状态掩码设置队列开关
            uint8_t mask = currentInstr->gateStateMask;
            for (int q = 0; q < numQueues && q < 8; q++) {
                // Q7对应最高位，Q0对应最低位
                int bitPosition = 7 - q;
                if (mask & (1 << bitPosition)) {
                    gateStates[q] = true;
                }
            }
        }
    }
    
    return gateStates;
}

const GCLInstruction* HeuristicGreedyScheduler::getCurrentInstruction(double simulationTimeSeconds) {
    // 将秒转换为微秒
    double timeUs = simulationTimeSeconds * 1e6;
    
    // 在循环GCL周期内查找对应的时间点
    double cycleTime = gclConfig.cycleTime;
    double relativeTime = fmod(timeUs, cycleTime);
    if (relativeTime < 0) relativeTime += cycleTime;
    
    // 查找对应的GCL指令
    for (size_t i = 0; i < gclConfig.instructions.size(); i++) {
        const GCLInstruction& instr = gclConfig.instructions[i];
        double instrEnd = instr.startTime + instr.duration;
        
        if (relativeTime >= instr.startTime && relativeTime < instrEnd) {
            return &gclConfig.instructions[i];
        }
    }
    
    // 如果没有找到，返回nullptr（所有门关闭）
    return nullptr;
}

bool HeuristicGreedyScheduler::isQueueOpenAtTime(int queueId, double simulationTimeSeconds) {
    if (queueId < 0 || queueId >= numQueues) {
        return false;
    }
    
    std::vector<bool> gateStates = getGateStatesAtTime(simulationTimeSeconds);
    return gateStates[queueId];
}
