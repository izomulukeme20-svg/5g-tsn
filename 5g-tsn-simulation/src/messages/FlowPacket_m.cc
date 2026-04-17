//
// Generated file, do not edit! Created by opp_msgtool 6.3 from messages/FlowPacket.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "FlowPacket_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

Register_Class(FlowPacket)

FlowPacket::FlowPacket(const char *name, short kind) : ::omnetpp::cPacket(name, kind)
{
}

FlowPacket::FlowPacket(const FlowPacket& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

FlowPacket::~FlowPacket()
{
}

FlowPacket& FlowPacket::operator=(const FlowPacket& other)
{
    if (this == &other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void FlowPacket::copy(const FlowPacket& other)
{
    this->flowId = other.flowId;
    this->srcNode = other.srcNode;
    this->dstNode = other.dstNode;
    this->pcp = other.pcp;
    this->qfi = other.qfi;
    this->drbId = other.drbId;
    this->srcMacAddress = other.srcMacAddress;
    this->dstMacAddress = other.dstMacAddress;
    this->ethertype = other.ethertype;
    this->vlanId = other.vlanId;
    this->size = other.size;
    this->tstart = other.tstart;
    this->pdb = other.pdb;
    this->arrivalTime = other.arrivalTime;
    this->deadline = other.deadline;
    this->wirelessDelay = other.wirelessDelay;
    this->jitter = other.jitter;
    this->hopCount = other.hopCount;
    this->isScheduled_ = other.isScheduled_;
    this->assignedQueue = other.assignedQueue;
    this->assignedSlot = other.assignedSlot;
}

void FlowPacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->flowId);
    doParsimPacking(b,this->srcNode);
    doParsimPacking(b,this->dstNode);
    doParsimPacking(b,this->pcp);
    doParsimPacking(b,this->qfi);
    doParsimPacking(b,this->drbId);
    doParsimPacking(b,this->srcMacAddress);
    doParsimPacking(b,this->dstMacAddress);
    doParsimPacking(b,this->ethertype);
    doParsimPacking(b,this->vlanId);
    doParsimPacking(b,this->size);
    doParsimPacking(b,this->tstart);
    doParsimPacking(b,this->pdb);
    doParsimPacking(b,this->arrivalTime);
    doParsimPacking(b,this->deadline);
    doParsimPacking(b,this->wirelessDelay);
    doParsimPacking(b,this->jitter);
    doParsimPacking(b,this->hopCount);
    doParsimPacking(b,this->isScheduled_);
    doParsimPacking(b,this->assignedQueue);
    doParsimPacking(b,this->assignedSlot);
}

void FlowPacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->flowId);
    doParsimUnpacking(b,this->srcNode);
    doParsimUnpacking(b,this->dstNode);
    doParsimUnpacking(b,this->pcp);
    doParsimUnpacking(b,this->qfi);
    doParsimUnpacking(b,this->drbId);
    doParsimUnpacking(b,this->srcMacAddress);
    doParsimUnpacking(b,this->dstMacAddress);
    doParsimUnpacking(b,this->ethertype);
    doParsimUnpacking(b,this->vlanId);
    doParsimUnpacking(b,this->size);
    doParsimUnpacking(b,this->tstart);
    doParsimUnpacking(b,this->pdb);
    doParsimUnpacking(b,this->arrivalTime);
    doParsimUnpacking(b,this->deadline);
    doParsimUnpacking(b,this->wirelessDelay);
    doParsimUnpacking(b,this->jitter);
    doParsimUnpacking(b,this->hopCount);
    doParsimUnpacking(b,this->isScheduled_);
    doParsimUnpacking(b,this->assignedQueue);
    doParsimUnpacking(b,this->assignedSlot);
}

int FlowPacket::getFlowId() const
{
    return this->flowId;
}

void FlowPacket::setFlowId(int flowId)
{
    this->flowId = flowId;
}

int FlowPacket::getSrcNode() const
{
    return this->srcNode;
}

void FlowPacket::setSrcNode(int srcNode)
{
    this->srcNode = srcNode;
}

int FlowPacket::getDstNode() const
{
    return this->dstNode;
}

void FlowPacket::setDstNode(int dstNode)
{
    this->dstNode = dstNode;
}

int FlowPacket::getPcp() const
{
    return this->pcp;
}

void FlowPacket::setPcp(int pcp)
{
    this->pcp = pcp;
}

int FlowPacket::getQfi() const
{
    return this->qfi;
}

void FlowPacket::setQfi(int qfi)
{
    this->qfi = qfi;
}

int FlowPacket::getDrbId() const
{
    return this->drbId;
}

void FlowPacket::setDrbId(int drbId)
{
    this->drbId = drbId;
}

int FlowPacket::getSrcMacAddress() const
{
    return this->srcMacAddress;
}

void FlowPacket::setSrcMacAddress(int srcMacAddress)
{
    this->srcMacAddress = srcMacAddress;
}

int FlowPacket::getDstMacAddress() const
{
    return this->dstMacAddress;
}

void FlowPacket::setDstMacAddress(int dstMacAddress)
{
    this->dstMacAddress = dstMacAddress;
}

int FlowPacket::getEthertype() const
{
    return this->ethertype;
}

void FlowPacket::setEthertype(int ethertype)
{
    this->ethertype = ethertype;
}

int FlowPacket::getVlanId() const
{
    return this->vlanId;
}

void FlowPacket::setVlanId(int vlanId)
{
    this->vlanId = vlanId;
}

int FlowPacket::getSize() const
{
    return this->size;
}

void FlowPacket::setSize(int size)
{
    this->size = size;
}

double FlowPacket::getTstart() const
{
    return this->tstart;
}

void FlowPacket::setTstart(double tstart)
{
    this->tstart = tstart;
}

double FlowPacket::getPdb() const
{
    return this->pdb;
}

void FlowPacket::setPdb(double pdb)
{
    this->pdb = pdb;
}

double FlowPacket::getArrivalTime() const
{
    return this->arrivalTime;
}

void FlowPacket::setArrivalTime(double arrivalTime)
{
    this->arrivalTime = arrivalTime;
}

double FlowPacket::getDeadline() const
{
    return this->deadline;
}

void FlowPacket::setDeadline(double deadline)
{
    this->deadline = deadline;
}

double FlowPacket::getWirelessDelay() const
{
    return this->wirelessDelay;
}

void FlowPacket::setWirelessDelay(double wirelessDelay)
{
    this->wirelessDelay = wirelessDelay;
}

double FlowPacket::getJitter() const
{
    return this->jitter;
}

void FlowPacket::setJitter(double jitter)
{
    this->jitter = jitter;
}

int FlowPacket::getHopCount() const
{
    return this->hopCount;
}

void FlowPacket::setHopCount(int hopCount)
{
    this->hopCount = hopCount;
}

bool FlowPacket::isScheduled() const
{
    return this->isScheduled_;
}

void FlowPacket::setIsScheduled(bool isScheduled)
{
    this->isScheduled_ = isScheduled;
}

int FlowPacket::getAssignedQueue() const
{
    return this->assignedQueue;
}

void FlowPacket::setAssignedQueue(int assignedQueue)
{
    this->assignedQueue = assignedQueue;
}

int FlowPacket::getAssignedSlot() const
{
    return this->assignedSlot;
}

void FlowPacket::setAssignedSlot(int assignedSlot)
{
    this->assignedSlot = assignedSlot;
}

class FlowPacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_flowId,
        FIELD_srcNode,
        FIELD_dstNode,
        FIELD_pcp,
        FIELD_qfi,
        FIELD_drbId,
        FIELD_srcMacAddress,
        FIELD_dstMacAddress,
        FIELD_ethertype,
        FIELD_vlanId,
        FIELD_size,
        FIELD_tstart,
        FIELD_pdb,
        FIELD_arrivalTime,
        FIELD_deadline,
        FIELD_wirelessDelay,
        FIELD_jitter,
        FIELD_hopCount,
        FIELD_isScheduled,
        FIELD_assignedQueue,
        FIELD_assignedSlot,
    };
  public:
    FlowPacketDescriptor();
    virtual ~FlowPacketDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(FlowPacketDescriptor)

FlowPacketDescriptor::FlowPacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(FlowPacket)), "omnetpp::cPacket")
{
    propertyNames = nullptr;
}

FlowPacketDescriptor::~FlowPacketDescriptor()
{
    delete[] propertyNames;
}

bool FlowPacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<FlowPacket *>(obj)!=nullptr;
}

const char **FlowPacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *FlowPacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int FlowPacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 21+base->getFieldCount() : 21;
}

unsigned int FlowPacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_flowId
        FD_ISEDITABLE,    // FIELD_srcNode
        FD_ISEDITABLE,    // FIELD_dstNode
        FD_ISEDITABLE,    // FIELD_pcp
        FD_ISEDITABLE,    // FIELD_qfi
        FD_ISEDITABLE,    // FIELD_drbId
        FD_ISEDITABLE,    // FIELD_srcMacAddress
        FD_ISEDITABLE,    // FIELD_dstMacAddress
        FD_ISEDITABLE,    // FIELD_ethertype
        FD_ISEDITABLE,    // FIELD_vlanId
        FD_ISEDITABLE,    // FIELD_size
        FD_ISEDITABLE,    // FIELD_tstart
        FD_ISEDITABLE,    // FIELD_pdb
        FD_ISEDITABLE,    // FIELD_arrivalTime
        FD_ISEDITABLE,    // FIELD_deadline
        FD_ISEDITABLE,    // FIELD_wirelessDelay
        FD_ISEDITABLE,    // FIELD_jitter
        FD_ISEDITABLE,    // FIELD_hopCount
        FD_ISEDITABLE,    // FIELD_isScheduled
        FD_ISEDITABLE,    // FIELD_assignedQueue
        FD_ISEDITABLE,    // FIELD_assignedSlot
    };
    return (field >= 0 && field < 21) ? fieldTypeFlags[field] : 0;
}

const char *FlowPacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "flowId",
        "srcNode",
        "dstNode",
        "pcp",
        "qfi",
        "drbId",
        "srcMacAddress",
        "dstMacAddress",
        "ethertype",
        "vlanId",
        "size",
        "tstart",
        "pdb",
        "arrivalTime",
        "deadline",
        "wirelessDelay",
        "jitter",
        "hopCount",
        "isScheduled",
        "assignedQueue",
        "assignedSlot",
    };
    return (field >= 0 && field < 21) ? fieldNames[field] : nullptr;
}

int FlowPacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "flowId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "srcNode") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "dstNode") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "pcp") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "qfi") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "drbId") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "srcMacAddress") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "dstMacAddress") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "ethertype") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "vlanId") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "size") == 0) return baseIndex + 10;
    if (strcmp(fieldName, "tstart") == 0) return baseIndex + 11;
    if (strcmp(fieldName, "pdb") == 0) return baseIndex + 12;
    if (strcmp(fieldName, "arrivalTime") == 0) return baseIndex + 13;
    if (strcmp(fieldName, "deadline") == 0) return baseIndex + 14;
    if (strcmp(fieldName, "wirelessDelay") == 0) return baseIndex + 15;
    if (strcmp(fieldName, "jitter") == 0) return baseIndex + 16;
    if (strcmp(fieldName, "hopCount") == 0) return baseIndex + 17;
    if (strcmp(fieldName, "isScheduled") == 0) return baseIndex + 18;
    if (strcmp(fieldName, "assignedQueue") == 0) return baseIndex + 19;
    if (strcmp(fieldName, "assignedSlot") == 0) return baseIndex + 20;
    return base ? base->findField(fieldName) : -1;
}

const char *FlowPacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_flowId
        "int",    // FIELD_srcNode
        "int",    // FIELD_dstNode
        "int",    // FIELD_pcp
        "int",    // FIELD_qfi
        "int",    // FIELD_drbId
        "int",    // FIELD_srcMacAddress
        "int",    // FIELD_dstMacAddress
        "int",    // FIELD_ethertype
        "int",    // FIELD_vlanId
        "int",    // FIELD_size
        "double",    // FIELD_tstart
        "double",    // FIELD_pdb
        "double",    // FIELD_arrivalTime
        "double",    // FIELD_deadline
        "double",    // FIELD_wirelessDelay
        "double",    // FIELD_jitter
        "int",    // FIELD_hopCount
        "bool",    // FIELD_isScheduled
        "int",    // FIELD_assignedQueue
        "int",    // FIELD_assignedSlot
    };
    return (field >= 0 && field < 21) ? fieldTypeStrings[field] : nullptr;
}

const char **FlowPacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *FlowPacketDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int FlowPacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void FlowPacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'FlowPacket'", field);
    }
}

const char *FlowPacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string FlowPacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: return long2string(pp->getFlowId());
        case FIELD_srcNode: return long2string(pp->getSrcNode());
        case FIELD_dstNode: return long2string(pp->getDstNode());
        case FIELD_pcp: return long2string(pp->getPcp());
        case FIELD_qfi: return long2string(pp->getQfi());
        case FIELD_drbId: return long2string(pp->getDrbId());
        case FIELD_srcMacAddress: return long2string(pp->getSrcMacAddress());
        case FIELD_dstMacAddress: return long2string(pp->getDstMacAddress());
        case FIELD_ethertype: return long2string(pp->getEthertype());
        case FIELD_vlanId: return long2string(pp->getVlanId());
        case FIELD_size: return long2string(pp->getSize());
        case FIELD_tstart: return double2string(pp->getTstart());
        case FIELD_pdb: return double2string(pp->getPdb());
        case FIELD_arrivalTime: return double2string(pp->getArrivalTime());
        case FIELD_deadline: return double2string(pp->getDeadline());
        case FIELD_wirelessDelay: return double2string(pp->getWirelessDelay());
        case FIELD_jitter: return double2string(pp->getJitter());
        case FIELD_hopCount: return long2string(pp->getHopCount());
        case FIELD_isScheduled: return bool2string(pp->isScheduled());
        case FIELD_assignedQueue: return long2string(pp->getAssignedQueue());
        case FIELD_assignedSlot: return long2string(pp->getAssignedSlot());
        default: return "";
    }
}

void FlowPacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: pp->setFlowId(string2long(value)); break;
        case FIELD_srcNode: pp->setSrcNode(string2long(value)); break;
        case FIELD_dstNode: pp->setDstNode(string2long(value)); break;
        case FIELD_pcp: pp->setPcp(string2long(value)); break;
        case FIELD_qfi: pp->setQfi(string2long(value)); break;
        case FIELD_drbId: pp->setDrbId(string2long(value)); break;
        case FIELD_srcMacAddress: pp->setSrcMacAddress(string2long(value)); break;
        case FIELD_dstMacAddress: pp->setDstMacAddress(string2long(value)); break;
        case FIELD_ethertype: pp->setEthertype(string2long(value)); break;
        case FIELD_vlanId: pp->setVlanId(string2long(value)); break;
        case FIELD_size: pp->setSize(string2long(value)); break;
        case FIELD_tstart: pp->setTstart(string2double(value)); break;
        case FIELD_pdb: pp->setPdb(string2double(value)); break;
        case FIELD_arrivalTime: pp->setArrivalTime(string2double(value)); break;
        case FIELD_deadline: pp->setDeadline(string2double(value)); break;
        case FIELD_wirelessDelay: pp->setWirelessDelay(string2double(value)); break;
        case FIELD_jitter: pp->setJitter(string2double(value)); break;
        case FIELD_hopCount: pp->setHopCount(string2long(value)); break;
        case FIELD_isScheduled: pp->setIsScheduled(string2bool(value)); break;
        case FIELD_assignedQueue: pp->setAssignedQueue(string2long(value)); break;
        case FIELD_assignedSlot: pp->setAssignedSlot(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'FlowPacket'", field);
    }
}

omnetpp::cValue FlowPacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: return pp->getFlowId();
        case FIELD_srcNode: return pp->getSrcNode();
        case FIELD_dstNode: return pp->getDstNode();
        case FIELD_pcp: return pp->getPcp();
        case FIELD_qfi: return pp->getQfi();
        case FIELD_drbId: return pp->getDrbId();
        case FIELD_srcMacAddress: return pp->getSrcMacAddress();
        case FIELD_dstMacAddress: return pp->getDstMacAddress();
        case FIELD_ethertype: return pp->getEthertype();
        case FIELD_vlanId: return pp->getVlanId();
        case FIELD_size: return pp->getSize();
        case FIELD_tstart: return pp->getTstart();
        case FIELD_pdb: return pp->getPdb();
        case FIELD_arrivalTime: return pp->getArrivalTime();
        case FIELD_deadline: return pp->getDeadline();
        case FIELD_wirelessDelay: return pp->getWirelessDelay();
        case FIELD_jitter: return pp->getJitter();
        case FIELD_hopCount: return pp->getHopCount();
        case FIELD_isScheduled: return pp->isScheduled();
        case FIELD_assignedQueue: return pp->getAssignedQueue();
        case FIELD_assignedSlot: return pp->getAssignedSlot();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'FlowPacket' as cValue -- field index out of range?", field);
    }
}

void FlowPacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: pp->setFlowId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcNode: pp->setSrcNode(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstNode: pp->setDstNode(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_pcp: pp->setPcp(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_qfi: pp->setQfi(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_drbId: pp->setDrbId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcMacAddress: pp->setSrcMacAddress(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstMacAddress: pp->setDstMacAddress(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_ethertype: pp->setEthertype(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_vlanId: pp->setVlanId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_size: pp->setSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_tstart: pp->setTstart(value.doubleValue()); break;
        case FIELD_pdb: pp->setPdb(value.doubleValue()); break;
        case FIELD_arrivalTime: pp->setArrivalTime(value.doubleValue()); break;
        case FIELD_deadline: pp->setDeadline(value.doubleValue()); break;
        case FIELD_wirelessDelay: pp->setWirelessDelay(value.doubleValue()); break;
        case FIELD_jitter: pp->setJitter(value.doubleValue()); break;
        case FIELD_hopCount: pp->setHopCount(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_isScheduled: pp->setIsScheduled(value.boolValue()); break;
        case FIELD_assignedQueue: pp->setAssignedQueue(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_assignedSlot: pp->setAssignedSlot(omnetpp::checked_int_cast<int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'FlowPacket'", field);
    }
}

const char *FlowPacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr FlowPacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void FlowPacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowPacket *pp = omnetpp::fromAnyPtr<FlowPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'FlowPacket'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

