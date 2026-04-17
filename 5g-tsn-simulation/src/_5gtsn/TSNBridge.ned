package src._5gtsn;

simple TSNBridge
{
    parameters:
        int bridgeId = default(0);
        int numPorts = default(4);
        int numQueues = default(8);
        int numTimeSlots = default(1024);       // 论文参数: 16ms / (1/64ms) = 1024
        double timeSlotSize @unit(s) = default(0.000015625s); // 论文参数: 1/64 ms ≈ 15.625μs
        double hyperPeriod @unit(s) = default(0.016s);
        double linkRate @unit(bps) = default(100Mbps);  // 论文参数: 100 Mbps
        bool enableEDRL = default(true);
        bool enableOAFS = default(true);
        bool enableTimeAwareShaper = default(true);
        bool enableGreedyScheduler = default(true);
        double gclUpdateInterval @unit(s) = default(0.001s);
        double trainingInterval @unit(s) = default(0.01s);
        
        @class(TSNBridge);
        @display("i=block/switch");
        
    gates:
        inout port[numPorts] @loose;
}
