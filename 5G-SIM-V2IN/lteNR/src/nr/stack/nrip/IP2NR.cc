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

#include "nr/stack/nrip/IP2NR.h"

#include "inet/common/ModuleAccess.h"
#include <inet/common/IInterfaceRegistrationListener.h>

#include <inet/applications/common/SocketTag_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/Udp.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include <inet/transportlayer/common/L4Tools.h>
#include <inet/networklayer/common/InterfaceEntry.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
#include <inet/networklayer/ipv4/IIpv4RoutingTable.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

#include <inet/linklayer/common/InterfaceTag_m.h>

Define_Module(IP2NR);

void IP2NR::initialize(int stage) {
	if (stage == inet::INITSTAGE_LOCAL) {
		stackGateOut_ = gate("stackNic$o");
		ipGateOut_ = gate("upperLayerOut");

		setNodeType(par("nodeType").stdstringValue());

		hoManager_ = nullptr;

		ueHold_ = false;

		binder_ = getBinder();

		dualConnectivityEnabled_ = getAncestorPar("dualConnectivityEnabled").boolValue();
		if (dualConnectivityEnabled_)
			sbTable_ = new SplitBearersTable();

		if (nodeType_ == ENODEB || nodeType_ == GNODEB) {
			// TODO not so elegant
			cModule *bs = getParentModule()->getParentModule();
			MacNodeId masterId = getAncestorPar("masterId");
			MacNodeId cellId = binder_->registerNode(bs, nodeType_, masterId);
			nodeId_ = cellId;
			nrNodeId_ = nodeId_;
		}
	}
	else if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT) {
		if (nodeType_ == ENODEB || nodeType_ == GNODEB) {
			registerInterface();
		}
		else if (nodeType_ == UE) {
			cModule *ue = getParentModule()->getParentModule();

			masterId_ = ue->par("masterId");
			nodeId_ = binder_->registerNode(ue, nodeType_, masterId_);
            nrMasterId_ = masterId_;
            nrNodeId_ = nodeId_;

			registerInterface();
		}
		else {
			throw cRuntimeError("unhandled node type: %i", nodeType_);
		}
	}
	else if (stage == inet::INITSTAGE_STATIC_ROUTING) {
		if (nodeType_ == UE) {
			// TODO: shift to routing stage
			// if the UE has been created dynamically, we need to manually add a default route having "wlan" as output interface
			// otherwise we are not able to reach devices outside the cellular network
			if (NOW > 0) {
				/**
				 * TODO:might need a bit more care, if interface has changed, the query might, too
				 */
				IIpv4RoutingTable *irt = getModuleFromPar<IIpv4RoutingTable>(par("routingTableModule"), this);
				Ipv4Route *defaultRoute = new Ipv4Route();
				defaultRoute->setDestination(Ipv4Address(inet::Ipv4Address::UNSPECIFIED_ADDRESS));
				defaultRoute->setNetmask(Ipv4Address(inet::Ipv4Address::UNSPECIFIED_ADDRESS));

				defaultRoute->setInterface(interfaceEntry);

				irt->addRoute(defaultRoute);

				// workaround for nodes using the HostAutoConfigurator:
				// Since the HostAutoConfigurator calls setBroadcast(true) for all
				// interfaces in setupNetworking called in INITSTAGE_NETWORK_CONFIGURATION
				// we must reset it to false since the cellular NIC does not support broadcasts
				// at the moment
				interfaceEntry->setBroadcast(false);
			}
		}
	}
	else if (stage == inet::INITSTAGE_TRANSPORT_LAYER) {
		registerMulticastGroups();
	}
}

void IP2NR::triggerHandoverUe(MacNodeId newMasterId, bool isNr) {
    EV << NOW << " IP2Nic::triggerHandoverUe - start holding packets" << endl;

    if (newMasterId != 0) {
        ueHold_ = true;
        nrMasterId_ = newMasterId;
        masterId_ = newMasterId;
    } else {

        nrMasterId_ = 0;
        masterId_ = 0;
    }
}

void IP2NR::toIpBs(Packet * pkt) {
	//std::cout << "IP2NR::toIpEnb start at " << simTime().dbl() << std::endl;

	auto ipHeader = pkt->peekAtFront<Ipv4Header>();
	auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
	networkProtocolInd->setProtocol(&Protocol::ipv4);
	networkProtocolInd->setNetworkProtocolHeader(ipHeader);
	pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
	pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

	EV << "IP2Nic::toIpBs - message from stack: send to IP layer" << endl;
	prepareForIpv4(pkt, &LteProtocol::ipv4uu);
	send(pkt, ipGateOut_);

//    if((ApplicationType)pkt->getTag<FlowControlInfo>()->getApplication() == V2X && getSystemModule()->par("v2vMulticastFlag").boolValue()){
//    	if(ipHeader->getDestAddress().getInt() != pkt->getTag<FlowControlInfo>()->getDstAddr()){
//    	    Ipv4Address newAddress(pkt->getTag<FlowControlInfo>()->getDstAddr());
//    		copiedHeader->setDestAddress(newAddress);
//    		auto t = pkt->popAtFront();
//    		pkt->insertAtFront(copiedHeader);
//    	}
//    }

	//std::cout << "IP2NR::toIpEnb end at " << simTime().dbl() << std::endl;
}

void IP2NR::registerInterface() {
	//std::cout << "IP2NR::registerInterface start at " << simTime().dbl() << std::endl;

	IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
	if (!ift)
		return;
	interfaceEntry = getContainingNicModule(this);
	interfaceEntry->setInterfaceName("wlan");

	interfaceEntry->setBroadcast(false);
	interfaceEntry->setMulticast(true);
	interfaceEntry->setLoopback(false);
	//changed the setting of the mtu
	interfaceEntry->setMtu(getSystemModule()->par("mtu").intValue());
	InterfaceToken token(0, getSimulation()->getUniqueNumber(), 64);
	interfaceEntry->setInterfaceToken(token);

	// capabilities
	interfaceEntry->setMulticast(true);
	interfaceEntry->setPointToPoint(true);

	//std::cout << "IP2NR::registerInterface end at " << simTime().dbl() << std::endl;
}

void IP2NR::toStackBs(Packet * pkt) {

	//std::cout << "IP2NR::toStackBs start at " << simTime().dbl() << std::endl;

	auto ipHeader = pkt->peekAtFront<Ipv4Header>();
	int transportProtocol = ipHeader->getProtocolId();
	auto srcAddr = ipHeader->getSrcAddress();
	auto destAddr = ipHeader->getDestAddress();
	unsigned short srcPort = 0;
	unsigned short dstPort = 0;
	MacNodeId destId = binder_->getMacNodeId(destAddr);

	short int tos = ipHeader->getTypeOfService();
	int headerSize = ipHeader->getHeaderLength().get();

	switch (transportProtocol) {
	case IP_PROT_TCP: {
		auto tcpHeader = pkt->peekDataAt<tcp::TcpHeader>(ipHeader->getChunkLength());
		headerSize += B(tcpHeader->getHeaderLength()).get();
		srcPort = tcpHeader->getSrcPort();
		dstPort = tcpHeader->getDestPort();
		break;
	}
	case IP_PROT_UDP: {
		headerSize += inet::UDP_HEADER_LENGTH.get();
		auto udpHeader = pkt->peekDataAt<inet::UdpHeader>(ipHeader->getChunkLength());
		srcPort = udpHeader->getSrcPort();
		dstPort = udpHeader->getDestPort();
		break;
	}
	}

	// prepare flow info for NIC
	pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(srcAddr.getInt());
	pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(destAddr.getInt());
	pkt->addTagIfAbsent<FlowControlInfo>()->setTypeOfService(tos);
	pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize);
	pkt->addTagIfAbsent<FlowControlInfo>()->setSrcPort(srcPort);
	pkt->addTagIfAbsent<FlowControlInfo>()->setDstPort(dstPort);
	// pkt->addTagIfAbsent<FlowControlInfo>()->setSequenceNumber(seqNums_[pair]++);

	MacNodeId master = binder_->getNextHop(destId);

	pkt->addTagIfAbsent<FlowControlInfo>()->setDestId(master);

	// mark packet for using NR
	if (!markPacket(pkt->getTag<FlowControlInfo>())) {
		delete pkt;
	}
	else {
		send(pkt, stackGateOut_);
	}

	//std::cout << "IP2NR::toStackEnb end at " << simTime().dbl() << std::endl;
}

void IP2NR::toStackUe(Packet * pkt) {

	//std::cout << "IP2NR::fromIpUe start at " << simTime().dbl() << std::endl;

	auto ipHeader = pkt->peekAtFront<Ipv4Header>();
	auto srcAddr = ipHeader->getSrcAddress();
	auto destAddr = ipHeader->getDestAddress();
	short int tos = ipHeader->getTypeOfService();
	unsigned short srcPort = 0;
	unsigned short dstPort = 0;
	int headerSize = ipHeader->getHeaderLength().get();
	int transportProtocol = ipHeader->getProtocolId();

	if (transportProtocol == IP_PROT_TCP) {
		auto tcpHeader = pkt->peekDataAt<tcp::TcpHeader>(ipHeader->getChunkLength());
		headerSize += B(tcpHeader->getHeaderLength()).get();
		srcPort = tcpHeader->getSrcPort();
		dstPort = tcpHeader->getDestPort();
	}
	else if (transportProtocol == IP_PROT_UDP) {
		headerSize += inet::UDP_HEADER_LENGTH.get();
		auto udpHeader = pkt->peekDataAt<inet::UdpHeader>(ipHeader->getChunkLength());
		srcPort = udpHeader->getSrcPort();
		dstPort = udpHeader->getDestPort();
	}

	pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(srcAddr.getInt());
	pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(destAddr.getInt());
	pkt->addTagIfAbsent<FlowControlInfo>()->setTypeOfService(tos);
	pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize);
	pkt->addTagIfAbsent<FlowControlInfo>()->setSrcPort(srcPort);
	pkt->addTagIfAbsent<FlowControlInfo>()->setDstPort(dstPort);
	//controlInfo->setSequenceNumber(seqNums_[pair]++);

	// mark packet for using NR
	if (!markPacket(pkt->getTag<FlowControlInfo>())) {
		delete pkt;
	}

	//** Send datagram to lte stack or LteIp peer **
	send(pkt, stackGateOut_);

	//std::cout << "IP2NR::fromIpUe end at " << simTime().dbl() << std::endl;
}

void IP2NR::fromIpUe(inet::Packet * datagram) {
	EV << "IP2Nic::fromIpUe - message from IP layer: send to stack: " << datagram->str() << std::endl;
	// Remove control info from IP datagram
	auto sockInd = datagram->removeTagIfPresent<SocketInd>();
	if (sockInd)
		delete sockInd;

	// Remove InterfaceReq Tag (we already are on an interface now)
	datagram->removeTagIfPresent<InterfaceReq>();

	if (ueHold_) {
		// hold packets until handover is complete
		ueHoldFromIp_.push_back(datagram);
	}
	else {
		if (masterId_ == 0 && nrMasterId_ == 0)  // UE is detached
				{
			EV << "IP2Nic::fromIpUe - UE is not attached to any serving node. Delete packet." << endl;
			delete datagram;
		}
		else
			toStackUe(datagram);
	}
}
