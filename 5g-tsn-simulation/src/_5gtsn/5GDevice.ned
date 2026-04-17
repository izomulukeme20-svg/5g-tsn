package src._5gtsn;

simple GDevice
{
    parameters:
        int deviceId = default(0);
        int gnbId = default(0);
        int numFlowTypes = default(4);
        bool isActive = default(false);
        double activityInterval @unit(s) = default(0.001s);
        double bsrInterval @unit(s) = default(0.001s);
        int numDestinations = default(1);
        @display("i=device/mobile");
    gates:
        input in @labels(EtherFrame-conn);
        output out @labels(EtherFrame-conn);
}
