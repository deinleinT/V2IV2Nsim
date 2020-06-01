/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/stack/sdap/layer/NRsdapUE.h"

Define_Module(NRsdapUE);


void NRsdapUE::initialize(int stage) {

    if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        upperLayer = gate("upperLayer$o");
        lowerLayer = gate("lowerLayer$o");
        upperLayerIn = gate("upperLayer$i");
        lowerLayerIn = gate("lowerLayer$i");

        fromUpperLayer = registerSignal("fromUpperLayer");
        fromLowerLayer = registerSignal("fromLowerLayer");
        toUpperLayer = registerSignal("toUpperLayer");
        toLowerLayer = registerSignal("toLowerLayer");
        pkdrop = registerSignal("pkdrop");



        qosHandler = check_and_cast<QosHandler*>(
                getParentModule()->getSubmodule("qosHandler"));

        nodeType = qosHandler->getNodeType();
        nodeId_ = getAncestorPar("macNodeId");
        hoErrorCount = 0;
        WATCH(nodeId_);

//    cModule * mod = getParentModule()->getSubmodule("rrc");
//
//    if (strcmp(mod->par("nodeType").stringValue(), "UE") == 0) {
//        nodeType = UE;
//    } else if (strcmp(mod->par("nodeType").stringValue(), "GNODEB") == 0) {
//        nodeType = GNODEB;
//    } else {
//        throw cRuntimeError("Unknown NodeType");
//    }

    }

}

void NRsdapUE::handleMessage(cMessage *msg) {

    //std::cout << "NRsdap::handleMessage start at " << simTime().dbl() << std::endl;

    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
    } else if (strcmp(msg->getArrivalGate()->getBaseName(), "upperLayer")
            == 0) {

        fromUpperToLower(msg);

    } else if (strcmp(msg->getArrivalGate()->getBaseName(), "lowerLayer")
            == 0) {

        fromLowerToUpper(msg);

    }

    //std::cout << "NRsdap::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRsdapUE::handleSelfMessage(cMessage *msg) {

    //std::cout << "NRsdap::handleSelfMessage start at " << simTime().dbl() << std::endl;

    //std::cout << "NRsdap::handleSelfMessage end at " << simTime().dbl() << std::endl;

}

//incoming messages from IP2NR, to pdcp
void NRsdapUE::fromUpperToLower(cMessage *msg) {

    //std::cout << "NRsdap::fromUpperToLower start at " << simTime().dbl() << std::endl;

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    FlowControlInfo * lteInfo = check_and_cast<FlowControlInfo*>(
            pkt->removeControlInfo());
    setTrafficInformation(pkt, lteInfo);
    // dest id
    MacNodeId destId = getNRBinder()->getMacNodeId(
            IPv4Address(lteInfo->getDstAddr()));
    // master of this ue (myself or a relay)
    MacNodeId master = getNRBinder()->getNextHop(destId);
    if (master != nodeId_) {
        destId = master;
    }

    if (nodeType == UE) {
        nodeId_ = getBinder()->getMacNodeId(IPv4Address(lteInfo->getSrcAddr()));
        destId = getNRBinder()->getNextHop(nodeId_);
    }

    if ((destId >= ENB_MIN_ID && destId <= ENB_MAX_ID) && (nodeId_ >= ENB_MIN_ID && nodeId_ <= ENB_MAX_ID)) {
        hoErrorCount++;
        delete msg;
        return;
    }

    //nodeid is ue, destid is nodeB
    NRSdapEntity* entity = getEntity(nodeId_, destId, (ApplicationType)(lteInfo->getApplication()));

    //unsigned int sno = entity->nextSequenceNumber();

    //lteInfo->setSequenceNumber(sno);

    lteInfo->setDestId(destId);

    lteInfo->setSourceId(nodeId_);

    lteInfo->setHeaderSize(1);

    SdapPdu * sdapPkt = new SdapPdu(pkt->getName());

    sdapPkt->setKind(lteInfo->getApplication());

    sdapPkt->encapsulate(pkt);

    sdapPkt->setControlInfo(lteInfo);

    send(sdapPkt, lowerLayer);

    //std::cout << "NRsdap::fromUpperToLower end at " << simTime().dbl() << std::endl;
}

//incoming messages from pdcp, to IP2NR
void NRsdapUE::fromLowerToUpper(cMessage *msg) {

    //std::cout << "NRsdap::fromLowerToUpper start at " << simTime().dbl() << std::endl;

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    SdapPdu * sdapPkt = check_and_cast<SdapPdu*>(pkt);
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            sdapPkt->removeControlInfo());
    cPacket* upPkt = sdapPkt->decapsulate();
    delete sdapPkt;
//    delete lteInfo;
    upPkt->setControlInfo(lteInfo);

    send(upPkt, upperLayer);

    //std::cout << "NRsdap::fromLowerToUpper end at " << simTime().dbl() << std::endl;
}

