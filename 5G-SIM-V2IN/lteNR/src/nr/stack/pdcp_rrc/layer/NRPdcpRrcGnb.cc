//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU), Computer Science 7 - Computer Networks and Communication Systems
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

#include "nr/stack/pdcp_rrc/layer/NRPdcpRrcGnb.h"

Define_Module(NRPdcpRrcGnb);

void NRPdcpRrcGnb::initialize(int stage) {
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        qosHandler = check_and_cast<QosHandlerGNB*>(
                getParentModule()->getSubmodule("qosHandler"));
    }
    nodeId_ = getAncestorPar("macNodeId");
}

void NRPdcpRrcGnb::handleMessage(cMessage *msg) {

    //std::cout << "NRPdcpRrcGnb::handleMessage start at " << simTime().dbl() << std::endl;

    LtePdcpRrcEnb::handleMessage(msg);

    //std::cout << "NRPdcpRrcGnb::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcGnb::toDataPort(cPacket *pkt) {

    //std::cout << "NRPdcpRrcGnb::toDataPort start at " << simTime().dbl() << std::endl;

    if (strcmp(pkt->getName(), "RRC") == 0) {
        cGate * tmpGate = gate("upperLayerRRC$o");
        send(pkt, tmpGate);

        return;
    }

    emit(receivedPacketFromLowerLayer, pkt);
    LtePdcpPdu* pdcpPkt = check_and_cast<LtePdcpPdu*>(pkt);
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            pdcpPkt->removeControlInfo());

    //EV << "NRPdcpRrcGnb : Received packet with CID " << lteInfo->getLcid() << "\n";
    //EV << "NRPdcpRrcGnb : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    cPacket* upPkt = pdcpPkt->decapsulate(); // Decapsulate packet
    delete pdcpPkt;

    headerDecompress(upPkt, lteInfo->getHeaderSize()); // Decompress packet header
    //handleControlInfo(upPkt, lteInfo);

    //EV << "NRPdcpRrcGnb : Sending packet " << upPkt->getName() << " on port DataPort$o\n";

    upPkt->setControlInfo(lteInfo);

	if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
		sendDelayed(upPkt, uniform(0, upPkt->getByteLength() / 10e5), dataPort_[OUT]);
	} else {
		// Send message
		send(upPkt, dataPort_[OUT]);
	}

    emit(sentPacketToUpperLayer, upPkt);

    //std::cout << "NRPdcpRrcGnb::toDataPort end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcGnb::fromDataPort(cPacket *pkt) {

    //std::cout << "NRPdcpRrcGnb::fromDataPort start at " << simTime().dbl() << std::endl;

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
    lteInfo->setDestId(getDestId(lteInfo));
    headerCompress(pkt, lteInfo->getHeaderSize()); // header compression

    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
            lteInfo->getSrcPort(), lteInfo->getDstPort(),
            lteInfo->getDirection(), lteInfo->getApplication())) == 0xFFFF) {

        mylcid = lcid_++;

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
                lteInfo->getSrcPort(), lteInfo->getDstPort(),
                lteInfo->getDirection(), mylcid, lteInfo->getApplication());

        unsigned int key = idToMacCid(lteInfo->getDestId(), mylcid);

        if (qosHandler->getQosInfo().find(key)
                == qosHandler->getQosInfo().end()) {
            QosInfo qosinfo(DL);
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
            qosinfo.trafficClass = (LteTrafficClass) lteInfo->getTraffic();
//            qosHandler->getQosInfo()[key] = qosinfo;
            qosHandler->insertQosInfo(key, qosinfo);

        }
    }

    //EV << "LteRrc : Assigned Lcid: " << mylcid << "\n";
    //EV << "LteRrc : Assigned Node ID: " << nodeId_ << "\n";

    // get the PDCP entity for this LCID
    LtePdcpEntity* entity = getEntity(mylcid,lteInfo->getDestId());

    // get the sequence number for this PDCP SDU.
    // Note that the numbering depends on the entity the packet is associated to.
    unsigned int sno = entity->nextSequenceNumber();

    // set sequence number
    lteInfo->setSequenceNumber(sno);
    // NOTE setLcid and setSourceId have been anticipated for using in "ctrlInfoToMacCid" function
    lteInfo->setLcid(mylcid);
    lteInfo->setSourceId(nodeId_);
    lteInfo->setCid(idToMacCid(lteInfo->getDestId(), mylcid));
    lteInfo->setContainsSeveralCids(false);

    // PDCP Packet creation
    LtePdcpPdu* pdcpPkt = new LtePdcpPdu("LtePdcpPdu");
    pdcpPkt->setByteLength(
            lteInfo->getRlcType() == UM ? PDCP_HEADER_UM : PDCP_HEADER_AM);
    //pdcpPkt->setByteLength(PDCP_HEADER_UM);
    pdcpPkt->setKind(lteInfo->getApplication());
    pdcpPkt->encapsulate(pkt);

    //EV << "NRPdcpRrcGnb : Preparing to send " << lteTrafficClassToA((LteTrafficClass) lteInfo->getTraffic()) << " traffic\n";
    //EV << "NRPdcpRrcGnb : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    pdcpPkt->setControlInfo(lteInfo);

    //EV << "NRPdcpRrcGnb : Sending packet " << pdcpPkt->getName() << " on port " << (lteInfo->getRlcType() == UM ? "UM_Sap$o\n" : "AM_Sap$o\n");

	if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
		sendDelayed(pdcpPkt, uniform(0, pdcpPkt->getByteLength() / 10e5), (lteInfo->getRlcType() == UM ? umSap_[OUT] : amSap_[OUT]));
	} else {
		// Send message
		send(pdcpPkt, (lteInfo->getRlcType() == UM ? umSap_[OUT] : amSap_[OUT]));
	}

    emit(sentPacketToLowerLayer, pdcpPkt);

    //std::cout << "NRPdcpRrcGnb::fromDataPort end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcGnb::resetConnectionTable(MacNodeId masterId, MacNodeId nodeId) {

    //std::cout << "NRPdcpRrcGnb::resetConnectionTable start at " << simTime().dbl() << std::endl;

    IPv4Address adress = binder_->getIPAddressByMacNodeId(nodeId);

    if (adress.isUnspecified())
        return;

    //dir --> UL: UE is source, gNB is dest
    //    --> DL: UD is dest, gNB is

    PdcpEntities::const_iterator it;
    for(it = entities_.begin();it != entities_.end();){
        if (nodeId == it->second->getUeId()) {
            delete it->second;
            it = entities_.erase(it);
        } else {
            ++it;
        }
    }


    ht_->erase_entry(adress.getInt());

    //std::cout << "NRPdcpRrcGnb::resetConnectionTable end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcGnb::exchangeConnection(MacNodeId nodeId, MacNodeId oldMasterId, MacNodeId newMasterId,NRPdcpRrcGnb *oldMasterPdcp, NRPdcpRrcGnb *newMasterPdcp){

    //nodeId is ue
    //oldMasterId is this
    //newMasterId is newNodeB

    IPv4Address adress = binder_->getIPAddressByMacNodeId(nodeId);

    //pdcp entities
    PdcpEntities oldEntities = oldMasterPdcp->getEntities();

    for(auto & var: oldEntities){
        if(var.second->getUeId() == nodeId){
            // copy this entry to new Master
            LtePdcpEntity * tmp = new LtePdcpEntity(*(var.second));
            newMasterPdcp->getEntities().insert(std::make_pair(var.first,tmp));

        }
    }

    //connection Table
        ConnectionsTable * oldCN = oldMasterPdcp->getCNTable();;

    for (auto & varOld : oldCN->getEntries()) {
        if (varOld.dstAddr_ == adress.getInt()
                || varOld.srcAddr_ == adress.getInt()) {
            //copy this entry to new master
            newMasterPdcp->getCNTable()->getEntries().push_back(varOld);
            //            if(varOld.lcid_ > newMasterPdcp->getLcid()){
            //                newMasterPdcp->setLcid(++varOld.lcid_);
            //            }
        }
    }
}
