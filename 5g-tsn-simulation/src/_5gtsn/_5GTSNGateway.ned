package src._5gtsn;

module _5GTSNGateway
{
    parameters:
        int gatewayId = default(0);
        int numTimeSlots = default(64);
        int numQueues = default(8);
        int numSwitches = default(3);
        double timeSlotSize @unit(s) = default(0.000015625s);
        double hyperPeriod @unit(s) = default(0.016s);
        double linkRate @unit(bps) = default(100Mbps);
        bool enableEDRL = default(true);
        bool enableOAFS = default(true);
        bool enableMFM = default(true);
        bool enableMTM = default(true);
        double trainingInterval @unit(s) = default(0.1s);
        
        @class(_5GTSNGateway);
        @display("i=device/gateway");
        
    gates:
        inout fiveGIn @labels(FiveG-conn);
        inout tsnOut @labels(EtherFrame-conn);
        inout tsnIn @labels(EtherFrame-conn);
}
