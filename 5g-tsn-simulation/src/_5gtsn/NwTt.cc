#include "NwTt.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/ethernet/common/Ethernet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/ieee8021q/Ieee8021qTagHeader_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/Protocol.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/linklayer/common/FcsMode_m.h"
#include <cstdarg>

Define_Module(NwTt);

void NwTt::initialize() {
    enableLogging = par("enableLogging");
    nwTtMac = MacAddress(par("nwTtMac").stringValue());
    defaultVlanId = par("defaultVlanId");
    endDeviceMac = MacAddress(par("endDeviceMac").stringValue());
    
    // 初始化计数器
    numDownlinkFramesEncapsulated = 0;
    numUplinkFramesDecapsulated = 0;
    numVlanFramesProduced = 0;
    numFramesDroppedByFormat = 0;
    numPcpMapped = 0;
    
    // 初始化QoS映射表 (PCP << 3 = DSCP)
    parseMappingConfig(par("pcpToDscpMap").stdstringValue(), pcpToDscpMap);
    parseMappingConfig(par("dscpToPcpMap").stdstringValue(), dscpToPcpMap);
    
    // 初始化发送队列和自消息
    txEndSelfMsg = new cMessage("txEnd");
    txQueue.setName("NwTtTxQueue");
    
    log("=====================================");
    log("NW-TT 模块初始化完成");
    log("=====================================");
    log("MAC地址: %s", nwTtMac.str().c_str());
    log("默认VLAN ID: %d", defaultVlanId);
    log("终端设备MAC: %s", endDeviceMac.str().c_str());
    log("UPF端口已连接: %s", gate("upfPort$i")->isConnected() ? "是" : "否");
    log("TSN端口已连接: %s", gate("tsnPort$i")->isConnected() ? "是" : "否");
    log("加载PCP→DSCP映射规则 %d 条", (int)pcpToDscpMap.size());
    log("加载DSCP→PCP映射规则 %d 条", (int)dscpToPcpMap.size());
    log("=====================================");
}

void NwTt::handleMessage(cMessage *msg) {
    if (msg == txEndSelfMsg) {
        // 发送完成，处理队列中的下一个包
        if (!txQueue.isEmpty()) {
            Packet* pkt = check_and_cast<Packet*>(txQueue.pop());
            sendPkt(pkt);
        }
        return;
    }
    
    Packet* pkt = check_and_cast<Packet*>(msg);
    
    if (msg->arrivedOn("upfPort$i")) {
        // 下行方向：UPF IP包 → 封装成802.1Q帧发往TSN
        log("收到下行IP包，长度 %d 字节", pkt->getByteLength());
        Packet* ethPkt = processDownlinkFromUpf(pkt);
        if (ethPkt) {
            queueOrSendPkt(ethPkt, "tsnPort$o");
            log("下行封装完成，已加入发送队列");
        }
    } 
    else if (msg->arrivedOn("tsnPort$i")) {
        // 上行方向：TSN 802.1Q帧 → 解封装成IP包发往UPF
        log("收到上行以太网帧，长度 %d 字节", pkt->getByteLength());
        Packet* ipPkt = processUplinkFromTsn(pkt);
        if (ipPkt) {
            queueOrSendPkt(ipPkt, "upfPort$o");
            log("上行解封装完成，已加入发送队列");
        }
    }
    
    delete pkt;
}

// 队列或发送数据包
void NwTt::queueOrSendPkt(Packet* pkt, const char* gateName) {
    cGate *outGate = gate(gateName);
    cChannel *channel = outGate->getTransmissionChannel();
    simtime_t txFinishTime = channel->getTransmissionFinishTime();
    
    if (txFinishTime <= simTime()) {
        // 信道空闲，直接发送
        sendPkt(pkt, gateName);
    } else {
        // 信道忙，加入队列，网关名称存在包名中
        pkt->setName(gateName);
        txQueue.insert(pkt);
        log("信道忙，数据包加入队列，队列长度: %d", txQueue.getLength());
    }
}

// 实际发送数据包
void NwTt::sendPkt(Packet* pkt, const char* gateName) {
    send(pkt, gateName);
    cGate *outGate = gate(gateName);
    cChannel *channel = outGate->getTransmissionChannel();
    simtime_t txFinishTime = channel->getTransmissionFinishTime();
    scheduleAt(txFinishTime, txEndSelfMsg);
    log("数据包已发送，下一次可发送时间: %fs", txFinishTime.dbl());
}

// 重载：从队列取出的数据包发送
void NwTt::sendPkt(Packet* pkt) {
    const char* gateName = pkt->getName();
    sendPkt(pkt, gateName);
}

// 下行处理：IP包 → 以太网帧
Packet* NwTt::processDownlinkFromUpf(Packet* packet) {
    Packet* ethPkt = new Packet("EthernetFrame");
    
    // 复制所有标签
    ethPkt->copyTags(*packet);
    
    // 记录原始长度
    B originalTotalLen = packet->getTotalLength();
    log("[下行处理] 收到IPv4包，长度: %d 字节", B(originalTotalLen).get());
    
    // 1. 提取DSCP
    int dscp = 0;
    int pcp = 0;
    auto dscpTag = packet->findTag<DscpInd>();
    if (dscpTag) {
        dscp = dscpTag->getDifferentiatedServicesCodePoint();
        pcp = dscpToPcp(dscp);
        numPcpMapped++;
    }
    else {
        const auto& ipv4Header = packet->peekAtFront<Ipv4Header>();
        if (ipv4Header != nullptr) {
            dscp = ipv4Header->getDscp();
            pcp = dscpToPcp(dscp);
            numPcpMapped++;
        }
    }
    log("[下行处理] DSCP: %d → PCP: %d (映射成功)", dscp, pcp);
    
    // 2. 构建以太网头
    auto ethHeader = makeShared<EthernetMacHeader>();
    ethHeader->setSrc(nwTtMac);
    ethHeader->setDest(endDeviceMac);
    ethHeader->setTypeOrLength(ETHERTYPE_8021Q_TAG); // 标记为VLAN帧
    log("[下行处理] 以太网头: Src=%s, Dest=%s, EtherType=0x%x (802.1Q VLAN)",
        nwTtMac.str().c_str(), endDeviceMac.str().c_str(), ETHERTYPE_8021Q_TAG);
    
    // 3. 构建VLAN标签
    auto vlanTag = makeShared<Ieee8021qTagEpdHeader>();
    vlanTag->setPcp(pcp);
    vlanTag->setDei(false);
    vlanTag->setVid(defaultVlanId);
    vlanTag->setTypeOrLength(ETHERTYPE_IPv4); // 上层为IPv4
    log("[下行处理] VLAN标签: PCP=%d, VLAN ID=%d, 上层协议=0x%x (IPv4)",
        pcp, defaultVlanId, ETHERTYPE_IPv4);
    
    // 4. 插入各层头和载荷
    ethPkt->insertAtBack(ethHeader);
    ethPkt->insertAtBack(vlanTag);
    ethPkt->insertAtBack(packet->peekData());
    auto ethernetFcs = makeShared<EthernetFcs>();
    ethernetFcs->setFcs(0);
    ethernetFcs->setFcsMode(FCS_DECLARED_CORRECT);
    ethPkt->insertAtBack(ethernetFcs);
    log("[下行处理] 已添加FCS校验，模式=DECLARED_CORRECT");
    
    // 计算最终长度
    B ethHeaderLen = ETHER_MAC_HEADER_BYTES;
    B vlanTagLen = B(4);
    B fcsLen = B(4);
    B totalLen = ethHeaderLen + vlanTagLen + originalTotalLen + fcsLen;
    B actualLen = ethPkt->getTotalLength();
    
    log("[下行处理] 封装完成: 总长度=%d 字节 (预期%d字节)", 
        B(actualLen).get(), B(totalLen).get());
    
    // 更新计数器
    numDownlinkFramesEncapsulated++;
    numVlanFramesProduced++;
    
    return ethPkt;
}

// 上行处理：以太网帧 → IP包
Packet* NwTt::processUplinkFromTsn(Packet* ethPkt) {
    Packet* ipPkt = new Packet("IpPacket");
    
    // 记录原始长度
    B originalEthLen = ethPkt->getTotalLength();
    log("[上行处理] 收到以太网帧，长度: %d 字节", B(originalEthLen).get());
    
    // 1. 弹出以太网头
    if (ethPkt->getTotalLength() < ETHER_MAC_HEADER_BYTES) {
        log("[上行处理] 错误：以太网帧长度不足，丢弃");
        numFramesDroppedByFormat++;
        delete ipPkt;
        return nullptr;
    }
    auto ethHeader = ethPkt->popAtFront<EthernetMacHeader>(B(ETHER_MAC_HEADER_BYTES), Chunk::PF_ALLOW_INCOMPLETE);
    B afterEthLen = ethPkt->getTotalLength();
    
    bool isVlanFrame = (ethHeader->getTypeOrLength() == ETHERTYPE_8021Q_TAG);
    log("[上行处理] 以太网头解析: EtherType=0x%x, 是802.1Q VLAN帧: %s",
        ethHeader->getTypeOrLength(), isVlanFrame ? "是" : "否");
    
    // 2. 弹出VLAN标签
    int pcp = 0;
    int vlanId = defaultVlanId;
    if (isVlanFrame) {
        if (ethPkt->getTotalLength() < B(4)) {
            log("[上行处理] 错误：VLAN帧长度不足，丢弃");
            numFramesDroppedByFormat++;
            delete ipPkt;
            return nullptr;
        }
        auto vlanTag = ethPkt->popAtFront<Ieee8021qTagEpdHeader>(B(4), Chunk::PF_ALLOW_INCOMPLETE);
        pcp = vlanTag->getPcp();
        vlanId = vlanTag->getVid();
        B afterVlanLen = ethPkt->getTotalLength();
        log("[上行处理] VLAN标签解析: PCP=%d, VLAN ID=%d, 剩余载荷长度: %d 字节",
            pcp, vlanId, B(afterVlanLen).get());
    }
    
    // 3. PCP转DSCP
    int dscp = pcpToDscp(pcp);
    numPcpMapped++;
    log("[上行处理] PCP: %d → DSCP: %d (映射成功)", pcp, dscp);
    
    // 4. 复制其他标签
    ipPkt->copyTags(*ethPkt);
    
    // 5. 设置上送 PPP 接口所需的 IPv4 协议标签
    ipPkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
    auto dscpInd = ipPkt->addTagIfAbsent<DscpInd>();
    dscpInd->setDifferentiatedServicesCodePoint(dscp);
    auto dscpReq = ipPkt->addTagIfAbsent<DscpReq>();
    dscpReq->setDifferentiatedServicesCodePoint(dscp);

    // 6. 插入IP载荷，PPP 头由 PppInterface 自动补齐
    ipPkt->insertAtBack(ethPkt->peekData());
    
    // 计算最终长度
    B ipPayloadLen = ethPkt->getTotalLength();
    B actualLen = ipPkt->getTotalLength();
    
    log("[上行处理] 解封装完成: 输出IPv4包长度=%d 字节", B(actualLen).get());
    
    // 更新计数器
    numUplinkFramesDecapsulated++;
    
    return ipPkt;
}

// PCP转DSCP
int NwTt::pcpToDscp(int pcp) {
    auto it = pcpToDscpMap.find(pcp);
    if (it != pcpToDscpMap.end()) {
        return it->second;
    }
    return 0; // 默认BE
}

// DSCP转PCP
int NwTt::dscpToPcp(int dscp) {
    auto it = dscpToPcpMap.find(dscp);
    if (it != dscpToPcpMap.end()) {
        return it->second;
    }
    return 0; // 默认最低优先级
}

// 解析映射配置
void NwTt::parseMappingConfig(const std::string& config, std::map<int, int>& map) {
    std::stringstream ss(config);
    std::string pair;
    
    while (std::getline(ss, pair, ',')) {
        // 去除空格
        pair.erase(remove_if(pair.begin(), pair.end(), ::isspace), pair.end());
        if (pair.empty()) continue;
        
        size_t colon = pair.find(':');
        if (colon == std::string::npos) continue;
        
        int key = std::stoi(pair.substr(0, colon));
        int value = std::stoi(pair.substr(colon+1));
        map[key] = value;
    }
}

// 日志函数
void NwTt::log(const char* format, ...) {
    if (!enableLogging) return;
    
    va_list args;
    va_start(args, format);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    printf("[NW-TT] %s %s\n", simTime().ustr().c_str(), buf);
    EV_INFO << "[NW-TT] " << simTime().ustr() << " " << buf << endl;
}

void NwTt::finish() {
    log("=====================================");
    log("NW-TT 运行统计结果");
    log("=====================================");
    log("下行封装成功帧数: %d", numDownlinkFramesEncapsulated);
    log("上行解封装成功帧数: %d", numUplinkFramesDecapsulated);
    log("生成802.1Q VLAN帧数: %d", numVlanFramesProduced);
    log("格式错误丢弃帧数: %d", numFramesDroppedByFormat);
    log("PCP/DSCP映射成功次数: %d", numPcpMapped);
    log("=====================================");
    log("NW-TT 运行结束");
}

NwTt::~NwTt() {
    // 清理资源
    cancelAndDelete(txEndSelfMsg);
    while (!txQueue.isEmpty()) {
        Packet* pkt = check_and_cast<Packet*>(txQueue.pop());
        delete pkt;
    }
}
