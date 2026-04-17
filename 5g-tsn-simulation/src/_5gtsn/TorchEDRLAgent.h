//
// TorchEDRLAgent.h - PyTorch-based EDRL Agent for TSN Scheduling
// 论文: An Ensemble Deep Reinforcement Learning-Based Asynchronous Scheduling Mechanism for 5G-TSN
// 完整实现PPO + DDPG + SAC三代理集成 + Q值投票机制
//

#ifndef TORCHEDRLAGENT_H_
#define TORCHEDRLAGENT_H_

#include <omnetpp.h>
// #include <torch/torch.h>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>
#include <fstream>
#include "TorchNetworks.h"

using namespace omnetpp;

class FlowPacket;

// ==================== 基础数据结构 ====================

struct NetworkState {
    int currentTimeSlot;
    std::vector<int> queueLengths;
    std::vector<std::vector<int>> slotOccupancy;
    double avgDelay;
    double satisfactionRatio;
    double throughput;
    double timestamp;
    
    NetworkState() : currentTimeSlot(0), avgDelay(0), satisfactionRatio(0), throughput(0), timestamp(0) {}
};

struct SchedulingDecision {
    int flowId;
    int assignedSlot;
    int assignedQueue;
    int assignedSwitch;
    int assignedPort;
    double confidence;
    double qValue;
    double logProb;
    double decisionTime;
    
    SchedulingDecision() : flowId(-1), assignedSlot(-1), assignedQueue(-1), 
                           assignedSwitch(-1), assignedPort(-1), confidence(0), 
                           qValue(0), logProb(0), decisionTime(0) {}
};

struct Reward {
    double delayReward;
    double satisfactionReward;
    double efficiencyReward;
    double penalty;
    double total;
    
    Reward() : delayReward(0), satisfactionReward(0), efficiencyReward(0), penalty(0), total(0) {}
};

struct Experience {
    NetworkState state;
    SchedulingDecision action;
    Reward reward;
    NetworkState nextState;
    bool done;
};

struct GCLMatrix {
    int numSlots;
    int numQueues;
    std::vector<std::vector<int>> gates;
    
    GCLMatrix(int slots = 1024, int queues = 8) : numSlots(slots), numQueues(queues) {
        gates.resize(slots, std::vector<int>(queues, 0));
    }
    
    void setGate(int slot, int queue, int value) {
        if (slot >= 0 && slot < numSlots && queue >= 0 && queue < numQueues) {
            gates[slot][queue] = value;
        }
    }
    
    int getGate(int slot, int queue) const {
        if (slot >= 0 && slot < numSlots && queue >= 0 && queue < numQueues) {
            return gates[slot][queue];
        }
        return 0;
    }
};

// ==================== 经验回放缓冲区 (论文Section IV-D) ====================

class TorchReplayBuffer {
private:
    std::deque<std::tuple<torch::Tensor, int, double, torch::Tensor, bool>> buffer;
    std::vector<double> priorities;
    int capacity;
    double alpha;
    double beta;
    double betaIncrement;
    std::mt19937 rng;
    
public:
    TorchReplayBuffer(int cap, double a = 0.6, double b = 0.4)
        : capacity(cap), alpha(a), beta(b), betaIncrement(0.001) {}
    
    void push(const torch::Tensor& state, int action, double reward, 
              const torch::Tensor& nextState, bool done, double priority = 1.0) {
        if ((int)buffer.size() >= capacity) {
            buffer.pop_front();
            priorities.erase(priorities.begin());
        }
        buffer.push_back(std::make_tuple(state, action, reward, nextState, done));
        priorities.push_back(std::pow(priority + 1e-6, alpha));
    }
    
    bool sampleBatch(int batchSize,
                     std::vector<torch::Tensor>& states,
                     std::vector<int>& actions,
                     std::vector<double>& rewards,
                     std::vector<torch::Tensor>& nextStates,
                     std::vector<bool>& dones,
                     std::vector<int>& indices,
                     std::vector<double>& weights) {
        if ((int)buffer.size() < batchSize) return false;
        
        double sum = std::accumulate(priorities.begin(), priorities.end(), 0.0);
        std::discrete_distribution<int> dist(priorities.begin(), priorities.end());
        
        for (int i = 0; i < batchSize; i++) {
            int idx = dist(rng);
            indices.push_back(idx);
            states.push_back(std::get<0>(buffer[idx]));
            actions.push_back(std::get<1>(buffer[idx]));
            rewards.push_back(std::get<2>(buffer[idx]));
            nextStates.push_back(std::get<3>(buffer[idx]));
            dones.push_back(std::get<4>(buffer[idx]));
            
            double weight = std::pow(buffer.size() * priorities[idx] / sum, -beta);
            weights.push_back(weight);
        }
        
        beta = std::min(1.0, beta + betaIncrement);
        return true;
    }
    
    void updatePriorities(const std::vector<int>& indices, const std::vector<double>& tdErrors) {
        for (size_t i = 0; i < indices.size() && i < tdErrors.size(); i++) {
            if (indices[i] >= 0 && indices[i] < (int)priorities.size()) {
                priorities[indices[i]] = std::pow(std::abs(tdErrors[i]) + 1e-6, alpha);
            }
        }
    }
    
    int size() const { return buffer.size(); }
    bool isReady(int batchSize) const { return (int)buffer.size() >= batchSize; }
    void clear() { buffer.clear(); priorities.clear(); }
};

// ==================== 论文公式(18-20): Q值投票机制 ====================

class QValueVotingMechanism {
private:
    std::deque<double> qValueHistory;
    double meanQ;
    double stdQ;
    double temperature;
    int historySize;
    
public:
    QValueVotingMechanism(double temp = 1.0, int histSize = 1000) 
        : meanQ(0), stdQ(1), temperature(temp), historySize(histSize) {}
    
    void updateStatistics() {
        if (qValueHistory.empty()) return;
        
        meanQ = 0;
        for (double q : qValueHistory) meanQ += q;
        meanQ /= qValueHistory.size();
        
        stdQ = 0;
        for (double q : qValueHistory) {
            stdQ += (q - meanQ) * (q - meanQ);
        }
        stdQ = std::sqrt(stdQ / qValueHistory.size() + 1e-8);
    }
    
    double computeAdaptiveThreshold(double qMax) {
        return (meanQ - stdQ) / (qMax + 1e-8);
    }
    
    std::vector<double> computeSelectionProbabilities(const std::vector<double>& qValues) {
        if (qValues.empty()) return {};
        
        double qMax = *std::max_element(qValues.begin(), qValues.end());
        double threshold = computeAdaptiveThreshold(qMax);
        
        std::vector<double> filteredQ;
        std::vector<int> filteredIndices;
        
        for (size_t i = 0; i < qValues.size(); i++) {
            double normQ = (qValues[i] - meanQ) / stdQ;
            if (normQ >= threshold) {
                filteredQ.push_back(qValues[i]);
                filteredIndices.push_back(i);
            }
        }
        
        if (filteredQ.empty()) {
            return std::vector<double>(qValues.size(), 1.0 / qValues.size());
        }
        
        std::vector<double> probs(filteredQ.size());
        double sumExp = 0;
        
        for (size_t i = 0; i < filteredQ.size(); i++) {
            probs[i] = std::exp(filteredQ[i] / temperature);
            sumExp += probs[i];
        }
        
        for (auto& p : probs) p /= sumExp;
        
        std::vector<double> finalProbs(qValues.size(), 0);
        for (size_t i = 0; i < filteredIndices.size(); i++) {
            finalProbs[filteredIndices[i]] = probs[i];
        }
        
        return finalProbs;
    }
    
    int selectAction(const std::vector<double>& qValues, std::mt19937& rng) {
        qValueHistory.push_back(std::accumulate(qValues.begin(), qValues.end(), 0.0) / qValues.size());
        if ((int)qValueHistory.size() > historySize) {
            qValueHistory.pop_front();
        }
        updateStatistics();
        
        auto probs = computeSelectionProbabilities(qValues);
        
        std::discrete_distribution<int> dist(probs.begin(), probs.end());
        return dist(rng);
    }
    
    void setTemperature(double temp) { temperature = temp; }
    double getMeanQ() const { return meanQ; }
    double getStdQ() const { return stdQ; }
};

// ==================== 论文公式(10): 高斯噪声探索 ====================

class GaussianNoiseExplorer {
private:
    double mean;
    double std;
    double stdDecay;
    double stdMin;
    std::mt19937 rng;
    
public:
    GaussianNoiseExplorer(double initialStd = 0.3, double decay = 0.995, double minStd = 0.01)
        : mean(0), std(initialStd), stdDecay(decay), stdMin(minStd) {}
    
    double addNoise(double value) {
        std::normal_distribution<double> dist(mean, std);
        return std::clamp(value + dist(rng), -1.0, 1.0);
    }
    
    void decay() {
        std = std::max(stdMin, std * stdDecay);
    }
    
    double getStd() const { return std; }
};

// ==================== PPO Agent (论文Section IV-C.1) ====================

class TorchPPOAgent {
private:
    PPOActorNetwork actor = nullptr;
    PPOCriticNetwork critic = nullptr;
    PPOActorNetwork actorOld = nullptr;
    
    std::unique_ptr<torch::optim::Adam> actorOptimizer;
    std::unique_ptr<torch::optim::Adam> criticOptimizer;
    
    int stateSize;
    int actionSize;
    double learningRate;
    double gamma;
    double lambda;
    double clipEpsilon;
    double entropyCoef;
    int epochs;
    
    std::vector<std::tuple<torch::Tensor, int, double, torch::Tensor, bool, double>> trajectory;
    std::mt19937 rng;
    
public:
    TorchPPOAgent(int stateSz, int actionSz, double lr = 0.0003)
        : stateSize(stateSz), actionSize(actionSz), learningRate(lr),
          gamma(0.99), lambda(0.95), clipEpsilon(0.2), entropyCoef(0.01), epochs(10) {
        
        actor = PPOActorNetwork(stateSize, actionSize);
        critic = PPOCriticNetwork(stateSize);
        actorOld = PPOActorNetwork(stateSize, actionSize);
        
        actorOptimizer = std::make_unique<torch::optim::Adam>(actor->parameters(), lr);
        criticOptimizer = std::make_unique<torch::optim::Adam>(critic->parameters(), lr);
        
        copyActorToOld();
    }
    
    void copyActorToOld() {
        torch::NoGradGuard noGrad;
        for (size_t i = 0; i < actor->parameters().size(); i++) {
            actorOld->parameters()[i].copy_(actor->parameters()[i]);
        }
    }
    
    std::pair<int, double> selectAction(const torch::Tensor& state, bool explore = true, 
                                         const ActionMask* mask = nullptr) {
        torch::NoGradGuard noGrad;
        auto probs = actor->forward(state, mask);
        
        int action;
        double logProb;
        
        if (explore) {
            std::vector<double> probVec(probs.data_ptr<float>(), 
                                        probs.data_ptr<float>() + probs.numel());
            std::discrete_distribution<int> dist(probVec.begin(), probVec.end());
            action = dist(rng);
        } else {
            action = probs.argmax().item<int>();
        }
        
        logProb = std::log(probs[action].item<float>() + 1e-8);
        
        return std::make_pair(action, logProb);
    }
    
    double getQValue(const torch::Tensor& state) {
        torch::NoGradGuard noGrad;
        return critic->forward(state).item<double>();
    }
    
    void storeTransition(const torch::Tensor& state, int action, double logProb,
                         double reward, const torch::Tensor& nextState, bool done) {
        trajectory.push_back(std::make_tuple(state, action, reward, nextState, done, logProb));
    }
    
    void train() {
        if ((int)trajectory.size() < 32) return;
        
        std::vector<torch::Tensor> states, nextStates;
        std::vector<int> actions;
        std::vector<double> rewards, logProbs;
        std::vector<bool> dones;
        
        for (const auto& t : trajectory) {
            states.push_back(std::get<0>(t));
            actions.push_back(std::get<1>(t));
            rewards.push_back(std::get<2>(t));
            nextStates.push_back(std::get<3>(t));
            dones.push_back(std::get<4>(t));
            logProbs.push_back(std::get<5>(t));
        }
        
        std::vector<double> values, nextValues, advantages, returns;
        
        for (size_t i = 0; i < states.size(); i++) {
            values.push_back(critic->forward(states[i]).item<double>());
            nextValues.push_back(critic->forward(nextStates[i]).item<double>());
        }
        
        double gae = 0;
        for (int i = rewards.size() - 1; i >= 0; i--) {
            double delta = rewards[i] + gamma * nextValues[i] * (1 - dones[i]) - values[i];
            gae = delta + gamma * lambda * gae;
            advantages.insert(advantages.begin(), gae);
            returns.insert(returns.begin(), gae + values[i]);
        }
        
        double meanAdv = std::accumulate(advantages.begin(), advantages.end(), 0.0) / advantages.size();
        double stdAdv = 0;
        for (double a : advantages) stdAdv += (a - meanAdv) * (a - meanAdv);
        stdAdv = std::sqrt(stdAdv / advantages.size() + 1e-8);
        
        for (int epoch = 0; epoch < epochs; epoch++) {
            for (size_t i = 0; i < states.size(); i++) {
                actorOptimizer->zero_grad();
                criticOptimizer->zero_grad();
                
                auto newProbs = actor->forward(states[i]);
                auto oldProbs = actorOld->forward(states[i]);
                
                double ratio = newProbs[actions[i]].item<double>() / 
                              (oldProbs[actions[i]].item<double>() + 1e-8);
                
                double advantage = (advantages[i] - meanAdv) / stdAdv;
                double clippedRatio = std::clamp(ratio, 1.0 - clipEpsilon, 1.0 + clipEpsilon);
                double actorLoss = -std::min(ratio * advantage, clippedRatio * advantage);
                
                double entropy = 0;
                for (int j = 0; j < newProbs.size(0); j++) {
                    double p = newProbs[j].item<double>();
                    if (p > 1e-8) entropy -= p * std::log(p);
                }
                
                auto actorLossTensor = torch::tensor(actorLoss - entropyCoef * entropy, torch::requires_grad());
                actorLossTensor.backward();
                actorOptimizer->step();
                
                auto valuePred = critic->forward(states[i]);
                auto valueTarget = torch::tensor(returns[i]);
                auto criticLoss = torch::mse_loss(valuePred, valueTarget);
                criticLoss.backward();
                criticOptimizer->step();
            }
        }
        
        copyActorToOld();
        trajectory.clear();
    }
    
    void saveModel(const std::string& path) {
        torch::save(actor, path + "_ppo_actor.pt");
        torch::save(critic, path + "_ppo_critic.pt");
    }
    
    void loadModel(const std::string& path) {
        torch::load(actor, path + "_ppo_actor.pt");
        torch::load(critic, path + "_ppo_critic.pt");
        copyActorToOld();
    }
};

// ==================== DDPG Agent (论文Section IV-C.2) ====================

class TorchDDPGAgent {
private:
    DDPGActorNetwork actor = nullptr;
    DDPGCriticNetwork critic = nullptr;
    DDPGActorNetwork targetActor = nullptr;
    DDPGCriticNetwork targetCritic = nullptr;
    
    std::unique_ptr<torch::optim::Adam> actorOptimizer;
    std::unique_ptr<torch::optim::Adam> criticOptimizer;
    
    TorchReplayBuffer replayBuffer;
    GaussianNoiseExplorer explorer;
    
    int stateSize;
    int actionSize;
    double learningRate;
    double gamma;
    double tau;
    int batchSize;
    
    std::mt19937 rng;
    
    void softUpdateActor() {
        torch::NoGradGuard noGrad;
        for (size_t i = 0; i < targetActor->parameters().size(); i++) {
            targetActor->parameters()[i].mul_(1 - tau).add_(actor->parameters()[i], tau);
        }
    }
    
    void softUpdateCritic() {
        torch::NoGradGuard noGrad;
        for (size_t i = 0; i < targetCritic->parameters().size(); i++) {
            targetCritic->parameters()[i].mul_(1 - tau).add_(critic->parameters()[i], tau);
        }
    }
    
public:
    TorchDDPGAgent(int stateSz, int actionSz, double lr = 0.0001, int bufferSize = 10000)
        : stateSize(stateSz), actionSize(actionSz), learningRate(lr),
          gamma(0.99), tau(0.005), batchSize(32), replayBuffer(bufferSize) {
        
        actor = DDPGActorNetwork(stateSize, actionSize);
        critic = DDPGCriticNetwork(stateSize, actionSize);
        targetActor = DDPGActorNetwork(stateSize, actionSize);
        targetCritic = DDPGCriticNetwork(stateSize, actionSize);
        
        actorOptimizer = std::make_unique<torch::optim::Adam>(actor->parameters(), lr);
        criticOptimizer = std::make_unique<torch::optim::Adam>(critic->parameters(), lr);
        
        torch::NoGradGuard noGrad;
        for (size_t i = 0; i < actor->parameters().size(); i++) {
            targetActor->parameters()[i].copy_(actor->parameters()[i]);
        }
        for (size_t i = 0; i < critic->parameters().size(); i++) {
            targetCritic->parameters()[i].copy_(critic->parameters()[i]);
        }
    }
    
    std::pair<int, double> selectAction(const torch::Tensor& state, bool explore = true) {
        torch::NoGradGuard noGrad;
        auto action = actor->forward(state, explore);
        
        if (explore) {
            explorer.decay();
        }
        
        int actionIdx = action.argmax().item<int>();
        auto actionTensor = torch::zeros({actionSize});
        actionTensor[actionIdx] = 1.0;
        double qValue = critic->forward(state, actionTensor).item<double>();
        
        return std::make_pair(actionIdx, qValue);
    }
    
    double getQValue(const torch::Tensor& state, int actionIdx) {
        torch::NoGradGuard noGrad;
        auto action = torch::zeros({actionSize});
        action[actionIdx] = 1.0;
        return critic->forward(state, action).item<double>();
    }
    
    void storeTransition(const torch::Tensor& state, int action, double reward,
                         const torch::Tensor& nextState, bool done) {
        replayBuffer.push(state, action, reward, nextState, done);
    }
    
    void train() {
        std::vector<torch::Tensor> states, nextStates;
        std::vector<int> actions;
        std::vector<double> rewards, weights;
        std::vector<bool> dones;
        std::vector<int> indices;
        
        if (!replayBuffer.sampleBatch(batchSize, states, actions, rewards, nextStates, 
                                       dones, indices, weights)) return;
        
        std::vector<double> tdErrors;
        
        for (size_t i = 0; i < states.size(); i++) {
            criticOptimizer->zero_grad();
            actorOptimizer->zero_grad();
            
            torch::NoGradGuard noGrad;
            auto nextAction = targetActor->forward(nextStates[i]);
            auto targetQ = targetCritic->forward(nextStates[i], nextAction);
            double targetValue = rewards[i] + gamma * targetQ.item<double>() * (1 - dones[i]);
            
            auto actionTensor = torch::zeros({actionSize});
            actionTensor[actions[i]] = 1.0;
            auto currentQ = critic->forward(states[i], actionTensor);
            auto criticLoss = torch::mse_loss(currentQ, torch::tensor(targetValue));
            criticLoss.backward();
            criticOptimizer->step();
            
            auto actorLoss = -critic->forward(states[i], actor->forward(states[i]));
            actorLoss.backward();
            actorOptimizer->step();
            
            tdErrors.push_back(std::abs(targetValue - currentQ.item<double>()));
        }
        
        replayBuffer.updatePriorities(indices, tdErrors);
        
        softUpdateActor();
        softUpdateCritic();
    }
    
    void saveModel(const std::string& path) {
        torch::save(actor, path + "_ddpg_actor.pt");
        torch::save(critic, path + "_ddpg_critic.pt");
    }
    
    void loadModel(const std::string& path) {
        torch::load(actor, path + "_ddpg_actor.pt");
        torch::load(critic, path + "_ddpg_critic.pt");
    }
};

// ==================== SAC Agent (论文Section IV-C.3) ====================

class TorchSACAgent {
private:
    SACActorNetwork actor = nullptr;
    SACCriticNetwork critic1 = nullptr;
    SACCriticNetwork critic2 = nullptr;
    SACCriticNetwork targetCritic1 = nullptr;
    SACCriticNetwork targetCritic2 = nullptr;
    
    std::unique_ptr<torch::optim::Adam> actorOptimizer;
    std::unique_ptr<torch::optim::Adam> critic1Optimizer;
    std::unique_ptr<torch::optim::Adam> critic2Optimizer;
    
    double logAlpha;
    double targetEntropy;
    torch::Tensor logAlphaTensor;
    std::unique_ptr<torch::optim::Adam> alphaOptimizer;
    
    TorchReplayBuffer replayBuffer;
    
    int stateSize;
    int actionSize;
    double learningRate;
    double gamma;
    double tau;
    int batchSize;
    
    std::mt19937 rng;
    
    void softUpdateCritic(SACCriticNetwork& target, SACCriticNetwork& source) {
        torch::NoGradGuard noGrad;
        for (size_t i = 0; i < target->parameters().size(); i++) {
            target->parameters()[i].mul_(1 - tau).add_(source->parameters()[i], tau);
        }
    }
    
public:
    TorchSACAgent(int stateSz, int actionSz, double lr = 0.0003, int bufferSize = 10000)
        : stateSize(stateSz), actionSize(actionSz), learningRate(lr),
          gamma(0.99), tau(0.005), batchSize(32), replayBuffer(bufferSize) {
        
        actor = SACActorNetwork(stateSize, actionSize);
        critic1 = SACCriticNetwork(stateSize, actionSize);
        critic2 = SACCriticNetwork(stateSize, actionSize);
        targetCritic1 = SACCriticNetwork(stateSize, actionSize);
        targetCritic2 = SACCriticNetwork(stateSize, actionSize);
        
        actorOptimizer = std::make_unique<torch::optim::Adam>(actor->parameters(), lr);
        critic1Optimizer = std::make_unique<torch::optim::Adam>(critic1->parameters(), lr);
        critic2Optimizer = std::make_unique<torch::optim::Adam>(critic2->parameters(), lr);
        
        logAlpha = std::log(0.2);
        targetEntropy = -actionSize;
        logAlphaTensor = torch::tensor(logAlpha, torch::requires_grad());
        std::vector<torch::Tensor> alphaParams = {logAlphaTensor};
        alphaOptimizer = std::make_unique<torch::optim::Adam>(alphaParams, lr);
        
        torch::NoGradGuard noGrad;
        for (size_t i = 0; i < critic1->parameters().size(); i++) {
            targetCritic1->parameters()[i].copy_(critic1->parameters()[i]);
        }
        for (size_t i = 0; i < critic2->parameters().size(); i++) {
            targetCritic2->parameters()[i].copy_(critic2->parameters()[i]);
        }
    }
    
    std::pair<int, double> selectAction(const torch::Tensor& state, bool explore = true) {
        torch::NoGradGuard noGrad;
        auto [action, logProb, mean] = actor->forward(state, explore);
        
        int actionIdx = action.argmax().item<int>();
        
        auto q1 = critic1->forward(state, action);
        auto q2 = critic2->forward(state, action);
        double qValue = std::min(q1.item<double>(), q2.item<double>());
        
        return std::make_pair(actionIdx, qValue);
    }
    
    double getQValue(const torch::Tensor& state, int actionIdx) {
        torch::NoGradGuard noGrad;
        auto action = torch::zeros({actionSize});
        action[actionIdx] = 1.0;
        auto q1 = critic1->forward(state, action);
        auto q2 = critic2->forward(state, action);
        return std::min(q1.item<double>(), q2.item<double>());
    }
    
    double getAlpha() const { return std::exp(logAlphaTensor.item<double>()); }
    
    void storeTransition(const torch::Tensor& state, int action, double reward,
                         const torch::Tensor& nextState, bool done) {
        replayBuffer.push(state, action, reward, nextState, done);
    }
    
    void train() {
        std::vector<torch::Tensor> states, nextStates;
        std::vector<int> actions;
        std::vector<double> rewards, weights;
        std::vector<bool> dones;
        std::vector<int> indices;
        
        if (!replayBuffer.sampleBatch(batchSize, states, actions, rewards, nextStates, 
                                       dones, indices, weights)) return;
        
        double alpha = getAlpha();
        
        for (size_t i = 0; i < states.size(); i++) {
            critic1Optimizer->zero_grad();
            critic2Optimizer->zero_grad();
            actorOptimizer->zero_grad();
            
            auto [newAction, logProb, mean] = actor->forward(states[i], true);
            
            torch::NoGradGuard noGrad;
            auto [nextAction, nextLogProb, nextMean] = actor->forward(nextStates[i], true);
            auto q1Target = targetCritic1->forward(nextStates[i], nextAction);
            auto q2Target = targetCritic2->forward(nextStates[i], nextAction);
            auto qTarget = torch::min(q1Target, q2Target) - alpha * nextLogProb;
            double targetValue = rewards[i] + gamma * qTarget.item<double>() * (1 - dones[i]);
            
            auto actionTensor = torch::zeros({actionSize});
            actionTensor[actions[i]] = 1.0;
            auto q1 = critic1->forward(states[i], actionTensor);
            auto q2 = critic2->forward(states[i], actionTensor);
            auto critic1Loss = torch::mse_loss(q1, torch::tensor(targetValue));
            auto critic2Loss = torch::mse_loss(q2, torch::tensor(targetValue));
            
            critic1Loss.backward();
            critic2Loss.backward();
            critic1Optimizer->step();
            critic2Optimizer->step();
            
            auto q1New = critic1->forward(states[i], newAction);
            auto q2New = critic2->forward(states[i], newAction);
            auto qNew = torch::min(q1New, q2New);
            auto actorLoss = (alpha * logProb - qNew).mean();
            actorLoss.backward();
            actorOptimizer->step();
            
            alphaOptimizer->zero_grad();
            auto alphaLoss = -(logAlphaTensor * (logProb + targetEntropy).detach()).mean();
            alphaLoss.backward();
            alphaOptimizer->step();
        }
        
        softUpdateCritic(targetCritic1, critic1);
        softUpdateCritic(targetCritic2, critic2);
    }
    
    void saveModel(const std::string& path) {
        torch::save(actor, path + "_sac_actor.pt");
        torch::save(critic1, path + "_sac_critic1.pt");
        torch::save(critic2, path + "_sac_critic2.pt");
    }
    
    void loadModel(const std::string& path) {
        torch::load(actor, path + "_sac_actor.pt");
        torch::load(critic1, path + "_sac_critic1.pt");
        torch::load(critic2, path + "_sac_critic2.pt");
    }
};

// ==================== EDRL Ensemble Agent ====================

class TorchEDRLAgent {
private:
    std::unique_ptr<TorchPPOAgent> ppoAgent;
    std::unique_ptr<TorchDDPGAgent> ddpgAgent;
    std::unique_ptr<TorchSACAgent> sacAgent;
    std::unique_ptr<QValueVotingMechanism> voting;
    
    NetworkState currentState;
    std::deque<NetworkState> stateHistory;
    int stateHistorySize;
    
    std::map<int, SchedulingDecision> pendingDecisions;
    std::deque<SchedulingDecision> decisionHistory;
    int decisionHistorySize;
    
    int stateSize;
    int actionSize;
    double learningRate;
    double gamma;
    double epsilon;
    double epsilonDecay;
    double epsilonMin;
    int batchSize;
    
    std::deque<double> rewardHistory;
    std::deque<double> satisfactionHistory;
    std::deque<double> delayHistory;
    
    struct TrainingLogEntry {
        int episode;
        double ppoReward;
        double ddpgReward;
        double sacReward;
        double avgDelay;
        double satisfaction;
        double epsilon;
    };
    std::vector<TrainingLogEntry> trainingLog;
    
    int numTimeSlots;
    int numQueues;
    int numSwitches;
    double timeSlotSize;
    double hyperPeriod;
    
    std::mt19937 rng;
    int flowCounter;
    
    bool enablePPO;
    bool enableDDPG;
    bool enableSAC;
    bool enableVoting;
    
    bool isTrainingMode;
    bool isModelLoaded;
    std::string modelPath;
    
    GCLMatrix currentGCL;
    
    torch::Tensor encodeState(const NetworkState& state);
    double computeReward(const NetworkState& state, const SchedulingDecision& decision, 
                         const NetworkState& nextState);
    void updateEpsilon();
    
public:
    TorchEDRLAgent();
    ~TorchEDRLAgent();
    
    void initialize(int numTimeSlots, int numQueues, int numSwitches, 
                   double timeSlotSize, double hyperPeriod,
                   double learningRate = 0.001, int batchSize = 32);
    
    SchedulingDecision makeDecision(const NetworkState& state, const FlowPacket* flow, 
                                     const ActionMask* mask = nullptr);
    SchedulingDecision makeDecisionPPO(const NetworkState& state, const ActionMask* mask = nullptr);
    SchedulingDecision makeDecisionDDPG(const NetworkState& state);
    SchedulingDecision makeDecisionSAC(const NetworkState& state);
    
    SchedulingDecision ensembleDecision(
        const SchedulingDecision& ppo, double ppoQ,
        const SchedulingDecision& ddpg, double ddpgQ,
        const SchedulingDecision& sac, double sacQ
    );
    
    void receiveFeedback(int flowId, double actualDelay, bool satisfied);
    void storeExperience(const Experience& exp);
    
    void updateNetworkState(const NetworkState& state);
    void updateQueueLengths(const std::vector<int>& lengths);
    void updateSlotOccupancy(const std::vector<std::vector<int>>& occupancy);
    
    void train();
    void trainPPO();
    void trainDDPG();
    void trainSAC();
    
    GCLMatrix generateGCL(const NetworkState& state);
    void updateGCL(const GCLMatrix& newGCL);
    const GCLMatrix& getCurrentGCL() const { return currentGCL; }
    
    double getAverageReward() const;
    double getAverageSatisfaction() const;
    double getAverageDelay() const;
    double getEpsilon() const { return epsilon; }
    
    void setEnablePPO(bool enable) { enablePPO = enable; }
    void setEnableDDPG(bool enable) { enableDDPG = enable; }
    void setEnableSAC(bool enable) { enableSAC = enable; }
    void setEnableVoting(bool enable) { enableVoting = enable; }
    
    void setTrainingMode(bool training) { isTrainingMode = training; }
    bool isTraining() const { return isTrainingMode; }
    bool isModelReady() const { return isModelLoaded; }
    
    void saveModel(const std::string& path);
    void loadModel(const std::string& path);
    
    void printStatistics();
    void printState(const NetworkState& state);
    
    void logTrainingStep(int episode, double ppoReward, double ddpgReward, double sacReward, 
                         double avgDelay, double satisfaction, double epsilon);
    void saveTrainingLog(const std::string& path);
};

#endif
