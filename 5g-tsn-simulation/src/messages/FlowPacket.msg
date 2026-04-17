// FlowPacket.msg - 流数据包定义
// 用于5G-TSN网络中的数据传输

packet FlowPacket {
    int flowId;
    int srcNode;
    int dstNode;
    int pcp;              // Priority Code Point (0-7)
    int qfi;              // QoS Flow Identifier
    int drbId;            // Data Radio Bearer ID
    int srcMacAddress;    // Source MAC address
    int dstMacAddress;    // Destination MAC address
    int ethertype;        // Ethernet type
    int vlanId;           // VLAN ID
    int size;             // Packet size in bytes
    double tstart;        // Start time
    double pdb;           // Per-stream Delay Bound
    double arrivalTime;   // Arrival time at gateway
    double deadline;      // Deadline for this packet
    double wirelessDelay; // Wireless transmission delay
    double jitter;        // Jitter
    int hopCount;         // Number of hops traversed
    bool isScheduled;     // Whether scheduled by OAFS
    int assignedQueue;    // Assigned queue priority
    int assignedSlot;     // Assigned time slot
}
