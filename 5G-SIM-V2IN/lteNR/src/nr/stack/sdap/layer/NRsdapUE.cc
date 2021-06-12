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

		qosHandler = check_and_cast<QosHandler*>(getParentModule()->getSubmodule("qosHandler"));

		nodeType = qosHandler->getNodeType();
		nodeId_ = getAncestorPar("macNodeId");
		hoErrorCount = 0;
		WATCH(nodeId_);

	}

}

void NRsdapUE::handleMessage(cMessage * msg) {

	//std::cout << "NRsdap::handleMessage start at " << simTime().dbl() << std::endl;

	if (strcmp(msg->getArrivalGate()->getBaseName(), "upperLayer") == 0) {

		fromUpperToLower(msg);

	}
	else if (strcmp(msg->getArrivalGate()->getBaseName(), "lowerLayer") == 0) {

		fromLowerToUpper(msg);

	}

	//std::cout << "NRsdap::handleMessage end at " << simTime().dbl() << std::endl;
}

//incoming messages from IP2NR, to pdcp
void NRsdapUE::fromUpperToLower(cMessage * msg) {

	//std::cout << "NRsdap::fromUpperToLower start at " << simTime().dbl() << std::endl;

	auto pktIn = check_and_cast<cPacket*>(msg);
	auto pkt = check_and_cast<inet::Packet*>(pktIn);
	auto lteInfo = pkt->getTag<FlowControlInfo>();

	setTrafficInformation(pkt->getName(), pkt);
	MacNodeId destId;
	MacNodeId master = getNRBinder()->getNextHop(nodeId_);
	if (master != nodeId_) {
		destId = master;
	}

	if ((destId >= ENB_MIN_ID && destId <= ENB_MAX_ID) && (nodeId_ >= ENB_MIN_ID && nodeId_ <= ENB_MAX_ID)) {
		hoErrorCount++;
		delete msg;
		return;
	}

	//nodeid is ue, destid is nodeB
	NRSdapEntity *entity = getEntity(nodeId_, destId, (ApplicationType)(lteInfo->getApplication()));

	pkt->addTagIfAbsent<FlowControlInfo>()->setDestId(destId);
	pkt->addTagIfAbsent<FlowControlInfo>()->setSourceId(nodeId_);
	pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(1);

	if (getSystemModule()->hasPar("considerProcessingDelay")) {
		if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
			//add processing delay
			sendDelayed(pkt, uniform(0, pkt->getByteLength() / 10e6), lowerLayer);
		}
		else {
			send(pkt, lowerLayer);
		}
	}

	//std::cout << "NRsdap::fromUpperToLower end at " << simTime().dbl() << std::endl;
}

//incoming messages from pdcp, to IP2NR
void NRsdapUE::fromLowerToUpper(cMessage * msg) {

	//std::cout << "NRsdap::fromLowerToUpper start at " << simTime().dbl() << std::endl;

	inet::Packet *pkt = check_and_cast<inet::Packet*>(msg);

	if (getSystemModule()->hasPar("considerProcessingDelay")) {
		if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
			//add processing delay
			sendDelayed(pkt, uniform(0, pkt->getByteLength() / 10e6), upperLayer);
		}
		else {
			send(pkt, upperLayer);
		}
	}

	//std::cout << "NRsdap::fromLowerToUpper end at " << simTime().dbl() << std::endl;
}

