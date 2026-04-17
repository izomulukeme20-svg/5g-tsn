#include "TsnEtherAdapter.h"

Define_Module(TsnEtherAdapter);

TsnEtherAdapter::TsnEtherAdapter() :
    txEndSelfMsg(nullptr)
{
}

TsnEtherAdapter::~TsnEtherAdapter() {
    cancelAndDelete(txEndSelfMsg);
    while (!txQueue.isEmpty()) {
        Packet *pkt = check_and_cast<Packet*>(txQueue.pop());
        delete pkt;
    }
}

void TsnEtherAdapter::initialize() {
    txEndSelfMsg = new cMessage("TsnEtherTxEnd");
    txQueue.setName("TsnEtherTxQueue");
}

void TsnEtherAdapter::handleMessage(cMessage *msg) {
    if (msg == txEndSelfMsg) {
        if (!txQueue.isEmpty()) {
            Packet *pkt = check_and_cast<Packet*>(txQueue.pop());
            sendPkt(pkt);
        }
        return;
    }

    Packet *pkt = check_and_cast<Packet*>(msg);

    if (msg->arrivedOn("tsnPort$i")) {
        queueOrSendPkt(pkt, "ethg$o");
    }
    else if (msg->arrivedOn("ethg$i")) {
        queueOrSendPkt(pkt, "tsnPort$o");
    }
}

void TsnEtherAdapter::queueOrSendPkt(Packet *pkt, const char *gateName) {
    cGate *outGate = gate(gateName);
    cChannel *channel = outGate->getTransmissionChannel();
    simtime_t txFinishTime = channel->getTransmissionFinishTime();

    if (txFinishTime <= simTime()) {
        sendPkt(pkt, gateName);
    } else {
        pkt->setName(gateName);
        txQueue.insert(pkt);
    }
}

void TsnEtherAdapter::sendPkt(Packet *pkt, const char *gateName) {
    send(pkt, gateName);
    cGate *outGate = gate(gateName);
    cChannel *channel = outGate->getTransmissionChannel();
    simtime_t txFinishTime = channel->getTransmissionFinishTime();
    scheduleAt(txFinishTime, txEndSelfMsg);
}

void TsnEtherAdapter::sendPkt(Packet *pkt) {
    const char *gateName = pkt->getName();
    sendPkt(pkt, gateName);
}

void TsnEtherAdapter::finish() {
}
