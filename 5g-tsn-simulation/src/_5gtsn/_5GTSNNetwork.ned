//
// ============================================================================
// NED 文件语法详解
// ============================================================================
//
// 【1. 包声明】
// package src._5gtsn;
// - package: 关键字，声明该 NED 文件所属的包
// - src._5gtsn: 包名，对应项目的目录结构 src/_5gtsn/
//
// 【2. 文件属性】
// @license(OMNeT++);  - @: 属性标记符号，用于添加元数据；声明许可证类型
// @icon(icons/network/network); - 定义在图形界面中显示的图标路径
//
// 【3. 导入语句】
// import inet.node.ethernet.Eth10G;  - import: 关键字，导入外部模块定义
// - Eth10G: 10G 以太网通道模块
// - Ipv4NetworkConfigurator: IPv4 网络自动配置器
// - Router: 路由器节点
// - StandardHost: 标准主机节点
//
// 【4. 网络定义】
// network _5GTSNNetwork { }
// - network: 关键字，声明一个网络类型
// - _5GTSNNetwork: 网络名称（标识符）
// - { }: 大括号定义网络的作用域/内容块
//
// 【5. 参数部分】
// parameters:
//     int numUes = default(2);        - int: 整数类型; default(2): 默认值为2
//     double simTime @unit(s) = default(10s); - double: 双精度浮点; @unit(s): 单位为秒
//     @display("bgb=1400,900");       - bgb: 背景边界框大小
//
// 【6. 子模块部分】
// submodules:
//     channelControl: simu5g.world.radio.LteChannelControl {
//         @display("p=100,50;is=s");
//     }
// - channelControl: 子模块实例名称
// - : 类型分隔符
// - simu5g.world.radio.LteChannelControl: 模块类型（完整路径）
// - @display 显示属性:
//   - p=100,50: position，模块在画布上的坐标位置
//   - is=s: image size，图标尺寸; is=l: large，大图标
//   - i=device/antennatower: icon，天线塔图标（用于gNodeB等基站设备）
//   - i=device/server: icon，服务器图标
//   - i=device/switch: icon，交换机图标
//   - i=device/smallrouter: icon，小型路由器图标
//   - column,150: 多实例时的列排列方式和间距
//
// 【7. 向量子模块】
// ue[numUes]: simu5g.nodes.NrUe { }
// - ue[numUes]: 向量模块，numUes 决定向量大小（数量）
// - [ ]: 数组/向量索引符号
//
// 【8. 连接部分】
// connections:
//     gnb.ppp <--> Eth10G <--> iUpf.pppg++;
// - . : 成员访问符号
// - ppp: 点对点接口/端口
// - <-->: 双向连接符号
// - Eth10G: 通道类型（10G以太网）
// - pppg: 点对点接口组
// - ++: 自动递增索引运算符，自动分配下一个可用端口索引
//
// 【9. 循环连接】
// for i=0..numSwitches-2 { }
// - for: 循环关键字
// - i=0..numSwitches-2: 循环范围，.. 表示范围运算符（从0到numSwitches-2）
// - tsnBridge[i]: 访问向量的第i个元素
// - ethg: 以太网接口组
//
// 【10. 模块功能说明】
// - channelControl: LTE 无线信道控制器
// - binder: SIMU5G 核心绑定器，管理模块间通信
// - carrierAggregation: 载波聚合功能
// - configurator: IPv4 网络自动配置
// - gnb: 5G 基站
// - ue[numUes]: 5G NR 用户设备（可配置数量）
// - upf: 用户平面功能（5G核心网）
// - iUpf: 中间 UPF (Intermediate UPF)
// - router: IP 路由器
// - tsnBridge[numSwitches]: TSN 交换机
// - endDevice[numEndDevices]: 终端设备
//
// 【11. 网络拓扑结构】
// [UE] <--无线--> [gNB] <--Eth10G--> [iUPF] <--Eth10G--> [UPF]
//                                                     |
//                                                 Eth10G
//                                                     |
//                                               [Router]
//                                                     |
//                                                 Eth10G
//                                                     |
//                                             [TSN Bridge 0] <--Eth10G--> [TSN Bridge 1] ... <--Eth10G--> [End Device]
//
// 这是一个典型的 5G-TSN 融合网络仿真拓扑，实现了：
// - 5G 无线接入网（gNB + UE）
// - 5G 核心网用户平面（UPF）
// - TSN 时间敏感网络
// - 终端设备
// ============================================================================
//

package src._5gtsn;

@license(OMNeT++);
@icon(icons/network/network);

import inet.node.ethernet.Eth10G;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.nodes.BackgroundCell;

//
// 5G-TSN 融合网络定义
// network: 关键字，声明一个网络类型
// _5GTSNNetwork: 网络名称（标识符）
// { }: 大括号定义网络的作用域/内容块
//
network _5GTSNNetwork
{
    //
    // parameters: 关键字，声明参数部分的开始
    // int: 整数类型
    // double: 双精度浮点类型
    // = : 赋值符号
    // default(n): 默认值函数，设置参数默认值为 n
    // @unit(s): 单位属性，表示单位为秒
    // @display("bgb=1400,900"): 显示属性，bgb 表示背景边界框大小
    //
    parameters:
        int numUes = default(2);              // UE用户设备数量
        int numBgCells = default(0);          // 背景小区数量
        int numSwitches = default(2);         // TSN交换机数量
        int numEndDevices = default(1);       // 终端设备数量
        double simTime @unit(s) = default(10s);  // 仿真时间，单位：秒

        @display("bgb=1400,900");             // 画布大小 1400x900 像素

    //
    // submodules: 关键字，声明子模块部分
    // 模块名: 模块类型 { 配置 }
    // : 类型分隔符
    // @display 显示属性:
    //   - p=x,y: position，模块在画布上的坐标位置
    //   - is=s/l: image size，图标尺寸; is=l: large，大图标
    //   - i=path: icon，图标路径
    //   - column,间距: 多实例时的列排列方式和间距
    //
    submodules:
        //
        // LTE无线信道控制器
        // p=100,50: 位置坐标 (100, 50)
        // is=s: 小图标
        //
        channelControl: simu5g.world.radio.LteChannelControl {
            @display("p=100,50;is=s");
        }

        //
        // SIMU5G核心绑定器，管理模块间通信
        // p=325,49: 位置坐标 (325, 49)
        //
        binder: simu5g.common.binder.Binder {
            @display("p=237,49;is=s");
        }

        //
        // 载波聚合功能模块
        // p=550,49: 位置坐标 (550, 49)
        // is=s: 小图标
        //
        carrierAggregation: simu5g.common.carrierAggregation.CarrierAggregation {
            @display("p=532,49;is=s");
        }

        //
        // IPv4网络自动配置器
        // p=803,65: 位置坐标 (803, 65)
        // is=l: 大图标
        //
        configurator: Ipv4NetworkConfigurator {
            @display("p=349,49;is=s");
        }

        //
        // 5G基站 (gNodeB - Next Generation NodeB)
        // p=200,350: 位置坐标 (200, 350)
        // i=device/antennatower: 天线塔图标 (Simu5G标准gNodeB图标)
        // is=l: 大图标
        //
        gnb: simu5g.nodes.gNodeB {
            @display("p=99,192;i=device/antennatower;is=l");
        }

        //
        // 5G NR用户设备 (向量子模块)
        // ue[numUes]: 向量模块，numUes 决定UE数量
        // [ ]: 数组/向量索引符号
        // p=199,602: 位置坐标 (199, 602)
        // is=l: 大图标
        //
        ue[numUes]: simu5g.nodes.NrUe {
            @display("p=101,573;is=l");
        }

        bgCell[numBgCells]: BackgroundCell {
            @display("p=448,588;is=l");
        }

        //
        // 用户平面功能 UPF (5G核心网)
        // p=468,568: 位置坐标 (468, 568)
        // i=device/server: 服务器图标
        // is=l: 大图标
        //
        upf: simu5g.nodes.Upf {
            @display("p=349,349;i=device/server;is=l");
        }

        nwTt: NwTtNode {
            @display("p=517,347;i=device/switch;is=l");
        }

        //
        // 中间UPF (Intermediate UPF)
        // p=550,350: 位置坐标 (550, 350)
        // i=device/server: 服务器图标
        // is=l: 大图标
        //
        iUpf: simu5g.nodes.Upf {
            @display("p=222,349;i=device/server;is=l");
        }

        //
        // IP路由器
        // p=718,351: 位置坐标 (718, 351)
        // i=device/smallrouter: 小路由器图标
        // is=l: 大图标
        //
        router: Router {
            @display("p=969,620;i=device/smallrouter;is=l");
        }

        //
        // TSN交换机 (向量子模块)
        // tsnBridge[numSwitches]: 向量模块，numSwitches 决定交换机数量
        // p=926,349: 起始位置坐标 (926, 349)
        // column,150: 列排列，间距150像素
        // i=device/switch: 交换机图标
        // is=l: 大图标
        //
        tsnBridge[numSwitches]: TSNBridge {
            @display("p=745,347,column,150;i=device/switch;is=l");
        }

        tsnEdgeAdapter[numEndDevices]: TsnEtherAdapter {
            @display("p=1058,280,column,100");
        }

        //
        // 终端设备 (向量子模块)
        // endDevice[numEndDevices]: 向量模块，numEndDevices 决定终端数量
        // p=1266,349: 起始位置坐标 (1266, 349)
        // column,100: 列排列，间距100像素
        // i=device/server: 服务器图标
        // is=l: 大图标
        //
        endDevice[numEndDevices]: StandardHost {
            @display("p=1267,432,column,100;i=device/server;is=l");
        }

    //
    // connections: 关键字，声明连接部分
    // 连接语法:
    //   模块.端口 <--> 通道类型 <--> 模块.端口
    //
    // 符号说明:
    //   . : 成员访问符号，访问模块的端口
    //   <--> : 双向连接符号
    //   ppp: 点对点接口/端口
    //   pppg: 点对点接口组
    //   ethg: 以太网接口组
    //   ++: 自动递增索引运算符，自动分配下一个可用端口索引
    //
    connections:
        // gNB <--Eth10G--> iUPF: 基站连接到中间UPF
        gnb.ppp <--> Eth10G <--> iUpf.pppg++;

        // iUPF <--Eth10G--> UPF: 中间UPF连接到UPF
        iUpf.pppg++ <--> Eth10G <--> upf.pppg++;

        upf.filterGate <--> Eth10G <--> nwTt.upfPort;
        nwTt.ethg <--> Eth10G <--> tsnBridge[0].port[0];

        //
        // 循环连接: 连接多个TSN交换机
        // for i=0..numSwitches-2 { }: 循环关键字，.. 表示范围运算符
        // tsnBridge[i].port[j]: 访问向量的第i个元素的port门
        //
        for i=0..numSwitches-2 {
            tsnBridge[i].port[1] <--> Eth10G <--> tsnBridge[i+1].port[0];
        }

        //
        // 循环连接: 最后一个TSN交换机连接到终端设备
        // tsnBridge[numSwitches-1]: 最后一个TSN交换机
        // endDevice[i]: 第i个终端设备
        //
        for i=0..numEndDevices-1 {
            tsnBridge[numSwitches-1].port[1] <--> Eth10G <--> tsnEdgeAdapter[i].tsnPort;
            tsnEdgeAdapter[i].ethg <--> Eth10G <--> endDevice[i].ethg++;
        }
}
