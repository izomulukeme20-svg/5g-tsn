package src._5gtsn;

import inet.linklayer.ppp.PppInterface;
import inet.networklayer.common.InterfaceTable;

//
// NW-TT 复合节点
// - 对 UPF 侧暴露标准 PPP 口
// - 对 TSN 侧暴露以太帧口，向下游交换节点发送标准 802.1Q 帧
//
module NwTtNode
{
    parameters:
        @networkNode();
        bool enableLogging = default(true);
        string nwTtMac = default("00:00:00:00:00:02");
        int defaultVlanId = default(1);
        string endDeviceMac = default("00:00:00:00:00:10");
        string pcpToDscpMap = default("7:56, 6:48, 5:40, 4:32, 3:24, 2:16, 1:8, 0:0");
        string dscpToPcpMap = default("56:7, 48:6, 40:5, 32:4, 24:3, 16:2, 8:1, 0:0");
        @display("i=device/switch");
    gates:
        inout upfPort @labels(PppFrame-conn);
        inout ethg @labels(EtherFrame-conn);
    submodules:
        interfaceTable: InterfaceTable {
            @display("p=100,100;is=s");
        }
        ppp: PppInterface {
            parameters:
                interfaceTableModule = default(absPath("^.interfaceTable"));
            @display("p=100,220");
        }
        translator: NwTt {
            parameters:
                enableLogging = parent.enableLogging;
                nwTtMac = parent.nwTtMac;
                defaultVlanId = parent.defaultVlanId;
                endDeviceMac = parent.endDeviceMac;
                pcpToDscpMap = parent.pcpToDscpMap;
                dscpToPcpMap = parent.dscpToPcpMap;
            @display("p=290,220");
        }
    connections allowunconnected:
        upfPort <--> ppp.phys;
        ppp.upperLayerOut --> translator.upfPort$i;
        translator.upfPort$o --> ppp.upperLayerIn;

        ethg <--> translator.tsnPort;
}
