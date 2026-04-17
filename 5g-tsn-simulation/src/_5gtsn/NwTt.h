#ifndef __NW_TT_H
#define __NW_TT_H

#include <omnetpp.h>
#include <map>
#include <queue>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/common/TagBase.h"

using namespace omnetpp;
using namespace inet;

// 3GPP R16 NW-TT 二层封装实现
// 负责802.1Q以太网帧与IP包的双向转换 + QoS映射
class NwTt : public cSimpleModule {
protected:
    // 模块参数
    bool enableLogging;
    MacAddress nwTtMac;                  // NW-TT自身MAC地址
    int defaultVlanId;                   // 默认VLAN ID
    MacAddress endDeviceMac;             // 终端设备MAC地址
    
    // QoS映射表
    std::map<int, int> pcpToDscpMap;      // TSN PCP → IPv4 DSCP
    std::map<int, int> dscpToPcpMap;      // IPv4 DSCP → TSN PCP
    
    // 发送队列与调度
    cQueue txQueue;                        // 发送队列
    cMessage *txEndSelfMsg;               // 发送完成自消息
    
    // 统计计数器
    int numDownlinkFramesEncapsulated;    // 下行封装成功帧数
    int numUplinkFramesDecapsulated;      // 上行解封装成功帧数
    int numVlanFramesProduced;            // 生成的802.1Q VLAN帧数
    int numFramesDroppedByFormat;         // 格式错误丢弃帧数
    int numPcpMapped;                     // PCP/DSCP映射成功次数

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~NwTt() override;

private:
    // 核心处理函数
    Packet* processDownlinkFromUpf(Packet* ipPkt);    // UPF IP包 → TSN 802.1Q帧
    Packet* processUplinkFromTsn(Packet* ethPkt);    // TSN 802.1Q帧 → UPF IP包
    
    // 发送调度
    void queueOrSendPkt(Packet* pkt, const char* gateName);
    void sendPkt(Packet* pkt, const char* gateName);
    void sendPkt(Packet* pkt);
    
    // QoS映射
    int pcpToDscp(int pcp);
    int dscpToPcp(int dscp);
    
    // 工具函数
    void parseMappingConfig(const std::string& config, std::map<int, int>& map);
    void log(const char* format, ...);
};

#endif
