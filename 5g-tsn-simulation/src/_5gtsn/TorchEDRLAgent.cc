//
// TorchEDRLAgent.cc - PyTorch-based EDRL Agent Implementation
// 论文: An Ensemble Deep Reinforcement Learning-Based Asynchronous Scheduling Mechanism for 5G-TSN
//

#include "TorchEDRLAgent.h"
#include "FlowPacket_m.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

TorchEDRLAgent::TorchEDRLAgent() 
    : stateHistorySize(100), decisionHistorySize(1000),
      stateSize(0), actionSize(0), learningRate(0.001), gamma(0.99),
      epsilon(1.0), epsilonDecay(0.995), epsilonMin(0.01), batchSize(32),
      numTimeSlots(1024), numQueues(8), numSwitches(1), timeSlotSize(0.000015625),
      hyperPeriod(0.016), flowCounter(0),
      enablePPO(true), enableDDPG(true), enableSAC(true), enableVoting(true),
      isTrainingMode(true), isModelLoaded(false), currentGCL(1024, 8) {
}

TorchEDRLAgent::~TorchEDRLAgent() {
}

void TorchEDRLAgent::initialize(int numTS, int numQ, int numSW, 
                                double tsSize, double hp,
                                double lr, int bs) {
    numTimeSlots = numTS;
    numQueues = numQ;
    numSwitches = numSW;
    timeSlotSize = tsSize;
    hyperPeriod = hp;
    learningRate = lr;
    batchSize = bs;
    
    stateSize = 1 + numQueues + numTimeSlots * numQueues + 3;
    actionSize = numTimeSlots * numQueues;
    
    EV << "Initializing TorchEDRLAgent with LibTorch backend" << std::endl;
    EV << "  State size: " << stateSize << std::endl;
    EV << "  Action size: " << actionSize << std::endl;
    EV << "  Learning rate: " << learningRate << std::endl;
    EV << "  Batch size: " << batchSize << std::endl;
    
    try {
        ppoAgent = std::make_unique<TorchPPOAgent>(stateSize, actionSize, learningRate);
        EV << "  PPO agent initialized" << std::endl;
        
        ddpgAgent = std::make_unique<TorchDDPGAgent>(stateSize, actionSize, learningRate, 10000);
        EV << "  DDPG agent initialized" << std::endl;
        
        sacAgent = std::make_unique<TorchSACAgent>(stateSize, actionSize, learningRate, 10000);
        EV << "  SAC agent initialized" << std::endl;
        
        voting = std::make_unique<QValueVotingMechanism>(1.0, 1000);
        EV << "  Q-value voting mechanism initialized" << std::endl;
        
        currentGCL = GCLMatrix(numTimeSlots, numQueues);
        
        EV << "TorchEDRLAgent initialization complete" << std::endl;
    } catch (const cRuntimeError& e) {
        throw cRuntimeError("Failed to initialize TorchEDRLAgent: %s", e.what());
    }
}

torch::Tensor TorchEDRLAgent::encodeState(const NetworkState& state) {
    std::vector<double> encoded;
    
    encoded.push_back(state.currentTimeSlot / (double)numTimeSlots);
    
    for (int q = 0; q < numQueues; q++) {
        int len = (q < (int)state.queueLengths.size()) ? state.queueLengths[q] : 0;
        encoded.push_back(len / 100.0);
    }
    
    for (int s = 0; s < numTimeSlots; s++) {
        for (int q = 0; q < numQueues; q++) {
            int occ = 0;
            if (s < (int)state.slotOccupancy.size() && q < (int)state.slotOccupancy[s].size()) {
                occ = state.slotOccupancy[s][q];
            }
            encoded.push_back(occ / 100.0);
        }
    }
    
    encoded.push_back(state.avgDelay / hyperPeriod);
    encoded.push_back(state.satisfactionRatio);
    encoded.push_back(state.throughput / 1e9);
    
    return torch::tensor(encoded, torch::kFloat32);
}

double TorchEDRLAgent::computeReward(const NetworkState& state, 
                                     const SchedulingDecision& decision,
                                     const NetworkState& nextState) {
    double mu = 0.4;
    double lambda = 0.3;
    double omega = 0.3;
    
    double ra = mu * nextState.satisfactionRatio;
    double rb = lambda * (1.0 - state.satisfactionRatio);
    double rc = omega * (1.0 - nextState.avgDelay / hyperPeriod);
    
    return ra + rb + rc;
}

void TorchEDRLAgent::updateEpsilon() {
    epsilon = std::max(epsilonMin, epsilon * epsilonDecay);
}

SchedulingDecision TorchEDRLAgent::makeDecision(const NetworkState& state, 
                                                 const FlowPacket* flow,
                                                 const ActionMask* mask) {
    SchedulingDecision decision;
    decision.flowId = flowCounter++;
    decision.decisionTime = simTime().dbl();
    
    if (flow) {
        decision.flowId = flow->getFlowId();
    }
    
    auto stateTensor = encodeState(state);
    
    SchedulingDecision ppoDecision, ddpgDecision, sacDecision;
    double ppoQ = 0, ddpgQ = 0, sacQ = 0;
    
    if (enablePPO) {
        ppoDecision = makeDecisionPPO(state, mask);
        ppoQ = ppoAgent->getQValue(stateTensor);
    }
    
    if (enableDDPG) {
        ddpgDecision = makeDecisionDDPG(state);
        ddpgQ = ddpgAgent->getQValue(stateTensor, ddpgDecision.assignedSlot * numQueues + ddpgDecision.assignedQueue);
    }
    
    if (enableSAC) {
        sacDecision = makeDecisionSAC(state);
        sacQ = sacAgent->getQValue(stateTensor, sacDecision.assignedSlot * numQueues + sacDecision.assignedQueue);
    }
    
    if (enableVoting && (enablePPO || enableDDPG || enableSAC)) {
        decision = ensembleDecision(ppoDecision, ppoQ, ddpgDecision, ddpgQ, sacDecision, sacQ);
    } else if (enablePPO) {
        decision = ppoDecision;
    } else if (enableDDPG) {
        decision = ddpgDecision;
    } else if (enableSAC) {
        decision = sacDecision;
    }
    
    decision.qValue = (ppoQ + ddpgQ + sacQ) / 3.0;
    
    if (isTrainingMode) {
        updateEpsilon();
    }
    
    pendingDecisions[decision.flowId] = decision;
    
    return decision;
}

SchedulingDecision TorchEDRLAgent::makeDecisionPPO(const NetworkState& state, 
                                                    const ActionMask* mask) {
    SchedulingDecision decision;
    auto stateTensor = encodeState(state);
    
    bool explore = isTrainingMode && (double)rand() / RAND_MAX < epsilon;
    auto [action, logProb] = ppoAgent->selectAction(stateTensor, explore, mask);
    
    decision.assignedSlot = action / numQueues;
    decision.assignedQueue = action % numQueues;
    decision.logProb = logProb;
    decision.confidence = std::exp(logProb);
    
    return decision;
}

SchedulingDecision TorchEDRLAgent::makeDecisionDDPG(const NetworkState& state) {
    SchedulingDecision decision;
    auto stateTensor = encodeState(state);
    
    bool explore = isTrainingMode && (double)rand() / RAND_MAX < epsilon;
    auto [action, qValue] = ddpgAgent->selectAction(stateTensor, explore);
    
    decision.assignedSlot = action / numQueues;
    decision.assignedQueue = action % numQueues;
    decision.qValue = qValue;
    decision.confidence = 1.0 / (1.0 + std::exp(-qValue));
    
    return decision;
}

SchedulingDecision TorchEDRLAgent::makeDecisionSAC(const NetworkState& state) {
    SchedulingDecision decision;
    auto stateTensor = encodeState(state);
    
    bool explore = isTrainingMode;
    auto [action, qValue] = sacAgent->selectAction(stateTensor, explore);
    
    decision.assignedSlot = action / numQueues;
    decision.assignedQueue = action % numQueues;
    decision.qValue = qValue;
    decision.confidence = 1.0 / (1.0 + std::exp(-qValue));
    
    return decision;
}

SchedulingDecision TorchEDRLAgent::ensembleDecision(
    const SchedulingDecision& ppo, double ppoQ,
    const SchedulingDecision& ddpg, double ddpgQ,
    const SchedulingDecision& sac, double sacQ) {
    
    std::vector<double> qValues;
    std::vector<SchedulingDecision> decisions;
    
    if (enablePPO) {
        qValues.push_back(ppoQ);
        decisions.push_back(ppo);
    }
    if (enableDDPG) {
        qValues.push_back(ddpgQ);
        decisions.push_back(ddpg);
    }
    if (enableSAC) {
        qValues.push_back(sacQ);
        decisions.push_back(sac);
    }
    
    if (qValues.empty()) {
        return SchedulingDecision();
    }
    
    int selectedIdx = voting->selectAction(qValues, rng);
    SchedulingDecision decision = decisions[selectedIdx];
    decision.qValue = qValues[selectedIdx];
    
    return decision;
}

void TorchEDRLAgent::receiveFeedback(int flowId, double actualDelay, bool satisfied) {
    auto it = pendingDecisions.find(flowId);
    if (it == pendingDecisions.end()) return;
    
    SchedulingDecision decision = it->second;
    pendingDecisions.erase(it);
    
    double reward = satisfied ? 1.0 : -0.5;
    reward -= actualDelay / hyperPeriod * 0.1;
    
    rewardHistory.push_back(reward);
    if (rewardHistory.size() > 1000) rewardHistory.pop_front();
    
    satisfactionHistory.push_back(satisfied ? 1.0 : 0.0);
    if (satisfactionHistory.size() > 1000) satisfactionHistory.pop_front();
    
    delayHistory.push_back(actualDelay);
    if (delayHistory.size() > 1000) delayHistory.pop_front();
    
    decisionHistory.push_back(decision);
    if (decisionHistory.size() > decisionHistorySize) decisionHistory.pop_front();
}

void TorchEDRLAgent::storeExperience(const Experience& exp) {
    auto stateTensor = encodeState(exp.state);
    auto nextStateTensor = encodeState(exp.nextState);
    
    int action = exp.action.assignedSlot * numQueues + exp.action.assignedQueue;
    
    if (enablePPO) {
        ppoAgent->storeTransition(stateTensor, action, exp.action.logProb,
                                  exp.reward.total, nextStateTensor, exp.done);
    }
    
    if (enableDDPG) {
        ddpgAgent->storeTransition(stateTensor, action, exp.reward.total,
                                   nextStateTensor, exp.done);
    }
    
    if (enableSAC) {
        sacAgent->storeTransition(stateTensor, action, exp.reward.total,
                                  nextStateTensor, exp.done);
    }
}

void TorchEDRLAgent::updateNetworkState(const NetworkState& state) {
    currentState = state;
    stateHistory.push_back(state);
    if (stateHistory.size() > stateHistorySize) stateHistory.pop_front();
}

void TorchEDRLAgent::updateQueueLengths(const std::vector<int>& lengths) {
    currentState.queueLengths = lengths;
}

void TorchEDRLAgent::updateSlotOccupancy(const std::vector<std::vector<int>>& occupancy) {
    currentState.slotOccupancy = occupancy;
}

void TorchEDRLAgent::train() {
    trainPPO();
    trainDDPG();
    trainSAC();
}

void TorchEDRLAgent::trainPPO() {
    if (enablePPO && ppoAgent) {
        ppoAgent->train();
    }
}

void TorchEDRLAgent::trainDDPG() {
    if (enableDDPG && ddpgAgent) {
        ddpgAgent->train();
    }
}

void TorchEDRLAgent::trainSAC() {
    if (enableSAC && sacAgent) {
        sacAgent->train();
    }
}

GCLMatrix TorchEDRLAgent::generateGCL(const NetworkState& state) {
    GCLMatrix gcl(numTimeSlots, numQueues);
    
    for (const auto& decision : decisionHistory) {
        if (decision.assignedSlot >= 0 && decision.assignedSlot < numTimeSlots &&
            decision.assignedQueue >= 0 && decision.assignedQueue < numQueues) {
            gcl.setGate(decision.assignedSlot, decision.assignedQueue, 1);
        }
    }
    
    return gcl;
}

void TorchEDRLAgent::updateGCL(const GCLMatrix& newGCL) {
    currentGCL = newGCL;
}

double TorchEDRLAgent::getAverageReward() const {
    if (rewardHistory.empty()) return 0.0;
    double sum = 0;
    for (double r : rewardHistory) sum += r;
    return sum / rewardHistory.size();
}

double TorchEDRLAgent::getAverageSatisfaction() const {
    if (satisfactionHistory.empty()) return 0.0;
    double sum = 0;
    for (double s : satisfactionHistory) sum += s;
    return sum / satisfactionHistory.size();
}

double TorchEDRLAgent::getAverageDelay() const {
    if (delayHistory.empty()) return 0.0;
    double sum = 0;
    for (double d : delayHistory) sum += d;
    return sum / delayHistory.size();
}

void TorchEDRLAgent::saveModel(const std::string& path) {
    modelPath = path;
    
    if (enablePPO && ppoAgent) {
        ppoAgent->saveModel(path);
    }
    if (enableDDPG && ddpgAgent) {
        ddpgAgent->saveModel(path);
    }
    if (enableSAC && sacAgent) {
        sacAgent->saveModel(path);
    }
    
    isModelLoaded = true;
    EV << "Model saved to " << path << std::endl;
}

void TorchEDRLAgent::loadModel(const std::string& path) {
    modelPath = path;
    
    try {
        if (enablePPO && ppoAgent) {
            ppoAgent->loadModel(path);
        }
        if (enableDDPG && ddpgAgent) {
            ddpgAgent->loadModel(path);
        }
        if (enableSAC && sacAgent) {
            sacAgent->loadModel(path);
        }
        
        isModelLoaded = true;
        EV << "Model loaded from " << path << std::endl;
    } catch (const cRuntimeError& e) {
        EV << "Warning: Could not load model from " << path << ": " << e.what() << std::endl;
        isModelLoaded = false;
    }
}

void TorchEDRLAgent::printStatistics() {
    EV << "=== TorchEDRLAgent Statistics ===" << std::endl;
    EV << "  Average Reward: " << getAverageReward() << std::endl;
    EV << "  Average Satisfaction: " << getAverageSatisfaction() * 100 << "%" << std::endl;
    EV << "  Average Delay: " << getAverageDelay() * 1000 << " ms" << std::endl;
    EV << "  Epsilon: " << epsilon << std::endl;
    EV << "  Pending Decisions: " << pendingDecisions.size() << std::endl;
    EV << "  Decision History: " << decisionHistory.size() << std::endl;
    
    if (voting) {
        EV << "  Q-value Mean: " << voting->getMeanQ() << std::endl;
        EV << "  Q-value Std: " << voting->getStdQ() << std::endl;
    }
}

void TorchEDRLAgent::printState(const NetworkState& state) {
    EV << "Network State:" << std::endl;
    EV << "  Time Slot: " << state.currentTimeSlot << std::endl;
    EV << "  Queue Lengths: [";
    for (size_t i = 0; i < state.queueLengths.size(); i++) {
        if (i > 0) EV << ", ";
        EV << state.queueLengths[i];
    }
    EV << "]" << std::endl;
    EV << "  Avg Delay: " << state.avgDelay * 1000 << " ms" << std::endl;
    EV << "  Satisfaction: " << state.satisfactionRatio * 100 << "%" << std::endl;
    EV << "  Throughput: " << state.throughput / 1e6 << " Mbps" << std::endl;
}

void TorchEDRLAgent::logTrainingStep(int episode, double ppoReward, double ddpgReward, 
                                      double sacReward, double avgDelay, 
                                      double satisfaction, double eps) {
    TrainingLogEntry entry;
    entry.episode = episode;
    entry.ppoReward = ppoReward;
    entry.ddpgReward = ddpgReward;
    entry.sacReward = sacReward;
    entry.avgDelay = avgDelay;
    entry.satisfaction = satisfaction;
    entry.epsilon = eps;
    
    trainingLog.push_back(entry);
}

void TorchEDRLAgent::saveTrainingLog(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        EV << "Error: Could not open training log file: " << path << std::endl;
        return;
    }
    
    file << "episode,ppo_reward,ddpg_reward,sac_reward,avg_delay,satisfaction,epsilon,record_time\n";
    for (const auto& entry : trainingLog) {
        std::time_t now = std::time(nullptr);
        std::tm* tm_info = std::localtime(&now);
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tm_info);
        
        file << entry.episode << ","
             << entry.ppoReward << ","
             << entry.ddpgReward << ","
             << entry.sacReward << ","
             << entry.avgDelay << ","
             << entry.satisfaction << ","
             << entry.epsilon << ","
             << timeBuffer << "\n";
    }
    
    file.close();
    EV << "Training log saved to " << path << std::endl;
}
