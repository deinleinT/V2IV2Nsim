//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nürnberg (FAU),
// Computer Science 7 - Computer Networks and Communication Systems
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

#include "nr/stack/pdcp_rrc/layer/NRPdcpRrcUe.h"

Define_Module(NRPdcpRrcUe);

void NRPdcpRrcUe::initialize(int stage){
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        qosHandler = check_and_cast<QosHandlerUE*>(
                getParentModule()->getSubmodule("qosHandler"));
    }
    nodeId_ = getAncestorPar("macNodeId");
}

void NRPdcpRrcUe::handleMessage(cMessage *msg) {

    //std::cout << "NRPdcpRrcUe::handleMessage start at " << simTime().dbl() << std::endl;

    LtePdcpRrcUe::handleMessage(msg);

    //std::cout << "NRPdcpRrcUe::handleMessage end at " << simTime().dbl() << std::endl;
}

//rlc to sdap
void NRPdcpRrcUe::toDataPort(cPacket *pkt) {

    //std::cout << "NRPdcpRrcUe::toDataPort start at " << simTime().dbl() << std::endl;

    if (strcmp(pkt->getName(), "RRC") == 0) {
        cGate * tmpGate = gate("upperLayerRRC$o");
        send(pkt, tmpGate);
        return;
    }

    emit(receivedPacketFromLowerLayer, pkt);
    LtePdcpPdu* pdcpPkt = check_and_cast<LtePdcpPdu*>(pkt);
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            pdcpPkt->removeControlInfo());

    //EV << "NRPdcpRrcUe : Received packet with CID " << lteInfo->getLcid() << "\n";
    //EV << "NRPdcpRrcUe : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    cPacket* upPkt = pdcpPkt->decapsulate(); // Decapsulate packet
    delete pdcpPkt;

    headerDecompress(upPkt, lteInfo->getHeaderSize()); // Decompress packet header
    //handleControlInfo(upPkt, lteInfo);

    //EV << "NRPdcpRrcUe : Sending packet " << upPkt->getName() << " on port DataPort$o\n";

    upPkt->setControlInfo(lteInfo);

    // Send message
    send(upPkt, dataPort_[OUT]);
    //sendDelayed(upPkt,uniform(NOW.dbl(),NOW.dbl()+uniform(0,upPkt->getByteLength()/10000)),dataPort_[OUT]);
    emit(sentPacketToUpperLayer, upPkt);

    //std::cout << "NRPdcpRrcUe::toDataPort end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcUe::fromDataPort(cPacket *pkt) {

    //std::cout << "NRPdcpRrcUe::fromDataPort start at " << simTime().dbl() << std::endl;

    if (strcmp(pkt->getName(), "RRC") == 0) {
        cGate * tmpGate = gate("lowerLayerRRC$o");
        send(pkt, tmpGate);
        return;
    }

    emit(receivedPacketFromUpperLayer, pkt);

    // Control Informations
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            pkt->removeControlInfo());

    setTrafficInformation(pkt, lteInfo);
    //lteInfo->setDestId(getDestId(lteInfo));
    headerCompress(pkt, lteInfo->getHeaderSize()); // header compression

    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
            lteInfo->getSrcPort(), lteInfo->getDstPort(),
            lteInfo->getDirection(), lteInfo->getApplication())) == 0xFFFF) {

        mylcid = lcid_++;

        //EV << "LteRrc : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
                lteInfo->getSrcPort(), lteInfo->getDstPort(),
                lteInfo->getDirection(), mylcid, lteInfo->getApplication());

        unsigned int key = idToMacCid(nodeId_, mylcid);

        if (qosHandler->getQosInfo().find(key)
                == qosHandler->getQosInfo().end()) {
            QosInfo qosinfo;
            qosinfo.destNodeId = lteInfo->getDestId();
            qosinfo.destAddress = IPv4Address(lteInfo->getDstAddr());
            qosinfo.appType = (ApplicationType) lteInfo->getApplication();
            qosinfo.qfi = lteInfo->getQfi();
            qosinfo.rlcType = lteInfo->getRlcType();
            qosinfo.radioBearerId = lteInfo->getRadioBearerId();
            qosinfo.senderAddress = IPv4Address(lteInfo->getSrcAddr());
            qosinfo.senderNodeId = lteInfo->getSourceId();
            qosinfo.lcid = mylcid;
            qosinfo.cid = key;
            qosinfo.trafficClass = (LteTrafficClass)lteInfo->getTraffic();
            qosHandler->getQosInfo()[key] = qosinfo;
        }
    }

    //EV << "LteRrc : Assigned Lcid: " << mylcid << "\n";
    //EV << "LteRrc : Assigned Node ID: " << nodeId_ << "\n";

    // get the PDCP entity for this LCID
    LtePdcpEntity* entity = getEntity(mylcid, nodeId_);

    // get the sequence number for this PDCP SDU.
    // Note that the numbering depends on the entity the packet is associated to.
    unsigned int sno = entity->nextSequenceNumber();

    // set sequence number
    lteInfo->setSequenceNumber(sno);
    // NOTE setLcid and setSourceId have been anticipated for using in "ctrlInfoToMacCid" function
    lteInfo->setLcid(mylcid);
    lteInfo->setCid(idToMacCid(lteInfo->getSourceId(), mylcid));
    lteInfo->setSourceId(nodeId_);
    lteInfo->setContainsSeveralCids(false);

    // PDCP Packet creation
    LtePdcpPdu* pdcpPkt = new LtePdcpPdu("LtePdcpPdu");
    pdcpPkt->setByteLength(
            lteInfo->getRlcType() == UM ? PDCP_HEADER_UM : PDCP_HEADER_AM);
    pdcpPkt->setKind(lteInfo->getApplication());
    pdcpPkt->encapsulate(pkt);

    //EV << "NRPdcpRrcUe : Preparing to send " << lteTrafficClassToA((LteTrafficClass) lteInfo->getTraffic()) << " traffic\n";
    //EV << "NRPdcpRrcUe : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    pdcpPkt->setControlInfo(lteInfo);

    //EV << "NRPdcpRrcUe : Sending packet " << pdcpPkt->getName() << " on port " << (lteInfo->getRlcType() == UM ? "UM_Sap$o\n" : "AM_Sap$o\n");

    // Send message
    send(pdcpPkt, (lteInfo->getRlcType() == UM ? umSap_[OUT] : amSap_[OUT]));

    emit(sentPacketToLowerLayer, pdcpPkt);

    //std::cout << "NRPdcpRrcUe::fromDataPort end at " << simTime().dbl() << std::endl;
}
