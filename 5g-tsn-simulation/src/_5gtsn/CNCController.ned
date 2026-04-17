package src._5gtsn;

simple CNCController
{
    parameters:
        int numSwitches = default(2);
        int numQueues = default(8);
        int numTimeSlots = default(1024);       // 论文参数: 16ms / (1/64ms) = 1024
        double timeSlotSize @unit(s) = default(0.000015625s); // 论文参数: 1/64 ms ≈ 15.625μs
        double hyperPeriod @unit(s) = default(0.016s);
        double configUpdateInterval @unit(s) = default(0.01s);
        double statusCollectInterval @unit(s) = default(0.1s);
        double optimizationInterval @unit(s) = default(1s);
        int historySize = default(100);
        double targetSatisfaction = default(0.95);
        double targetMaxDelay @unit(s) = default(0.01s);
        bool enableGreedyScheduler = default(true);
        
        @class(CNCController);
        @display("i=device/server");
        
    gates:
        output out[] @loose;
}
