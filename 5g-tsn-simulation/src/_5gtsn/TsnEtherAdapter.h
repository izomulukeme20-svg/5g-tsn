#ifndef TSNETHERADAPTER_H_
#define TSNETHERADAPTER_H_

#include <omnetpp.h>
#include "inet/common/packet/Packet.h"

using namespace omnetpp;
using namespace inet;

class TsnEtherAdapter : public cSimpleModule {
private:
    cMessage *txEndSelfMsg;
    cPacketQueue txQueue;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void queueOrSendPkt(Packet *pkt, const char *gateName);
    void sendPkt(Packet *pkt, const char *gateName);
    void sendPkt(Packet *pkt);

public:
    TsnEtherAdapter();
    virtual ~TsnEtherAdapter();
};

#endif /* TSNETHERADAPTER_H_ */
