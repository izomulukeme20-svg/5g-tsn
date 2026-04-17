package src._5gtsn;

import inet.linklayer.ethernet.common.MacRelayUnit;

module TSNSwitch extends MacRelayUnit
{
    parameters:
        int switchId = default(0);
        int numQueues = default(8);
        int numTimeSlots = default(1024);
        double timeSlotSize @unit(s) = default(0.000015625s);
        double hyperPeriod @unit(s) = default(0.016s);
        double processingDelay @unit(s) = default(0.000001s);
        double linkRate @unit(bps) = default(100Mbps);
        
        @class(TSNSwitch);
}
