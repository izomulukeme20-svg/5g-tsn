// ControlMessage.msg - 控制消息定义
// 用于5G-TSN网络中的控制信令

message GCLUpdate {
    int switchId;
    int portId;
    string gclData;       // GCL configuration data
}

message FlowSchedule {
    int flowId;
    int priority;
    int timeSlot;
    double expectedDelay;
}

message ControlMessage {
    @customize(true);
    int msgType;          // Message type
    double timestamp;
    
    GCLUpdate gclUpdate;
    FlowSchedule flowSchedule;
}
