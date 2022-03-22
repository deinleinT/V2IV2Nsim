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

#include "nr/epc/TrafficFlowFilterNR.h"

Define_Module(TrafficFlowFilterNR);

void TrafficFlowFilterNR::initialize(int stage) {
	// wait until all the IP addresses are configured
	if (stage != inet::INITSTAGE_NETWORK_LAYER)
		return;

	// get reference to the binder
	if (stage != inet::INITSTAGE_NETWORK_LAYER_3) {
		binder_ = getNRBinder();

		fastForwarding_ = par("fastForwarding");

		// reading and setting owner type
		ownerType_ = selectOwnerType(par("ownerType"));

		//binder_->testPrintQosValues();

		if (ownerType_ == USER_PLANE_FUNCTION) {
			qosHandler = check_and_cast<QosHandler*>(getParentModule()->getSubmodule("qosHandler"));
		} else if (ownerType_ == GNB || ownerType_ == ENB) {
			qosHandler = check_and_cast<QosHandler*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("qosHandler"));
		}

		//TODO --> in Binder, to which gnb is the upf connected?
		if (ownerType_ == GNB || ownerType_ == ENB) {
		    cModule* upfModule = getParentModule()->gate("ppp$o")->getNextGate()->getOwnerModule();
            if (!(upfModule->hasPar("nodeType") && strcmp(upfModule->par("nodeType"), "UPF") == 0)) {
                // if: connected gate is part of container module, UPF is a sub-component -> search UPF component
                upfModule = upfModule->getModuleByPath(".upf");
                if (!upfModule) {
                    error("TrafficFlowFilterNR::initialize - could not find UPF module in compound module. Aborting...");
                }
            }

			std::string moduleName = upfModule->getFullPath();

			if (moduleName.find("upf") != std::string::npos) {
				LteMacBase *mac = check_and_cast<LteMacBase*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("mac"));
				MacNodeId gnbId = mac->getMacNodeId();
				binder_->fillUpfGnbMap(gnbId, moduleName);
			} else {
				//not connected to a UPF
			    EV << "TrafficFlowFilterNR::initialize - Not connected to a UPF." << endl;
			}
		}
	}
	//
}

EpcNodeType TrafficFlowFilterNR::selectOwnerType(const char *type) {
	//std::cout << "TrafficFlowFilterNR::selectOwnerType start at "<< simTime().dbl() << std::endl;

	//EV << "TrafficFlowFilterNR::selectOwnerType - setting owner type to " << type << endl;
	if (strcmp(type, "GNODEB") == 0)
		return GNB;
	else if (strcmp(type, "UPF") == 0 || strcmp(type, "PGW") == 0)
		return USER_PLANE_FUNCTION;
	else if (strcmp(type, "ENODEB") == 0)
		return ENB;
	else
		error("TrafficFlowFilterNR::selectOwnerType - unknown owner type [%s]. Aborting...", type);
}

void TrafficFlowFilterNR::handleMessage(cMessage *msg) {
	//std::cout << "TrafficFlowFilterNR::handleMessage start at " << simTime().dbl() << std::endl;

	//EV << "TrafficFlowFilterSimplified::handleMessage - Received Packet:" << endl;
	//EV << "name: " << msg->getFullName() << endl;

	// receive and read IP datagram
	IPv4Datagram *datagram = check_and_cast<IPv4Datagram*>(msg);
	IPv4Address &destAddr = datagram->getDestAddress();
	IPv4Address &srcAddr = datagram->getSrcAddress();
	std::string name = std::string(msg->getName());

	//EV << "TrafficFlowFilterNR::handleMessage - Received datagram : " << datagram->getName() << " - src[" << srcAddr << "] - dest[" << destAddr << "]\n";

	// run packet filter and associate a flowId to the connection (default bearer?)
	// search within tftTable the proper entry for this destination
	TrafficFlowTemplateId tftId = findTrafficFlow(srcAddr, destAddr); // search for the tftId in the binder
	if (tftId == -2) {
		// the destination has been removed from the simulation. Delete msg
		//EV << "TrafficFlowFilterNR::handleMessage - Destination has been removed from the simulation. Delete packet." << endl;
		delete msg;
	} else if (tftId == -3) {
		//need to send the packet to another upf
		//send to connectedUPF_ if connected, else send to random connected upf
		int index = gateSize("fromToN9Interface");
		std::string upfConnected;
		int gateIndex = 0;
		for (int i = 0; i < index; i++) {
		    cModule* upfModule = gate("fromToN9Interface$o", i)->getNextGate()->getNextGate()->getOwnerModule();
            if (!(upfModule->hasPar("nodeType") && strcmp(upfModule->par("nodeType"), "UPF") == 0)) {
                // if: connected gate is part of container module, UPF is a sub-component -> search UPF component
                upfModule = upfModule->getModuleByPath(".upf");
                if (!upfModule) {
                    error("TrafficFlowFilterNR::handleMessage - could not find UPF module in compound module. Aborting...");
                }
            }
			std::string destinationName = upfModule->getFullPath();
			gateIndex = i;
			if (connectedUPF_ == destinationName) {
				break;
			}
		}

		// send datagram to other UPF
		// - to final UPF, if directly connected
		// - to other UPF, if not directly connected
		cGate *outGate = gate("fromToN9Interface$o", gateIndex);
		simtime_t delay = SIMTIME_ZERO;
		if (outGate->getChannel()->isBusy()) {
		    delay = delay + outGate->getChannel()->getTransmissionFinishTime();
		}
		if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
			//add processing delay
		    delay = delay + uniform(0,datagram->getTotalLengthField()/10e5);
		}
        sendDelayed(datagram, delay, outGate);

		connectedUPF_ = "";
	} else {
		// add control info to the normal ip datagram. This info will be read by the GTP-U application
		TftControlInfo *tftInfo = new TftControlInfo();
		tftInfo->setTft(tftId);

		std::string applName = std::string(name);
		if (strcmp(name.c_str(), "V2X") == 0
				|| (applName.find("V2X") != string::npos)
				|| (applName.find("v2x") != string::npos)) {
			tftInfo->setMsgCategory(V2X);
			tftInfo->setQfi(qosHandler->getQfi(V2X));
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		} else if (strcmp(name.c_str(), "VoIP") == 0
				|| (applName.find("VoIP") != string::npos)
				|| (applName.find("voip") != string::npos)
				|| (applName.find("Voip") != string::npos)) {
			tftInfo->setQfi(qosHandler->getQfi(VOIP));
			tftInfo->setMsgCategory(VOIP);
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		} else if (strcmp(name.c_str(), "Video") == 0
				|| (applName.find("Video") != string::npos)
				|| (applName.find("video") != string::npos)) {
			tftInfo->setQfi(qosHandler->getQfi(VOD));
			tftInfo->setMsgCategory(VOD);
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		} else /*if (strcmp(pkt->getName(), "Data") == 0 || strcmp(pkt->getName(), "Data-frag") == 0) */{
			tftInfo->setQfi(qosHandler->getQfi(DATA_FLOW));
			tftInfo->setMsgCategory(DATA_FLOW);
			tftInfo->setRadioBearerId(qosHandler->getRadioBearerId(tftInfo->getQfi()));
		}

		datagram->removeControlInfo();
		datagram->setControlInfo(tftInfo);

		//EV << "TrafficFlowFilterNR::handleMessage - setting tft=" << tftId << endl;

		if (getSystemModule()->par("considerProcessingDelay").boolValue()) {
			//add processing delay
			sendDelayed(datagram, uniform(0,datagram->getTotalLengthField()/10e5),"gtpUserGateOut");
		} else {
			// send the datagram to the GTP-U module
			send(datagram, "gtpUserGateOut");
		}
	}

	//std::cout << "TrafficFlowFilterNR::handleMessage end at " << simTime().dbl() << std::endl;
}

TrafficFlowTemplateId TrafficFlowFilterNR::findTrafficFlow(L3Address srcAddress, L3Address destAddress) {

	//std::cout << "TrafficFlowFilterNR::findTrafficFlow start at " << simTime().dbl() << std::endl;

	MacNodeId destId = binder_->getMacNodeId(destAddress.toIPv4());
	if (destId == 0) {
		if (ownerType_ == GNB || ownerType_ == ENB)
			return -1; // the destination is outside the LTE network, so send the packet to the PGW
		else
			// PGW
			return -2; // the destination UE has been removed from the simulation
	}

	MacNodeId destMaster = binder_->getNextHop(destId);

	if (ownerType_ == GNB || ownerType_ == ENB) {
		MacNodeId srcMaster = binder_->getNextHop(binder_->getMacNodeId(srcAddress.toIPv4()));
		if (fastForwarding_ && srcMaster == destMaster)
			return 0;                 // local delivery
		return -1;   // send the packet to the PGW
	}

	//if several upfs are in the scenario
	if (ownerType_ == USER_PLANE_FUNCTION) {
		//get local upf name
		std::string localUPFname = getParentModule()->getFullPath();
		//MacNodeId connectedGnbWithDestId = binder_->getConnectedGnb(destId);
		std::string connectedUpfToMasterId = binder_->getConnectedUpf(destMaster);
		if (connectedUpfToMasterId == localUPFname) {
			return destMaster;
		} else {
			connectedUPF_ = connectedUpfToMasterId;
			return -3;
		}
	}
	//
	//std::cout << "TrafficFlowFilterNR::findTrafficFlow end at " << simTime().dbl() << std::endl;

	return destMaster;
}

