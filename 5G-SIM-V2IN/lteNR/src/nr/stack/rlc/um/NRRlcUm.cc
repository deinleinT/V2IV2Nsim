//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU),
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

#include "nr/stack/rlc/um/NRRlcUm.h"

#include "nr/stack/mac/layer/NRMacGnb.h"
#include "nr/stack/mac/layer/NRMacUe.h"
#include "nr/corenetwork/cellInfo/NRCellInfo.h"
#include "nr/apps/TrafficGenerator/packet/V2XMessage_m.h"

Define_Module(NRRlcUm);

void NRRlcUm::initialize() {

	std::string nodeType = getParentModule()->getParentModule()->par("nodeType").stdstringValue();
	std::string macType = getParentModule()->getParentModule()->par("LteMacType").stdstringValue();

	if (nodeType.compare("ENODEB") == 0 || nodeType.compare("GNODEB") == 0) {
		if (macType.compare("NRMacGnb") != 0)
			throw cRuntimeError("NRRlcUm::initialize - %s module found, must be NRMacGnb. Aborting", macType.c_str());

	} else if (nodeType.compare("UE") == 0) {
		if (macType.compare("NRMacUe") != 0)
			throw cRuntimeError("NRRlcUm::initialize - %s module found, must be NRMacUe. Aborting", macType.c_str());

	}

	up_[IN] = gate("UM_Sap_up$i");
	up_[OUT] = gate("UM_Sap_up$o");
	down_[IN] = gate("UM_Sap_down$i");
	down_[OUT] = gate("UM_Sap_down$o");

	qosHandler = check_and_cast<QosHandler*>(getParentModule()->getParentModule()->getSubmodule("qosHandler"));

	WATCH_MAP(txEntities_);
	WATCH_MAP(rxEntities_);

	receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
	receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
	sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
	sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

	totalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
	totalRlcThroughputDl.setName("UEtotalRlcThroughputDl");
	totalRcvdBytesUl = 0;
	totalRcvdBytesDl = 0;

	numberOfConnectedUes = 0;
	cellConnectedUes.setName("cellConnectedUes");

	UEtotalRlcThroughputDlMean = registerSignal("UEtotalRlcThroughputDlMean");
	UEtotalRlcThroughputUlMean = registerSignal("UEtotalRlcThroughputUlMean");
}

void NRRlcUm::handleUpperMessage(cPacket *pkt) {
	//std::cout << "NRRlcUm::handleUpperMessage start at " << simTime().dbl() << std::endl;

	//EV << "NRRlcUm::handleUpperMessage - Received packet " << pkt->getName() << " from upper layer, size " << pkt->getByteLength() << "\n";

	FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());

	UmTxEntity *txbuf = getTxBuffer(lteInfo);

	// Create a new RLC packet
	LteRlcSdu *rlcPkt = new LteRlcSdu("rlcUmPkt");
	rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
	rlcPkt->setLengthMainPacket(pkt->getByteLength());
	rlcPkt->setKind(pkt->getKind());
	rlcPkt->encapsulate(pkt);

	// create a message so as to notify the MAC layer that the queue contains new data
	LteRlcPdu *newDataPkt = new LteRlcPdu("newDataPkt");
	newDataPkt->setKind(rlcPkt->getKind());
	newDataPkt->setSnoMainPacket(lteInfo->getSequenceNumber());

	// make a copy of the RLC SDU
	LteRlcSdu *rlcPktDup = rlcPkt->dup();
	// the MAC will only be interested in the size of this packet
	newDataPkt->encapsulate(rlcPktDup);
	newDataPkt->setControlInfo(lteInfo->dup());

	//EV << "NRRlcUm::handleUpperMessage - Sending message " << newDataPkt->getName() << " to port UM_Sap_down$o\n";
	send(newDataPkt, down_[OUT]);

	rlcPkt->setControlInfo(lteInfo);

	drop(rlcPkt);

	// Bufferize RLC SDU
	//EV << "NRRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getName() << " into the Tx Buffer\n";
	txbuf->enque(rlcPkt);

	//std::cout << "NRRlcUm::handleUpperMessage end at " << simTime().dbl() << std::endl;
}

void NRRlcUm::handleMessage(cMessage *msg) {

	//std::cout << "NRRlcUm::handleMessage start at " << simTime().dbl() << std::endl;

	if (strcmp(msg->getName(), "RRC") == 0) {
		cGate *incoming = msg->getArrivalGate();
		if (incoming == up_[IN]) {
			//from pdcp to mac
			send(msg, gate("lowerLayer$o"));
		} else if (incoming == down_[IN]) {
			//from mac to pdcp
			send(msg, gate("upperLayer$o"));
		}
	}

	LteRlcUm::handleMessage(msg);

	//std::cout << "NRRlcUm::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRRlcUm::handleLowerMessage(cPacket *pkt) {
	//std::cout << "NRRlcUm::handleLowerMessage start at " << simTime().dbl() << std::endl;

	//EV << "NRRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";

	FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

	if (strcmp(pkt->getName(), "LteMacSduRequest") == 0) {

		LteMacSduRequest *macSduRequest = check_and_cast<LteMacSduRequest*>(pkt);
		if (macSduRequest->containsSeveralCids()) {

			LteRlcUm *rlc;
			FlowControlInfo *info = NULL;
			LteRlcUmDataPdu *rlcPdu = new LteRlcUmDataPdu("lteRlcFragment");

			if (macSduRequest->getRequest().size() > 1) {
				for (auto &var : macSduRequest->getRequest()) { // var is LteMacSduRequest Packet
					unsigned int size = var->getSduSize(); // bytes allowed to send
					if (size <= RLC_HEADER_UM) // size at least bigger than headersize
						continue;

					UmTxEntity *txbuf = getTxBuffer(check_and_cast<FlowControlInfo*>(var->getControlInfo()));
					take(var);

					std::pair<LteRlcUm*, LteRlcUmDataPdu*> result = txbuf->rlcPduMake(size, true); // build a pdu with size

					if (result.second->getNumSdu() == 0) {
						delete result.second;
						continue;
					}
					rlcPdu->insertRequest(result.second);
					rlc = result.first;
					info = check_and_cast<FlowControlInfo*>(result.second->getControlInfo()->dup());
				}

				if (rlcPdu->getRequest().size() == 0) {
					//Request failed!
					delete macSduRequest;
					delete rlcPdu;
					return;
				}
				if (rlcPdu->getRequest().size() > 1)
					rlcPdu->setContainsFlag(true);
				qosHandler->modifyControlInfo(check_and_cast<LteControlInfo*>(info));
				rlcPdu->setControlInfo(info);
				rlcPdu->setByteLength(pkt->getByteLength());
				rlc->sendToLowerLayer(rlcPdu);
			} else {
				LteMacSduRequest *macSduReq = macSduRequest->getRequest().back();
				macSduRequest->getRequest().pop_back();
				ASSERT(macSduRequest->getRequest().size() == 0);
				unsigned int size = macSduReq->getSduSize();

				if (size <= RLC_HEADER_UM) { // size at least bigger than headersize
					delete macSduReq;
					delete macSduRequest;
					return;
				}

				UmTxEntity *txbuf = getTxBuffer(lteInfo);

				drop(pkt);

				// do segmentation/concatenation and send a pdu to the lower layer
				txbuf->rlcPduMake(size);
				delete macSduReq;

			}

		} else {
			// get the corresponding Tx buffer

			unsigned int size = macSduRequest->getSduSize();

			if (size <= RLC_HEADER_UM) { // size at least bigger than headersize
				delete macSduRequest;
				return;
			}

			UmTxEntity *txbuf = getTxBuffer(lteInfo);

			drop(pkt);

			// do segmentation/concatenation and send a pdu to the lower layer
			txbuf->rlcPduMake(size);
		}
		delete macSduRequest;

	} else { // data pdu

		// Extract informations from fragment
		UmRxEntity *rxbuf = getRxBuffer(lteInfo);
		drop(pkt);

		// Bufferize PDU
		//EV << "NRRlcUm::handleLowerMessage - Enque packet " << pkt->getName() << " into the Rx Buffer\n";
		rxbuf->enque(pkt);
	}

	//std::cout << "NRRlcUm::handleLowerMessage end at " << simTime().dbl() << std::endl;
}

void NRRlcUm::sendDefragmented(cPacket *pkt) {
	//std::cout << "NRRlcUm::sendDefragmented start at " << simTime().dbl() << std::endl;

	Enter_Method_Silent
	("sendToLowerLayer");
	take(pkt);

	//for V2X Broadcast
	std::string nodeType = getParentModule()->getParentModule()->par("nodeType").stdstringValue();
	FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

	if (((ApplicationType) lteInfo->getApplication() == V2X_STATUS || (ApplicationType) lteInfo->getApplication() == V2X_REQUEST || (ApplicationType) lteInfo->getApplication() == V2X)
			&& getSystemModule()->par("v2vMulticastFlag").boolValue() && ((nodeType.compare("ENODEB") == 0 || nodeType.compare("GNODEB") == 0))) {
		//find out the connected cars
		//the new FlowControlinfo
		MacNodeId ueId = lteInfo->getSourceId();

		//distance
		double range = getSystemModule()->par("v2vMulticastDistance").doubleValue();
		NRMacGnb *mac = check_and_cast<NRMacGnb*>(getParentModule()->getParentModule()->getSubmodule("mac"));
		MacNodeId macId = mac->getMacNodeId();

		NRMacUe *ueMac = check_and_cast<NRMacUe*>(getMacByMacNodeId(ueId));
		LtePhyBase *uePhy = check_and_cast<LtePhyBase*>(ueMac->getParentModule()->getSubmodule("phy"));
		inet::Coord ueCoord = uePhy->getCoord();

		//getNRCellInfo(nodeId)
		NRCellInfo *cellInfo = check_and_cast<NRCellInfo*>(mac->getCellInfo());
		std::vector<MacNodeId> ues = cellInfo->getConnectedUes();

		for (auto &var : ues) {

			if (macId == var)
				continue;

			if (ueId == var)
				continue;

			IPv4Address ipAddress = getBinder()->getIPAddressByMacNodeId(var);
			if (ipAddress.isUnspecified()) {
				continue;
			}

			if (getSimulation()->getModule(getBinder()->getOmnetId(var)) == nullptr) {
				continue;
			}

			LtePhyBase *phy = check_and_cast<LtePhyBase*>(getMacByMacNodeId(var)->getParentModule()->getSubmodule("phy"));
			inet::Coord coord = phy->getCoord();
			double distance = ueCoord.distance(coord);

			if (distance <= range) {
				//send message to this ue
				cPacket *nextDup = pkt->dup();
				FlowControlInfo *dupLteInfo = lteInfo->dup();
				dupLteInfo->setDestId(var);
				dupLteInfo->setDstAddr(ipAddress.getInt());

				nextDup->setControlInfo(dupLteInfo);

				send(nextDup, up_[OUT]);
			}
		}
	}
	//

	//EV << "NRRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
	send(pkt, up_[OUT]);

	//std::cout << "NRRlcUm::sendDefragmented end at " << simTime().dbl() << std::endl;
}
