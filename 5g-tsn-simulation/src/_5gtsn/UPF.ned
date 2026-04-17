package src._5gtsn;

simple UPF
{
    parameters:
        int upfId = default(0);
        int numUes = default(2);
        int numGateways = default(1);
        double processingInterval @unit(s) = default(0.0001s);
        @display("i=device/server;is=l");
    gates:
        input gncIn[];
        output gnbOut[];
        input gatewayIn[];
        output gatewayOut[];
}
