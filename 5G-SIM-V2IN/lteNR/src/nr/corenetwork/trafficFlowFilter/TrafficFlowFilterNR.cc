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

#include "nr/corenetwork/trafficFlowFilter/TrafficFlowFilterNR.h"
#include "corenetwork/trafficFlowFilter/TrafficFlowFilter.h"
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

Define_Module(TrafficFlowFilterNR);

using namespace omnetpp;
using namespace inet;

void TrafficFlowFilterNR::initialize(int stage) {

	//code TrafficFlowFilter::initialize(stage) is located here

	// wait until all the IP addresses are configured
	if (stage != inet::INITSTAGE_NETWORK_LAYER)
		return;

	// get reference to the binder
	binder_ = getNRBinder();

	fastForwarding_ = par("fastForwarding");

	// reading and setting owner type
	ownerType_ = selectOwnerType(par("ownerType"));

	//mec
	if (getParentModule()->hasPar("meHost")) {
		meHost = getParentModule()->par("meHost").stdstringValue();
		if (isBaseStation(ownerType_) && strcmp(meHost.c_str(), "")) {
			std::stringstream meHostName;
			meHostName << meHost.c_str() << ".virtualisationInfrastructure";
			meHost = meHostName.str();
			meHostAddress = inet::L3AddressResolver().resolve(meHost.c_str());
		}
	}
	//end mec

	// register service processing IP-packets on the LTE Uu Link
	registerService(LteProtocol::ipv4uu, gate("internetFilterGateIn"), gate("internetFilterGateIn"));

	if (stage != inet::INITSTAGE_LAST) {

		if (ownerType_ == UPF) {
			qosHandler = check_and_cast<QosHandler*>(getParentModule()->getSubmodule("qosHandler"));
		}
		else if (ownerType_ == GNB || ownerType_ == ENB) {
			qosHandler = check_and_cast<QosHandler*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("qosHandler"));
		}

		if (ownerType_ == GNB || ownerType_ == ENB) {
			std::string moduleName = getParentModule()->gate("ppp$o")->getNextGate()->getOwnerModule()->getName();

			if (moduleName.find("upf") != std::string::npos) {
				LteMacBase *mac = check_and_cast<LteMacBase*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("mac"));
				MacNodeId gnbId = mac->getMacNodeId();
				check_and_cast<NRBinder*>(binder_)->fillUpfGnbMap(gnbId, moduleName);
			}
			else {
				//not connected to a UPF
			}
		}
	}
	//
}

void TrafficFlowFilterNR::handleMessage(cMessage * msg) {
	//std::cout << "TrafficFlowFilterNR::handleMessage start at " << simTime().dbl() << std::endl;

	Packet *pkt = check_and_cast<Packet*>(msg);

	// receive and read IP datagram
	const auto &ipv4Header = pkt->peekAtFront<Ipv4Header>();
	const Ipv4Address &destAddr = ipv4Header->getDestAddress();
	const Ipv4Address &srcAddr = ipv4Header->getSrcAddress();
	pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
	pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
	std::string name = std::string(msg->getName());

	// run packet filter and associate a flowId to the connection (default bearer?)
	// search within tftTable the proper entry for this destination
	TrafficFlowTemplateId tftId = findTrafficFlow(srcAddr, destAddr); // search for the tftId in the binder
	if (tftId == -2) {
		// the destination has been removed from the simulation. Delete msg
		//EV << "TrafficFlowFilterNR::handleMessage - Destination has been removed from the simulation. Delete packet." << endl;
		delete msg;
	}
	else if (tftId == -4) {
		//need to send the packet to another upf
		//send to connectedUPF_ if connected, else send to random connected upf
		int index = gateSize("fromToN9Interface");
		std::string upfConnected;
		int gateIndex = 0;
		for (int i = 0; i < index; i++) {
			std::string destinationName = gate("fromToN9Interface$o", i)->getNextGate()->getNextGate()->getOwnerModule()->getName();
			gateIndex = i;
			if (connectedUPF_ == destinationName) {
				if (getSystemModule()->hasPar("considerProcessingDelay")) {
					if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
						//add processing delay
						sendDelayed(pkt, uniform(0, pkt->getBitLength() / 8 / 10e6), "fromToN9Interface$o", i);
					}
					else {
						send(pkt, "fromToN9Interface$o", i);
					}
				}
				connectedUPF_ = "";
				return;
			}
		}
		if (getSystemModule()->hasPar("considerProcessingDelay")) {
			if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
				//add processing delay
				sendDelayed(pkt, uniform(0, pkt->getBitLength() / 8 / 10e6), "fromToN9Interface$o", gateIndex);
			}
			else {
				send(pkt, "fromToN9Interface$o", gateIndex);
			}
		}
		connectedUPF_ = "";
	}
	else {
		// add control info to the normal ip datagram. This info will be read by the GTP-U application
		auto tftInfo = pkt->addTag<TftControlInfo>();

		std::string applName = std::string(name);
		if (strcmp(name.c_str(), "V2X") == 0 || (applName.find("V2X") != string::npos) || (applName.find("v2x") != string::npos)) {
			tftInfo->setMsgCategory(V2X);
			tftInfo->setQfi(qosHandler->getQfi(V2X));
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		}
		else if (strcmp(name.c_str(), "VoIP") == 0 || (applName.find("VoIP") != string::npos) || (applName.find("voip") != string::npos)
				|| (applName.find("Voip") != string::npos)) {
			tftInfo->setQfi(qosHandler->getQfi(VOIP));
			tftInfo->setMsgCategory(VOIP);
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		}
		else if (strcmp(name.c_str(), "Video") == 0 || (applName.find("Video") != string::npos) || (applName.find("video") != string::npos)) {
			tftInfo->setQfi(qosHandler->getQfi(VOD));
			tftInfo->setMsgCategory(VOD);
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		}
		else /*if (strcmp(pkt->getName(), "Data") == 0 || strcmp(pkt->getName(), "Data-frag") == 0) */{
			tftInfo->setQfi(qosHandler->getQfi(DATA_FLOW));
			tftInfo->setMsgCategory(DATA_FLOW);
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		}

		tftInfo->setTft(tftId);

		//EV << "TrafficFlowFilterNR::handleMessage - setting tft=" << tftId << endl;

		if (getSystemModule()->hasPar("considerProcessingDelay")) {
			if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
				//add processing delay
				sendDelayed(pkt, uniform(0, pkt->getBitLength() / 8 / 10e6), "gtpUserGateOut");
			}
			else {
				// send the datagram to the GTP-U module
				send(pkt, "gtpUserGateOut");
			}
		}
	}

	//std::cout << "TrafficFlowFilterNR::handleMessage end at " << simTime().dbl() << std::endl;
}

TrafficFlowTemplateId TrafficFlowFilterNR::findTrafficFlow(L3Address srcAddress, L3Address destAddress) {

	//std::cout << "TrafficFlowFilterNR::findTrafficFlow start at " << simTime().dbl() << std::endl;

	//mec
	// check this before the other!
	if (isBaseStation(ownerType_) && destAddress.operator ==(meHostAddress)) {
		// the destination is the ME Host
		EV << "TrafficFlowFilter::findTrafficFlow - returning flowId (-3) for tunneling to " << meHost << endl;
		return -3;
	}
	else if (ownerType_ == GTPENDPOINT) {
		// send only messages direct to UEs --> UEs have macNodeId != 0
		MacNodeId destId = binder_->getMacNodeId(destAddress.toIpv4());
		MacNodeId destMaster = binder_->getNextHop(destId);
		EV << "TrafficFlowFilter::findTrafficFlow - returning flowId for " << binder_->getModuleNameByMacNodeId(destMaster) << ": " << destMaster
					<< endl;
		return destMaster;
	}
	//end mec
	//

	MacNodeId destId = binder_->getMacNodeId(destAddress.toIpv4());
	if (destId == 0) {
		EV << "TrafficFlowFilter::findTrafficFlow - destId = " << destId << endl;

		if (isBaseStation(ownerType_))
			return -1; // the destination is outside the LTE network, so send the packet to the PGW
		else
			// PGW or UPF
			return -2; // the destination UE has been removed from the simulation
	}

	MacNodeId destBS = binder_->getNextHop(destId);
	if (destBS == 0)
		return -2;   // the destination UE is not attached to any nodeB

	// the serving node for the UE might be a secondary node in case of NR Dual Connectivity
	// obtains the master node, if any (the function returns destEnb if it is a master already)
	MacNodeId destMaster = binder_->getMasterNode(destBS);

	if (isBaseStation(ownerType_)) {
		MacNodeId srcMaster = binder_->getNextHop(binder_->getMacNodeId(srcAddress.toIpv4()));
		if (fastForwarding_ && srcMaster == destMaster)
			return 0;                 // local delivery
		return -1;   // send the packet to the PGW/UPF
	}

	//if several upfs are in the scenario
	if (ownerType_ == UPF) {
		//get local upf name
		std::string localUPFname = getParentModule()->getName();
		MacNodeId connectedGnbWithDestId = check_and_cast<NRBinder*>(binder_)->getConnectedGnb(destId);
		std::string connectedUpfToMasterId = check_and_cast<NRBinder*>(binder_)->getConnectedUpf(destMaster);
		if (connectedUpfToMasterId == localUPFname) {
			return destMaster;
		}
		else {
			connectedUPF_ = connectedUpfToMasterId;
			return -4;
		}
	}
	//
	//std::cout << "TrafficFlowFilterNR::findTrafficFlow end at " << simTime().dbl() << std::endl;

	return destMaster;
}

