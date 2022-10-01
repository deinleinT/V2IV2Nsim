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

#include "nr/stack/pdcp_rrc/layer/NRPdcpRrcUE.h"

#include "inet/transportlayer/common/L4Tools.h"
#include "nr/stack/pdcp_rrc/ConnectionsTableMod.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

Define_Module(NRPdcpRrcUE);

void NRPdcpRrcUE::initialize(int stage) {
	//NRPdcpRrcUe::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		dualConnectivityEnabled_ = getAncestorPar("dualConnectivityEnabled").boolValue();

		qosHandler = check_and_cast<QosHandlerUE*>(getParentModule()->getSubmodule("qosHandler"));
	}

	//local
	if (stage == inet::INITSTAGE_LAST) {
		delete ht_;
		ht_ = new ConnectionsTableMod();
	}

	//LtePdcpRrcBase::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		dataPort_[IN_GATE] = gate("DataPort$i");
		dataPort_[OUT_GATE] = gate("DataPort$o");
		eutranRrcSap_[IN_GATE] = gate("EUTRAN_RRC_Sap$i");
		eutranRrcSap_[OUT_GATE] = gate("EUTRAN_RRC_Sap$o");
		tmSap_[IN_GATE] = gate("TM_Sap$i", 0);
		tmSap_[OUT_GATE] = gate("TM_Sap$o", 0);
		umSap_[IN_GATE] = gate("UM_Sap$i", 0);
		umSap_[OUT_GATE] = gate("UM_Sap$o", 0);
		amSap_[IN_GATE] = gate("AM_Sap$i", 0);
		amSap_[OUT_GATE] = gate("AM_Sap$o", 0);

		binder_ = getNRBinder();
		headerCompressedSize_ = B(par("headerCompressedSize"));
		if (headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED && headerCompressedSize_ < MIN_COMPRESSED_HEADER_SIZE) {
			throw cRuntimeError("Size of compressed header must not be less than %i",
			MIN_COMPRESSED_HEADER_SIZE.get());
		}

		nodeId_ = getAncestorPar("macNodeId");

		// statistics
		receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
		receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
		sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
		sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

		WATCH(headerCompressedSize_);
		WATCH(nodeId_);
		WATCH(lcid_);
	}

	//LtePdcpRrcUeD2D::initialize(stage);
	if (stage == inet::INITSTAGE_NETWORK_LAYER) {
		nodeId_ = getAncestorPar("macNodeId");
		lcid_ = 10000;
	}
}

void NRPdcpRrcUE::resetConnectionTable(MacNodeId masterId, MacNodeId nodeId) {

	//std::cout << "NRPdcpRrcGnb::resetConnectionTable start at " << simTime().dbl() << std::endl;

	//dir --> UL: UE is source, gNB is dest
	//    --> DL: UE is dest, gNB is source

    PdcpTxEntities::const_iterator it;
    for (it = txEntities_.begin(); it != txEntities_.end();) {
        it->second->deleteModule();
        it = txEntities_.erase(it);
    }

    PdcpRxEntities::const_iterator its;
    for (its = rxEntities_.begin(); its != rxEntities_.end();) {
        its->second->deleteModule();
        its = rxEntities_.erase(its);
    }

	ht_->erase_entry(nodeId);
	delete ht_;
	ht_ = new ConnectionsTableMod();

	//std::cout << "NRPdcpRrcGnb::resetConnectionTable end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcUE::setTrafficInformation(cPacket * pkt, FlowControlInfo * lteInfo) {
	//std::cout << "LtePdcpRrcBase::setTrafficInformation start at " << simTime().dbl() << std::endl;

	std::string applName = std::string(pkt->getName());
	if ((strcmp(pkt->getName(), "VoIP") == 0) || (applName.find("VoIP") != std::string::npos) || (applName.find("voip") != std::string::npos)) {
		lteInfo->setApplication(VOIP);
		lteInfo->setTraffic(CONVERSATIONAL);
		lteInfo->setRlcType((int) par("conversationalRlc"));
	}
	else if ((strcmp(pkt->getName(), "gaming")) == 0) {
		lteInfo->setApplication(GAMING);
		lteInfo->setTraffic(INTERACTIVE);
		lteInfo->setRlcType((int) par("interactiveRlc"));
	}
	else if ((strcmp(pkt->getName(), "VoDPacket") == 0) || (strcmp(pkt->getName(), "VoDFinishPacket") == 0) || (strcmp(pkt->getName(), "Video") == 0)
			|| (applName.find("Video") != std::string::npos) || (applName.find("video") != std::string::npos)) {
		lteInfo->setApplication(VOD);
		lteInfo->setTraffic(STREAMING);
		lteInfo->setRlcType((int) par("streamingRlc"));
	}
	else if (strcmp(pkt->getName(), "V2X") == 0 || (applName.find("V2X") != std::string::npos) || (applName.find("v2x") != std::string::npos)) {
		lteInfo->setApplication(V2X);
		lteInfo->setTraffic(V2X_TRAFFIC);
		lteInfo->setRlcType((int) par("conversationalRlc"));
	}
	else if (strcmp(pkt->getName(), "Data") == 0 || strcmp(pkt->getName(), "Data-frag") == 0 || (applName.find("Data") != std::string::npos)
			|| (applName.find("data") != std::string::npos)) {
		lteInfo->setApplication(DATA_FLOW);
		lteInfo->setTraffic(BACKGROUND);
		lteInfo->setRlcType((int) par("backgroundRlc"));
	}
	else {
		lteInfo->setApplication(CBR);
		lteInfo->setTraffic(BACKGROUND);
		lteInfo->setRlcType((int) par("backgroundRlc"));
	}

	lteInfo->setDirection(LtePdcpRrcUe::getDirection());

	//std::cout << "LtePdcpRrcBase::setTrafficInformation end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcUE::handleMessage(cMessage * msg) {

	//std::cout << "NRPdcpRrcUe::handleMessage start at " << simTime().dbl() << std::endl;

	NRPdcpRrcUe::handleMessage(msg);

	//std::cout << "NRPdcpRrcUe::handleMessage end at " << simTime().dbl() << std::endl;
}

//rlc to sdap
void NRPdcpRrcUE::toDataPort(cPacket * pktIn) {

	//std::cout << "NRPdcpRrcUe::toDataPort start at " << simTime().dbl() << std::endl;

	Enter_Method_Silent
	("NRPdcpRrcGnb::toDataPort");

	emit(receivedPacketFromLowerLayer, pktIn);

	auto pkt = check_and_cast<Packet*>(pktIn);
	take(pkt);

	//headerDecompress(pkt);

	//EV << "NRPdcpRrcUe : Sending packet " << upPkt->getName() << " on port DataPort$o\n";

	pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

	if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
		sendDelayed(pkt, uniform(0, pkt->getByteLength() / 10e5), dataPort_[OUT_GATE]);
	}
	else {
		// Send message
		send(pkt, dataPort_[OUT_GATE]);
	}
	emit(sentPacketToUpperLayer, pkt);

	//std::cout << "NRPdcpRrcUe::toDataPort end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcUE::fromDataPort(cPacket * pktIn) {

	//std::cout << "NRPdcpRrcUe::fromDataPort start at " << simTime().dbl() << std::endl;

	emit(receivedPacketFromUpperLayer, pktIn);

	auto pkt = check_and_cast<inet::Packet*>(pktIn);
	auto lteInfo = pkt->getTag<FlowControlInfo>();

	setTrafficInformation(pkt, lteInfo);

	//headerCompress(pkt); // header compression

	LogicalCid mylcid;
	if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getApplication(), lteInfo->getDirection())) == 0xFFFF) {

		mylcid = lcid_++;

		ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getApplication(), lteInfo->getDirection(), mylcid);

		pkt->addTagIfAbsent<FlowControlInfo>()->setLcid(mylcid);
		unsigned int key = ctrlInfoToMacCid(lteInfo);

		if (qosHandler->getQosInfo().find(key) == qosHandler->getQosInfo().end()) {
			QosInfo qosinfo;
			qosinfo.destNodeId = lteInfo->getDestId();
			qosinfo.destAddress = Ipv4Address(lteInfo->getDstAddr());
			qosinfo.appType = (ApplicationType) lteInfo->getApplication();
			qosinfo.qfi = lteInfo->getQfi();
			qosinfo.rlcType = lteInfo->getRlcType();
			qosinfo.radioBearerId = lteInfo->getRadioBearerId();
			qosinfo.senderAddress = Ipv4Address(lteInfo->getSrcAddr());
			qosinfo.senderNodeId = lteInfo->getSourceId();
			qosinfo.lcid = mylcid;
			qosinfo.cid = key;
			qosinfo.trafficClass = (LteTrafficClass) lteInfo->getTraffic();
			qosinfo.dir = (Direction)lteInfo->getDirection();
            qosHandler->insertQosInfo(key, qosinfo);
			//qosHandler->getQosInfo()[key] = qosinfo;
		}
	}

	// obtain CID
//    MacNodeId destId = lteInfo->getDestId();
	MacCid cid = idToMacCid(nodeId_, mylcid);
	pkt->addTagIfAbsent<FlowControlInfo>()->setSourceId(nodeId_);
	pkt->addTagIfAbsent<FlowControlInfo>()->setCid(cid);
	pkt->addTagIfAbsent<FlowControlInfo>()->setLcid(mylcid);
	pkt->addTagIfAbsent<FlowControlInfo>()->setContainsSeveralCids(false);

	LteTxPdcpEntity *entity = getTxEntity(cid);

	entity->handlePacketFromUpperLayer(pkt);

	//std::cout << "NRPdcpRrcUe::fromDataPort end at " << simTime().dbl() << std::endl;
}

void NRPdcpRrcUE::sendToLowerLayer(Packet * pkt) {
	//std::cout << "NRPdcpRrcUe::sendToLowerLayer start at " << simTime().dbl() << std::endl;

	auto lteInfo = pkt->getTag<FlowControlInfo>();

	std::string portName;
	omnetpp::cGate *gate;
	switch (lteInfo->getRlcType()) {
	case UM:
		portName = "UM_Sap$o";
		gate = umSap_[OUT_GATE];
		break;
	case AM:
		portName = "AM_Sap$o";
		gate = amSap_[OUT_GATE];
		break;
	case TM:
		portName = "TM_Sap$o";
		gate = tmSap_[OUT_GATE];
		break;
	default:
		throw cRuntimeError("NRPdcpRrcGnb::sendToLowerLayer(): invalid RlcType %d", lteInfo->getRlcType());
	}

	// consider processing delay
	if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
		sendDelayed(pkt, uniform(0, pkt->getByteLength() / 10e5), gate);
	}
	else {
		send(pkt, gate);
	}

	emit(sentPacketToLowerLayer, pkt);

	//std::cout << "NRPdcpRrcUe::sendToLowerLayer end at " << simTime().dbl() << std::endl;
}