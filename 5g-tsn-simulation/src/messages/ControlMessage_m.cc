//
// Generated file, do not edit! Created by opp_msgtool 6.3 from messages/ControlMessage.msg.
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
#include "ControlMessage_m.h"

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

Register_Class(GCLUpdate)

GCLUpdate::GCLUpdate(const char *name, short kind) : ::omnetpp::cMessage(name, kind)
{
}

GCLUpdate::GCLUpdate(const GCLUpdate& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

GCLUpdate::~GCLUpdate()
{
}

GCLUpdate& GCLUpdate::operator=(const GCLUpdate& other)
{
    if (this == &other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void GCLUpdate::copy(const GCLUpdate& other)
{
    this->switchId = other.switchId;
    this->portId = other.portId;
    this->gclData = other.gclData;
}

void GCLUpdate::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->switchId);
    doParsimPacking(b,this->portId);
    doParsimPacking(b,this->gclData);
}

void GCLUpdate::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->switchId);
    doParsimUnpacking(b,this->portId);
    doParsimUnpacking(b,this->gclData);
}

int GCLUpdate::getSwitchId() const
{
    return this->switchId;
}

void GCLUpdate::setSwitchId(int switchId)
{
    this->switchId = switchId;
}

int GCLUpdate::getPortId() const
{
    return this->portId;
}

void GCLUpdate::setPortId(int portId)
{
    this->portId = portId;
}

const char * GCLUpdate::getGclData() const
{
    return this->gclData.c_str();
}

void GCLUpdate::setGclData(const char * gclData)
{
    this->gclData = gclData;
}

class GCLUpdateDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_switchId,
        FIELD_portId,
        FIELD_gclData,
    };
  public:
    GCLUpdateDescriptor();
    virtual ~GCLUpdateDescriptor();

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

Register_ClassDescriptor(GCLUpdateDescriptor)

GCLUpdateDescriptor::GCLUpdateDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(GCLUpdate)), "omnetpp::cMessage")
{
    propertyNames = nullptr;
}

GCLUpdateDescriptor::~GCLUpdateDescriptor()
{
    delete[] propertyNames;
}

bool GCLUpdateDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<GCLUpdate *>(obj)!=nullptr;
}

const char **GCLUpdateDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *GCLUpdateDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int GCLUpdateDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 3+base->getFieldCount() : 3;
}

unsigned int GCLUpdateDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_switchId
        FD_ISEDITABLE,    // FIELD_portId
        FD_ISEDITABLE,    // FIELD_gclData
    };
    return (field >= 0 && field < 3) ? fieldTypeFlags[field] : 0;
}

const char *GCLUpdateDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "switchId",
        "portId",
        "gclData",
    };
    return (field >= 0 && field < 3) ? fieldNames[field] : nullptr;
}

int GCLUpdateDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "switchId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "portId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "gclData") == 0) return baseIndex + 2;
    return base ? base->findField(fieldName) : -1;
}

const char *GCLUpdateDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_switchId
        "int",    // FIELD_portId
        "string",    // FIELD_gclData
    };
    return (field >= 0 && field < 3) ? fieldTypeStrings[field] : nullptr;
}

const char **GCLUpdateDescriptor::getFieldPropertyNames(int field) const
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

const char *GCLUpdateDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int GCLUpdateDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void GCLUpdateDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'GCLUpdate'", field);
    }
}

const char *GCLUpdateDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string GCLUpdateDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        case FIELD_switchId: return long2string(pp->getSwitchId());
        case FIELD_portId: return long2string(pp->getPortId());
        case FIELD_gclData: return oppstring2string(pp->getGclData());
        default: return "";
    }
}

void GCLUpdateDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        case FIELD_switchId: pp->setSwitchId(string2long(value)); break;
        case FIELD_portId: pp->setPortId(string2long(value)); break;
        case FIELD_gclData: pp->setGclData((value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'GCLUpdate'", field);
    }
}

omnetpp::cValue GCLUpdateDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        case FIELD_switchId: return pp->getSwitchId();
        case FIELD_portId: return pp->getPortId();
        case FIELD_gclData: return pp->getGclData();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'GCLUpdate' as cValue -- field index out of range?", field);
    }
}

void GCLUpdateDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        case FIELD_switchId: pp->setSwitchId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_portId: pp->setPortId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_gclData: pp->setGclData(value.stringValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'GCLUpdate'", field);
    }
}

const char *GCLUpdateDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr GCLUpdateDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void GCLUpdateDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    GCLUpdate *pp = omnetpp::fromAnyPtr<GCLUpdate>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'GCLUpdate'", field);
    }
}

Register_Class(FlowSchedule)

FlowSchedule::FlowSchedule(const char *name, short kind) : ::omnetpp::cMessage(name, kind)
{
}

FlowSchedule::FlowSchedule(const FlowSchedule& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

FlowSchedule::~FlowSchedule()
{
}

FlowSchedule& FlowSchedule::operator=(const FlowSchedule& other)
{
    if (this == &other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void FlowSchedule::copy(const FlowSchedule& other)
{
    this->flowId = other.flowId;
    this->priority = other.priority;
    this->timeSlot = other.timeSlot;
    this->expectedDelay = other.expectedDelay;
}

void FlowSchedule::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->flowId);
    doParsimPacking(b,this->priority);
    doParsimPacking(b,this->timeSlot);
    doParsimPacking(b,this->expectedDelay);
}

void FlowSchedule::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->flowId);
    doParsimUnpacking(b,this->priority);
    doParsimUnpacking(b,this->timeSlot);
    doParsimUnpacking(b,this->expectedDelay);
}

int FlowSchedule::getFlowId() const
{
    return this->flowId;
}

void FlowSchedule::setFlowId(int flowId)
{
    this->flowId = flowId;
}

int FlowSchedule::getPriority() const
{
    return this->priority;
}

void FlowSchedule::setPriority(int priority)
{
    this->priority = priority;
}

int FlowSchedule::getTimeSlot() const
{
    return this->timeSlot;
}

void FlowSchedule::setTimeSlot(int timeSlot)
{
    this->timeSlot = timeSlot;
}

double FlowSchedule::getExpectedDelay() const
{
    return this->expectedDelay;
}

void FlowSchedule::setExpectedDelay(double expectedDelay)
{
    this->expectedDelay = expectedDelay;
}

class FlowScheduleDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_flowId,
        FIELD_priority,
        FIELD_timeSlot,
        FIELD_expectedDelay,
    };
  public:
    FlowScheduleDescriptor();
    virtual ~FlowScheduleDescriptor();

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

Register_ClassDescriptor(FlowScheduleDescriptor)

FlowScheduleDescriptor::FlowScheduleDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(FlowSchedule)), "omnetpp::cMessage")
{
    propertyNames = nullptr;
}

FlowScheduleDescriptor::~FlowScheduleDescriptor()
{
    delete[] propertyNames;
}

bool FlowScheduleDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<FlowSchedule *>(obj)!=nullptr;
}

const char **FlowScheduleDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *FlowScheduleDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int FlowScheduleDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 4+base->getFieldCount() : 4;
}

unsigned int FlowScheduleDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_flowId
        FD_ISEDITABLE,    // FIELD_priority
        FD_ISEDITABLE,    // FIELD_timeSlot
        FD_ISEDITABLE,    // FIELD_expectedDelay
    };
    return (field >= 0 && field < 4) ? fieldTypeFlags[field] : 0;
}

const char *FlowScheduleDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "flowId",
        "priority",
        "timeSlot",
        "expectedDelay",
    };
    return (field >= 0 && field < 4) ? fieldNames[field] : nullptr;
}

int FlowScheduleDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "flowId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "priority") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "timeSlot") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "expectedDelay") == 0) return baseIndex + 3;
    return base ? base->findField(fieldName) : -1;
}

const char *FlowScheduleDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_flowId
        "int",    // FIELD_priority
        "int",    // FIELD_timeSlot
        "double",    // FIELD_expectedDelay
    };
    return (field >= 0 && field < 4) ? fieldTypeStrings[field] : nullptr;
}

const char **FlowScheduleDescriptor::getFieldPropertyNames(int field) const
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

const char *FlowScheduleDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int FlowScheduleDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void FlowScheduleDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'FlowSchedule'", field);
    }
}

const char *FlowScheduleDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string FlowScheduleDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: return long2string(pp->getFlowId());
        case FIELD_priority: return long2string(pp->getPriority());
        case FIELD_timeSlot: return long2string(pp->getTimeSlot());
        case FIELD_expectedDelay: return double2string(pp->getExpectedDelay());
        default: return "";
    }
}

void FlowScheduleDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: pp->setFlowId(string2long(value)); break;
        case FIELD_priority: pp->setPriority(string2long(value)); break;
        case FIELD_timeSlot: pp->setTimeSlot(string2long(value)); break;
        case FIELD_expectedDelay: pp->setExpectedDelay(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'FlowSchedule'", field);
    }
}

omnetpp::cValue FlowScheduleDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: return pp->getFlowId();
        case FIELD_priority: return pp->getPriority();
        case FIELD_timeSlot: return pp->getTimeSlot();
        case FIELD_expectedDelay: return pp->getExpectedDelay();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'FlowSchedule' as cValue -- field index out of range?", field);
    }
}

void FlowScheduleDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        case FIELD_flowId: pp->setFlowId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_priority: pp->setPriority(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_timeSlot: pp->setTimeSlot(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_expectedDelay: pp->setExpectedDelay(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'FlowSchedule'", field);
    }
}

const char *FlowScheduleDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr FlowScheduleDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void FlowScheduleDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    FlowSchedule *pp = omnetpp::fromAnyPtr<FlowSchedule>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'FlowSchedule'", field);
    }
}

ControlMessage_Base::ControlMessage_Base(const char *name, short kind) : ::omnetpp::cMessage(name, kind)
{
    take(&this->gclUpdate);
    take(&this->flowSchedule);
}

ControlMessage_Base::ControlMessage_Base(const ControlMessage_Base& other) : ::omnetpp::cMessage(other)
{
    take(&this->gclUpdate);
    take(&this->flowSchedule);
    copy(other);
}

ControlMessage_Base::~ControlMessage_Base()
{
    drop(&this->gclUpdate);
    drop(&this->flowSchedule);
}

ControlMessage_Base& ControlMessage_Base::operator=(const ControlMessage_Base& other)
{
    if (this == &other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void ControlMessage_Base::copy(const ControlMessage_Base& other)
{
    this->msgType = other.msgType;
    this->timestamp = other.timestamp;
    this->gclUpdate = other.gclUpdate;
    this->gclUpdate.setName(other.gclUpdate.getName());
    this->flowSchedule = other.flowSchedule;
    this->flowSchedule.setName(other.flowSchedule.getName());
}

void ControlMessage_Base::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->msgType);
    doParsimPacking(b,this->timestamp);
    doParsimPacking(b,this->gclUpdate);
    doParsimPacking(b,this->flowSchedule);
}

void ControlMessage_Base::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->msgType);
    doParsimUnpacking(b,this->timestamp);
    doParsimUnpacking(b,this->gclUpdate);
    doParsimUnpacking(b,this->flowSchedule);
}

int ControlMessage_Base::getMsgType() const
{
    return this->msgType;
}

void ControlMessage_Base::setMsgType(int msgType)
{
    this->msgType = msgType;
}

double ControlMessage_Base::getTimestamp() const
{
    return this->timestamp;
}

void ControlMessage_Base::setTimestamp(double timestamp)
{
    this->timestamp = timestamp;
}

const GCLUpdate& ControlMessage_Base::getGclUpdate() const
{
    return this->gclUpdate;
}

void ControlMessage_Base::setGclUpdate(const GCLUpdate& gclUpdate)
{
    this->gclUpdate = gclUpdate;
}

const FlowSchedule& ControlMessage_Base::getFlowSchedule() const
{
    return this->flowSchedule;
}

void ControlMessage_Base::setFlowSchedule(const FlowSchedule& flowSchedule)
{
    this->flowSchedule = flowSchedule;
}

class ControlMessageDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_msgType,
        FIELD_timestamp,
        FIELD_gclUpdate,
        FIELD_flowSchedule,
    };
  public:
    ControlMessageDescriptor();
    virtual ~ControlMessageDescriptor();

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

Register_ClassDescriptor(ControlMessageDescriptor)

ControlMessageDescriptor::ControlMessageDescriptor() : omnetpp::cClassDescriptor("ControlMessage", "omnetpp::cMessage")
{
    propertyNames = nullptr;
}

ControlMessageDescriptor::~ControlMessageDescriptor()
{
    delete[] propertyNames;
}

bool ControlMessageDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<ControlMessage_Base *>(obj)!=nullptr;
}

const char **ControlMessageDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = { "customize",  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *ControlMessageDescriptor::getProperty(const char *propertyName) const
{
    if (!strcmp(propertyName, "customize")) return "true";
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int ControlMessageDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 4+base->getFieldCount() : 4;
}

unsigned int ControlMessageDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_msgType
        FD_ISEDITABLE,    // FIELD_timestamp
        FD_ISCOMPOUND | FD_ISCOBJECT | FD_ISCOWNEDOBJECT,    // FIELD_gclUpdate
        FD_ISCOMPOUND | FD_ISCOBJECT | FD_ISCOWNEDOBJECT,    // FIELD_flowSchedule
    };
    return (field >= 0 && field < 4) ? fieldTypeFlags[field] : 0;
}

const char *ControlMessageDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "msgType",
        "timestamp",
        "gclUpdate",
        "flowSchedule",
    };
    return (field >= 0 && field < 4) ? fieldNames[field] : nullptr;
}

int ControlMessageDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "msgType") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "timestamp") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "gclUpdate") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "flowSchedule") == 0) return baseIndex + 3;
    return base ? base->findField(fieldName) : -1;
}

const char *ControlMessageDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_msgType
        "double",    // FIELD_timestamp
        "GCLUpdate",    // FIELD_gclUpdate
        "FlowSchedule",    // FIELD_flowSchedule
    };
    return (field >= 0 && field < 4) ? fieldTypeStrings[field] : nullptr;
}

const char **ControlMessageDescriptor::getFieldPropertyNames(int field) const
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

const char *ControlMessageDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int ControlMessageDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void ControlMessageDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'ControlMessage_Base'", field);
    }
}

const char *ControlMessageDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string ControlMessageDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        case FIELD_msgType: return long2string(pp->getMsgType());
        case FIELD_timestamp: return double2string(pp->getTimestamp());
        case FIELD_gclUpdate: return pp->getGclUpdate().str();
        case FIELD_flowSchedule: return pp->getFlowSchedule().str();
        default: return "";
    }
}

void ControlMessageDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        case FIELD_msgType: pp->setMsgType(string2long(value)); break;
        case FIELD_timestamp: pp->setTimestamp(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ControlMessage_Base'", field);
    }
}

omnetpp::cValue ControlMessageDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        case FIELD_msgType: return pp->getMsgType();
        case FIELD_timestamp: return pp->getTimestamp();
        case FIELD_gclUpdate: return omnetpp::toAnyPtr(&pp->getGclUpdate()); break;
        case FIELD_flowSchedule: return omnetpp::toAnyPtr(&pp->getFlowSchedule()); break;
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'ControlMessage_Base' as cValue -- field index out of range?", field);
    }
}

void ControlMessageDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        case FIELD_msgType: pp->setMsgType(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_timestamp: pp->setTimestamp(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ControlMessage_Base'", field);
    }
}

const char *ControlMessageDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_gclUpdate: return omnetpp::opp_typename(typeid(GCLUpdate));
        case FIELD_flowSchedule: return omnetpp::opp_typename(typeid(FlowSchedule));
        default: return nullptr;
    };
}

omnetpp::any_ptr ControlMessageDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        case FIELD_gclUpdate: return omnetpp::toAnyPtr(&pp->getGclUpdate()); break;
        case FIELD_flowSchedule: return omnetpp::toAnyPtr(&pp->getFlowSchedule()); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void ControlMessageDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    ControlMessage_Base *pp = omnetpp::fromAnyPtr<ControlMessage_Base>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ControlMessage_Base'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

