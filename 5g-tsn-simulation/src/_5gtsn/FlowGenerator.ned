package src._5gtsn;

import inet.applications.contract.IApp;

simple FlowGenerator like IApp
{
    parameters:
        int nodeId = default(0);
        int numDestinations = default(1);
        bool isActive = default(true);
        int seed = default(0);
        
        @class(FlowGenerator);
        @display("i=block/source");
        
    gates:
        input socketIn;
        output socketOut;
}
