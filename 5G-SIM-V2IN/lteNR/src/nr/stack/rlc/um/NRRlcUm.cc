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

#include "nr/stack/rlc/um/NRRlcUm.h"

#include "nr/stack/mac/layer/NRMacUE.h"
#include "nr/stack/mac/layer/NRMacGNB.h"
#include "nr/common/cellInfo/NRCellInfo.h"
#include "nr/apps/TrafficGenerator/packet/V2XMessage_m.h"

Define_Module(NRRlcUm);

void NRRlcUm::initialize(int stage) {

    //LteRlcUm::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        nodeType = getParentModule()->getParentModule()->par("nodeType").stdstringValue();
        up_[IN_GATE] = gate("UM_Sap_up$i");
        up_[OUT_GATE] = gate("UM_Sap_up$o");
        down_[IN_GATE] = gate("UM_Sap_down$i");
        down_[OUT_GATE] = gate("UM_Sap_down$o");

        // parameters
        mapAllLcidsToSingleBearer_ = par("mapAllLcidsToSingleBearer");

        // statistics
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");
        rlcPacketLossDl = registerSignal("rlcPacketLossDl");
        rlcPacketLossUl = registerSignal("rlcPacketLossUl");

        totalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
        totalRlcThroughputDl.setName("UEtotalRlcThroughputDl");

        //for measuring the throughput for remote vehicles and human driven vehicles
        if (nodeType.compare("UE") != 0) {
            ueTotalRlcThroughputDlInit = true;
            ueTotalRlcThroughputUlInit = true;
            ueTotalRlcThroughputUlStartTime = NOW;
            ueTotalRlcThroughputDlStartTime = NOW;
            ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
            ueTotalRlcThroughputDl.setName("UEtotalRlcThroughputDl");

        }
        else {
            ueTotalRlcThroughputDlInit = false;
            ueTotalRlcThroughputUlInit = false;
            throughputTimer = new cMessage("throughputTimer");
            throughputInterval = getSimulation()->getSystemModule()->par("throughputInterval").doubleValue();
            double time = NOW.dbl();
            double firstSchedule = floor(time + throughputInterval);
            scheduleAt(firstSchedule, throughputTimer);
        }

        totalRcvdBytesUl = 0;
        totalRcvdBytesDl = 0;

        numberOfConnectedUes = 0;
        cellConnectedUes.setName("cellConnectedUes");

        UEtotalRlcThroughputDlMean = registerSignal("UEtotalRlcThroughputDlMean");
        UEtotalRlcThroughputUlMean = registerSignal("UEtotalRlcThroughputUlMean");

        throughputInBitsPerSecondDL = 0;
        throughputInBitsPerSecondUL = 0;

        qosHandler = check_and_cast<QosHandler*>(getParentModule()->getParentModule()->getSubmodule("qosHandler"));
    }
}

void NRRlcUm::handleUpperMessage(cPacket * pktAux) {
	//std::cout << "NRRlcUm::handleUpperMessage start at " << simTime().dbl() << std::endl;

	//LteRlcUm::handleUpperMessage(pkt);

    emit(receivedPacketFromUpperLayer, pktAux);

    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();

    auto chunk = pkt->peekAtFront<inet::Chunk>();
    EV << "LteRlcUm::handleUpperMessage - Received packet " << chunk->getClassName() << " from upper layer, size " << pktAux->getByteLength() << "\n";

    UmTxEntity* txbuf = getTxBuffer(lteInfo);

    // Create a new RLC packet
    auto rlcPkt = inet::makeShared<LteRlcSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    pkt->insertAtFront(rlcPkt);

    drop(pkt);

//    if (txbuf->isHoldingDownstreamInPackets())
//    {
//        // do not store in the TX buffer and do not signal the MAC layer
//        EV << "LteRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getClassName() << " into the Holding Buffer\n";
//        txbuf->enqueHoldingPackets(pkt);
//    }
//    else
//    {
        if(txbuf->enque(pkt)){
            EV << "LteRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getClassName() << " into the Tx Buffer\n";

            // create a message so as to notify the MAC layer that the queue contains new data
            auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
            // make a copy of the RLC SDU
            auto pktDup = pkt->dup();
            pktDup->insertAtFront(newDataPkt);
            // the MAC will only be interested in the size of this packet

            EV << "LteRlcUm::handleUpperMessage - Sending message " << newDataPkt->getClassName() << " to port UM_Sap_down$o\n";
            send(pktDup, down_[OUT_GATE]);
        } else {
            // Queue is full - drop SDU
            dropBufferOverflow(pkt);
        }
//    }

	//std::cout << "NRRlcUm::handleUpperMessage end at " << simTime().dbl() << std::endl;
}

void NRRlcUm::handleMessage(cMessage * msg) {

	//std::cout << "NRRlcUm::handleMessage start at " << simTime().dbl() << std::endl;
	if (nodeType.compare("UE") == 0) {
		if (msg->isSelfMessage()) {
			//throughput per second for each vehicle
			checkRemoteCarStatus();
			UERlcThroughputPerSecondDl.record(throughputInBitsPerSecondDL);
			UERlcThroughputPerSecondUl.record(throughputInBitsPerSecondUL);
			throughputInBitsPerSecondDL = 0;
			throughputInBitsPerSecondUL = 0;
			scheduleAt(NOW + throughputInterval, throughputTimer);
			return;
		}
	}

	LteRlcUm::handleMessage(msg);

	//std::cout << "NRRlcUm::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRRlcUm::handleLowerMessage(cPacket * pkt) {
	//std::cout << "NRRlcUm::handleLowerMessage start at " << simTime().dbl() << std::endl;

	LteRlcUm::handleLowerMessage(pkt);

	//std::cout << "NRRlcUm::handleLowerMessage end at " << simTime().dbl() << std::endl;
}

void NRRlcUm::sendDefragmented(cPacket * pkt) {
	//std::cout << "NRRlcUm::sendDefragmented start at " << simTime().dbl() << std::endl;

	Enter_Method_Silent("sendToLowerLayer");
	take(pkt);

	//for V2X Broadcast
	if (getSystemModule()->hasPar("v2vMulticastFlag")) {
		std::string nodeType = getParentModule()->getParentModule()->par("nodeType").stdstringValue();
		FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

		if ((ApplicationType) lteInfo->getApplication() == V2X && getSystemModule()->par("v2vMulticastFlag").boolValue()
				&& ((nodeType.compare("ENODEB") == 0 || nodeType.compare("GNODEB") == 0))) {
			//find out the connected cars
			//the new FlowControlinfo
			MacNodeId ueId = lteInfo->getSourceId();

			//distance
			double range = getSystemModule()->par("v2vMulticastDistance").doubleValue();
			NRMacGNB *mac = check_and_cast<NRMacGNB*>(getParentModule()->getParentModule()->getSubmodule("mac"));
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

				inet::Ipv4Address ipAddress = getNRBinder()->getIpAddressByMacNodeId(var);
				if (ipAddress.isUnspecified()) {
					continue;
				}

				if (getSimulation()->getModule(getNRBinder()->getOmnetId(var)) == nullptr) {
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

					send(nextDup, up_[OUT_GATE]);
				}
			}
		}
	}
	//

	send(pkt, up_[OUT_GATE]);

	//std::cout << "NRRlcUm::sendDefragmented end at " << simTime().dbl() << std::endl;
}

