//
// SPDX-FileCopyrightText: 2020 Thomas Deinlein <thomas.deinlein@fau.de>
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/stack/sdap/layer/NRsdapGNB.h"

Define_Module(NRsdapGNB);

void NRsdapGNB::initialize(int stage) {

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

    }

}

void NRsdapGNB::handleMessage(cMessage *msg) {

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

void NRsdapGNB::handleSelfMessage(cMessage *msg) {

    //std::cout << "NRsdap::handleSelfMessage start at " << simTime().dbl() << std::endl;
	//TODO
    //std::cout << "NRsdap::handleSelfMessage end at " << simTime().dbl() << std::endl;

}

//incoming messages from IP2NR, to pdcp
void NRsdapGNB::fromUpperToLower(cMessage *msg) {

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

    //nodeid is nodeB-Id, destid is ue
    NRSdapEntity* entity = getEntity(nodeId_, destId, (ApplicationType)(lteInfo->getApplication()));

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
void NRsdapGNB::fromLowerToUpper(cMessage *msg) {

    //std::cout << "NRsdap::fromLowerToUpper start at " << simTime().dbl() << std::endl;

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    SdapPdu * sdapPkt = check_and_cast<SdapPdu*>(pkt);
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            sdapPkt->removeControlInfo());
    cPacket* upPkt = sdapPkt->decapsulate();
    delete sdapPkt;

    upPkt->setControlInfo(lteInfo);

    send(upPkt, upperLayer);

    //std::cout << "NRsdap::fromLowerToUpper end at " << simTime().dbl() << std::endl;
}

