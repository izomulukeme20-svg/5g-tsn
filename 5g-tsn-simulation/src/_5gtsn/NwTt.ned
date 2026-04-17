package src._5gtsn;

// 3GPP R16定义的网络侧TSN转换器 (NW-TT)
// 位于UPF与TSN网络之间，负责二层/三层语义转换与QoS映射
simple NwTt {
    parameters:
        @class(NwTt);
        @display("i=device/switch;bgb=300,200");
        
        // 基础配置
        bool enableLogging = default(true);
        string nwTtMac = default("00:00:00:00:00:02"); // NW-TT自身的MAC地址
        int defaultVlanId = default(1); // 默认VLAN ID
        string endDeviceMac = default("00:00:00:00:00:10"); // 终端设备的MAC地址
        
        // QoS映射规则 (802.1Q PCP <-> IPv4 DSCP)
        // 默认映射规则: PCP << 3 = DSCP (1:8对应关系，简单可逆)
        string pcpToDscpMap = default("7:56, 6:48, 5:40, 4:32, 3:24, 2:16, 1:8, 0:0");
        string dscpToPcpMap = default("56:7, 48:6, 40:5, 32:4, 24:3, 16:2, 8:1, 0:0");

    gates:
        inout tsnPort;  // 连接 TSN 侧的二层以太帧端口
        inout upfPort;  // 连接 PPP 接口上层的 IPv4 包端口
}
