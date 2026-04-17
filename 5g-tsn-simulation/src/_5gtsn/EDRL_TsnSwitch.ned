package src._5gtsn;

import src._5gtsn.TSNSwitch;

module EDRL_TsnSwitch extends TSNSwitch
{
    parameters:
        int bridgeId = default(0);
        bool enableEDRL = default(true);
        bool enableOAFS = default(true);
        bool enableTimeAwareShaper = default(true);
        bool enableGCLRecording = default(true);
        bool enableTrafficRecording = default(true);
        double gclUpdateInterval @unit(s) = default(0.000015625s);
        double trainingInterval @unit(s) = default(0.1s);
        int gclRecordInterval = default(10);
        int trafficRecordLimit = default(100000);
        string modelSavePath = default("simulations/");
        bool loadModelOnStart = default(false);
        string modelLoadPath = default("simulations/edrl_model.txt");
        
        @class(EDRL_TsnSwitch);
        @display("i=device/switch");
        
    submodules:
        edrlController: EDRLController {
            @display("p=300,100");
            switchId = default(0);
            enableTraining = default(true);
            trainingInterval = default(0.1s);
            modelPath = default("simulations/edrl_model.txt");
            loadModelOnStart = default(false);
        }
        oafsScheduler: OAFSScheduler {
            @display("p=500,100");
            numTimeSlots = default(1024);
            numQueues = default(8);
            timeSlotSize = default(0.000015625s);
        }
        gclRecorder: GCLRecorder {
            @display("p=700,100");
            outputPath = default("simulations/gcl_changes.csv");
            recordInterval = default(10);
        }
}

simple EDRLController
{
    parameters:
        int switchId = default(0);
        bool enableTraining = default(true);
        double trainingInterval @unit(s) = default(0.1s);
        string modelPath = default("simulations/edrl_model.txt");
        bool loadModelOnStart = default(false);
        
        @class(EDRLController);
        @display("i=block/control");
}

simple OAFSScheduler
{
    parameters:
        int numTimeSlots = default(1024);
        int numQueues = default(8);
        double timeSlotSize @unit(s) = default(0.000015625s);
        
        @class(OAFSScheduler);
        @display("i=block/scheduler");
}

simple GCLRecorder
{
    parameters:
        string outputPath = default("simulations/gcl_changes.csv");
        int recordInterval = default(10);
        
        @class(GCLRecorder);
        @display("i=block/recorder");
}
