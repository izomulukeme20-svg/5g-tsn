package src._5gtsn;

import inet.physicallayer.wired.ethernet.EthernetPhy;

module TsnEtherAdapter
{
    parameters:
        @display("i=block/wrxtx");
    gates:
        inout tsnPort;
        inout ethg @labels(EtherFrame-conn);
    submodules:
        phy: EthernetPhy {
            @display("p=150,120");
        }
    connections allowunconnected:
        tsnPort$i --> phy.upperLayerIn;
        phy.upperLayerOut --> tsnPort$o;
        ethg <--> phy.phys;
}
