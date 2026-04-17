//
// TorchNetworks.h - PyTorch Neural Network Module Definitions
// 论文: An Ensemble Deep Reinforcement Learning-Based Asynchronous Scheduling Mechanism for 5G-TSN
//

#ifndef TORCHNETWORKS_H_
#define TORCHNETWORKS_H_

#include <torch/torch.h>
#include <vector>
#include <string>

// 动作掩码结构 (论文公式11)
struct ActionMask {
    std::vector<bool> validActions;
    int numValidActions;
    
    ActionMask(int size) : validActions(size, true), numValidActions(size) {}
    
    void maskAction(int action) {
        if (action >= 0 && action < (int)validActions.size() && validActions[action]) {
            validActions[action] = false;
            numValidActions--;
        }
    }
    
    void unmaskAction(int action) {
        if (action >= 0 && action < (int)validActions.size() && !validActions[action]) {
            validActions[action] = true;
            numValidActions++;
        }
    }
    
    bool isValid(int action) const {
        return action >= 0 && action < (int)validActions.size() && validActions[action];
    }
    
    torch::Tensor applyMask(torch::Tensor& logits) const {
        auto maskedLogits = logits.clone();
        for (size_t i = 0; i < validActions.size(); i++) {
            if (!validActions[i]) {
                maskedLogits[i] = -1e9;
            }
        }
        return maskedLogits;
    }
};

// PPO Actor网络 (论文Section IV-C.1)
class PPOActorNetworkImpl : public torch::nn::Module {
public:
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc_out{nullptr};
    int actionSize;
    
    PPOActorNetworkImpl(int stateSize, int actionSize, int hiddenSize = 256) 
        : actionSize(actionSize) {
        fc1 = register_module("fc1", torch::nn::Linear(stateSize, hiddenSize));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenSize, hiddenSize));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenSize, hiddenSize));
        fc_out = register_module("fc_out", torch::nn::Linear(hiddenSize, actionSize));
        initWeights();
    }
    
    void initWeights() {
        for (auto& p : parameters()) {
            if (p.sizes().size() > 1) {
                torch::nn::init::xavier_uniform_(p);
            } else {
                torch::nn::init::zeros_(p);
            }
        }
    }
    
    torch::Tensor forward(torch::Tensor state, const ActionMask* mask = nullptr) {
        auto x = torch::relu(fc1->forward(state));
        x = torch::relu(fc2->forward(x));
        x = torch::relu(fc3->forward(x));
        auto logits = fc_out->forward(x);
        
        if (mask) {
            logits = mask->applyMask(logits);
        }
        
        return torch::softmax(logits, -1);
    }
};
TORCH_MODULE(PPOActorNetwork);

// PPO Critic网络
class PPOCriticNetworkImpl : public torch::nn::Module {
public:
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc_out{nullptr};
    
    PPOCriticNetworkImpl(int stateSize, int hiddenSize = 256) {
        fc1 = register_module("fc1", torch::nn::Linear(stateSize, hiddenSize));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenSize, hiddenSize));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenSize, hiddenSize));
        fc_out = register_module("fc_out", torch::nn::Linear(hiddenSize, 1));
        initWeights();
    }
    
    void initWeights() {
        for (auto& p : parameters()) {
            if (p.sizes().size() > 1) {
                torch::nn::init::xavier_uniform_(p);
            } else {
                torch::nn::init::zeros_(p);
            }
        }
    }
    
    torch::Tensor forward(torch::Tensor state) {
        auto x = torch::relu(fc1->forward(state));
        x = torch::relu(fc2->forward(x));
        x = torch::relu(fc3->forward(x));
        return fc_out->forward(x);
    }
};
TORCH_MODULE(PPOCriticNetwork);

// DDPG Actor网络 (论文Section IV-C.2)
class DDPGActorNetworkImpl : public torch::nn::Module {
public:
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc_out{nullptr};
    int actionSize;
    double noiseScale;
    
    DDPGActorNetworkImpl(int stateSize, int actionSize, int hiddenSize = 256) 
        : actionSize(actionSize), noiseScale(0.1) {
        fc1 = register_module("fc1", torch::nn::Linear(stateSize, hiddenSize));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenSize, hiddenSize));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenSize, hiddenSize));
        fc_out = register_module("fc_out", torch::nn::Linear(hiddenSize, actionSize));
        initWeights();
    }
    
    void initWeights() {
        for (auto& p : parameters()) {
            if (p.sizes().size() > 1) {
                torch::nn::init::uniform_(p, -3e-3, 3e-3);
            } else {
                torch::nn::init::zeros_(p);
            }
        }
    }
    
    torch::Tensor forward(torch::Tensor state, bool addNoise = false) {
        auto x = torch::relu(fc1->forward(state));
        x = torch::relu(fc2->forward(x));
        x = torch::relu(fc3->forward(x));
        auto action = torch::tanh(fc_out->forward(x));
        
        if (addNoise) {
            auto noise = torch::randn_like(action) * noiseScale;
            action = torch::clamp(action + noise, -1.0, 1.0);
        }
        
        return action;
    }
    
    void decayNoise(double decayRate) {
        noiseScale = std::max(0.01, noiseScale * decayRate);
    }
};
TORCH_MODULE(DDPGActorNetwork);

// DDPG Critic网络
class DDPGCriticNetworkImpl : public torch::nn::Module {
public:
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc_out{nullptr};
    
    DDPGCriticNetworkImpl(int stateSize, int actionSize, int hiddenSize = 256) {
        fc1 = register_module("fc1", torch::nn::Linear(stateSize + actionSize, hiddenSize));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenSize, hiddenSize));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenSize, hiddenSize));
        fc_out = register_module("fc_out", torch::nn::Linear(hiddenSize, 1));
        initWeights();
    }
    
    void initWeights() {
        for (auto& p : parameters()) {
            if (p.sizes().size() > 1) {
                torch::nn::init::uniform_(p, -3e-3, 3e-3);
            } else {
                torch::nn::init::zeros_(p);
            }
        }
    }
    
    torch::Tensor forward(torch::Tensor state, torch::Tensor action) {
        auto x = torch::cat({state, action}, -1);
        x = torch::relu(fc1->forward(x));
        x = torch::relu(fc2->forward(x));
        x = torch::relu(fc3->forward(x));
        return fc_out->forward(x);
    }
};
TORCH_MODULE(DDPGCriticNetwork);

// SAC Actor网络 (论文Section IV-C.3)
class SACActorNetworkImpl : public torch::nn::Module {
public:
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr};
    torch::nn::Linear mean_out{nullptr}, log_std_out{nullptr};
    int actionSize;
    double logStdMin, logStdMax;
    
    SACActorNetworkImpl(int stateSize, int actionSize, int hiddenSize = 256) 
        : actionSize(actionSize), logStdMin(-20), logStdMax(2) {
        fc1 = register_module("fc1", torch::nn::Linear(stateSize, hiddenSize));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenSize, hiddenSize));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenSize, hiddenSize));
        mean_out = register_module("mean_out", torch::nn::Linear(hiddenSize, actionSize));
        log_std_out = register_module("log_std_out", torch::nn::Linear(hiddenSize, actionSize));
        initWeights();
    }
    
    void initWeights() {
        for (auto& p : parameters()) {
            if (p.sizes().size() > 1) {
                torch::nn::init::xavier_uniform_(p);
            } else {
                torch::nn::init::zeros_(p);
            }
        }
    }
    
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> forward(torch::Tensor state, 
                                                                    bool sample = true,
                                                                    double epsilon = 1e-6) {
        auto x = torch::relu(fc1->forward(state));
        x = torch::relu(fc2->forward(x));
        x = torch::relu(fc3->forward(x));
        
        auto mean = mean_out->forward(x);
        auto logStd = torch::clamp(log_std_out->forward(x), logStdMin, logStdMax);
        auto std = torch::exp(logStd);
        
        torch::Tensor action, logProb;
        
        if (sample) {
            auto normal = torch::randn_like(mean);
            auto x_t = mean + std * normal;
            action = torch::tanh(x_t);
            logProb = torch::log(std * (1 - torch::tanh(x_t).pow(2) + epsilon));
            logProb = logProb.sum(-1, true);
        } else {
            action = torch::tanh(mean);
            logProb = torch::zeros({state.size(0), 1});
        }
        
        return std::make_tuple(action, logProb, mean);
    }
};
TORCH_MODULE(SACActorNetwork);

// SAC Critic网络 (双Q网络)
class SACCriticNetworkImpl : public torch::nn::Module {
public:
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc_out{nullptr};
    
    SACCriticNetworkImpl(int stateSize, int actionSize, int hiddenSize = 256) {
        fc1 = register_module("fc1", torch::nn::Linear(stateSize + actionSize, hiddenSize));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenSize, hiddenSize));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenSize, hiddenSize));
        fc_out = register_module("fc_out", torch::nn::Linear(hiddenSize, 1));
        initWeights();
    }
    
    void initWeights() {
        for (auto& p : parameters()) {
            if (p.sizes().size() > 1) {
                torch::nn::init::xavier_uniform_(p);
            } else {
                torch::nn::init::zeros_(p);
            }
        }
    }
    
    torch::Tensor forward(torch::Tensor state, torch::Tensor action) {
        auto x = torch::cat({state, action}, -1);
        x = torch::relu(fc1->forward(x));
        x = torch::relu(fc2->forward(x));
        x = torch::relu(fc3->forward(x));
        return fc_out->forward(x);
    }
};
TORCH_MODULE(SACCriticNetwork);

#endif
