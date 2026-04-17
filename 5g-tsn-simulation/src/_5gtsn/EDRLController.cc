#include "EDRLController.h"
#include "TorchEDRLAgent.h"
#include "FlowPacket_m.h"

Define_Module(EDRLController);

EDRLController::EDRLController() : edrlAgent(nullptr), trainingTimer(nullptr) {}

EDRLController::~EDRLController() {
    cancelAndDelete(trainingTimer);
    delete edrlAgent;
}

void EDRLController::initialize() {
    switchId = par("switchId");
    enableTraining = par("enableTraining");
    trainingInterval = par("trainingInterval");
    modelPath = par("modelPath").stdstringValue();
    loadModelOnStart = par("loadModelOnStart");
    
    edrlAgent = new TorchEDRLAgent();
    edrlAgent->initialize(1024, 8, 3, 0.000015625, 0.016);
    edrlAgent->setTrainingMode(enableTraining);
    
    if (loadModelOnStart && !modelPath.empty()) {
        edrlAgent->loadModel(modelPath);
        EV << "EDRLController loaded model from " << modelPath << std::endl;
    }
    
    if (enableTraining) {
        trainingTimer = new cMessage("TrainingTimer");
        scheduleAt(simTime() + trainingInterval, trainingTimer);
    }
    
    EV << "EDRLController initialized for switch " << switchId << std::endl;
}

void EDRLController::handleMessage(cMessage* msg) {
    if (msg == trainingTimer) {
        if (enableTraining && edrlAgent) {
            edrlAgent->train();
            EV << "EDRLController training step completed" << std::endl;
        }
        scheduleAt(simTime() + trainingInterval, trainingTimer);
    } else {
        delete msg;
    }
}

void EDRLController::finish() {
    if (edrlAgent && !modelPath.empty()) {
        edrlAgent->saveModel(modelPath);
        EV << "EDRLController saved model to " << modelPath << std::endl;
    }
}
