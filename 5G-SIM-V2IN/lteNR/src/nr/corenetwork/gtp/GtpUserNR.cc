/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
 */

#include "nr/corenetwork/gtp/GtpUserNR.h"
#include "corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include <iostream>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/common/packet/printer/PacketPrinter.h>

Define_Module(GtpUserNR);

using namespace omnetpp;
using namespace inet;

void GtpUserNR::initialize(int stage) {

	//instead of calling GtpUser::initialize(stage);

	cSimpleModule::initialize(stage);

	// wait until all the IP addresses are configured
	if (stage != inet::INITSTAGE_APPLICATION_LAYER)
		return;
	localPort_ = par("localPort");

	// get reference to the binder
	binder_ = getNRBinder();

	// transport layer access
	socket_.setOutputGate(gate("socketOut"));
	socket_.bind(localPort_);

	tunnelPeerPort_ = par("tunnelPeerPort");

	ownerType_ = selectOwnerType(getAncestorPar("nodeType"));

	// find the address of the core network gateway
	if (ownerType_ != PGW && ownerType_ != UPF) {
		// check if this is a gNB connected as secondary node
		bool connectedBS = isBaseStation(ownerType_) && getParentModule()->gate("ppp$o")->isConnected();

		if (connectedBS || ownerType_ == GTPENDPOINT)
			gwAddress_ = L3AddressResolver().resolve(getAncestorPar("gateway"));
	}

	//mec
	if (getParentModule()->hasPar("meHost")) {
		std::string meHost = getParentModule()->par("meHost").stringValue();
		if (isBaseStation(ownerType_) && strcmp(meHost.c_str(), "")) {

			std::stringstream meHostName;
			meHostName << meHost.c_str() << ".gtpEndpoint";
			meHostGtpEndpoint = meHostName.str();
			meHostGtpEndpointAddress = inet::L3AddressResolver().resolve(meHostGtpEndpoint.c_str());

			EV << "GtpUser::initialize - meHost: " << meHost << " meHostGtpEndpointAddress: " << meHostGtpEndpointAddress.str() << endl;

			std::stringstream meHostName2;
			meHostName2 << meHost.c_str() << ".virtualisationInfrastructure";
			meHostVirtualisationInfrastructure = meHostName.str();
			meHostVirtualisationInfrastructureAddress = inet::L3AddressResolver().resolve(meHostVirtualisationInfrastructure.c_str());
		}
	}

	if (isBaseStation(ownerType_)) {
		myMacNodeID = getParentModule()->par("macNodeId");
	}

	ie_ = detectInterface();

}

//TODO
void GtpUserNR::handleFromTrafficFlowFilter(inet::Packet * datagram) {

	//std::cout << "GtpUserNR::handleFromTrafficFlowFilter start at " << simTime().dbl() << std::endl;

	// extract control info from the datagram
	TftControlInfo *tftInfo = datagram->removeTag<TftControlInfo>();
	TrafficFlowTemplateId flowId = tftInfo->getTft();
	datagram->setControlInfo(tftInfo);
	//delete (tftInfo);

	// If we are on the eNB and the flowId represents the ID of this eNB, forward the packet locally
	if (flowId == 0) {
		// local delivery
		send(datagram, "pppGate");
	}
	else {
		// create a new GtpUserMessage
		// and encapsulate the datagram within the GtpUserMessage
		auto header = makeShared<GtpUserMsg>();
		header->setTeid(0);
		header->setChunkLength(B(8));
		auto gtpPacket = new Packet(datagram->getName());
		gtpPacket->insertAtFront(header);
		auto data = datagram->peekData();
		gtpPacket->insertAtBack(data);

		delete datagram;

		L3Address tunnelPeerAddress;
		if (flowId == -1) // send to the PGW or UPF
				{
			tunnelPeerAddress = gwAddress_;
		}
		//mec
		else if (flowId == -3) // for E/GNB --> tunneling to the connected GTPENDPOINT (ME Host)
				{
			tunnelPeerAddress = meHostGtpEndpointAddress;
		}
		//
		//end mec
		else {
			// get the symbolic IP address of the tunnel destination ID
			// then obtain the address via IPvXAddressResolver
			const char *symbolicName = binder_->getModuleNameByMacNodeId(flowId);
			tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
		}
		socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);

	}

	//std::cout << "GtpUserNR::handleFromTrafficFlowFilter end at " << simTime().dbl() << std::endl;

}

//TODO
void GtpUserNR::handleFromUdp(inet::Packet * pkt) {

	//std::cout << "GtpUserNR::handleFromUdp start at " << simTime().dbl() << std::endl;

	// re-create the original IP datagram and send it to the local network
	auto originalPacket = new Packet(pkt->getName());
	auto gtpUserMsg = pkt->popAtFront<GtpUserMsg>();
	originalPacket->insertAtBack(pkt->peekData());
	originalPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

	delete pkt;

	if (ownerType_ == PGW || ownerType_ == UPF) {
		const auto &hdr = originalPacket->peekAtFront<Ipv4Header>();
		const Ipv4Address &destAddr = hdr->getDestAddress();
		MacNodeId destId = binder_->getMacNodeId(destAddr);

		if (destId != 0) {
			// create a new GtpUserMessage
			// encapsulate the datagram within the GtpUserMsg
			auto header = makeShared<GtpUserMsg>();
			header->setTeid(0);
			header->setChunkLength(B(8));
			auto gtpPacket = new Packet(originalPacket->getName());
			gtpPacket->insertAtFront(header);
			auto data = originalPacket->peekData();
			gtpPacket->insertAtBack(data);
			delete originalPacket;

			MacNodeId destMaster = binder_->getNextHop(destId);
			const char *symbolicName = binder_->getModuleNameByMacNodeId(destMaster);
			L3Address tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
			socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);
			EV << "GtpUser::handleFromUdp - Destination is a MEC server. Sending GTP packet to " << symbolicName << endl;
			return;
		}
	}

	//mec
	if (isBaseStation(ownerType_)) {
		const auto &hdr = originalPacket->peekAtFront<Ipv4Header>();
		const Ipv4Address &destAddr = hdr->getDestAddress();

		// add Interface-Request for cellular NIC
		if (ie_ != nullptr)
			originalPacket->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie_->getInterfaceId());

		MacNodeId destId = binder_->getMacNodeId(destAddr);
		MacNodeId destMaster = binder_->getNextHop(destId);

		EV << "GtpUser::handleFromUdp - DestMaster: " << destMaster << " (MacNodeId) my: " << myMacNodeID << " (MacNodeId)" << endl;

		L3Address tunnelPeerAddress;
		//checking if serving eNodeB it's --> send to the Radio-NIC
		if (myMacNodeID == destMaster) {
			EV << "GtpUser::handleFromUdp - Datagram local delivery to " << destAddr.str() << endl;
			// local delivery
			send(originalPacket, "pppGate");
			return;
		}
		else if (destAddr.operator ==(meHostVirtualisationInfrastructureAddress.toIpv4())) {
			//tunneling to the ME Host GtpEndpoint
			EV << "GtpUser::handleFromUdp - Datagram for " << destAddr.str() << ": tunneling to (GTPENDPOINT) " << meHostGtpEndpointAddress.str()
						<< endl;
			tunnelPeerAddress = meHostGtpEndpointAddress;
		}
		else {
			//tunneling to the PGW/UPF
			EV << "GtpUser::handleFromUdp - Datagram for " << destAddr.str() << ": tunneling to the CN gateway " << gwAddress_.str() << endl;
			tunnelPeerAddress = gwAddress_;

		}

		// send the message to the correct BS or to the Internet, through the PGW
		// * create a new GtpUserMessage
		// * encapsulate the datagram within the GtpUserMsg
		auto header = makeShared<GtpUserMsg>();
		header->setTeid(0);
		header->setChunkLength(B(8));
		auto gtpMsg = new Packet(originalPacket->getName());
		gtpMsg->insertAtFront(header);
		auto data = originalPacket->peekData();
		gtpMsg->insertAtBack(data);
		delete originalPacket;

		// create a new GtpUserMessage
		socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);

	}
	if (ownerType_ == GTPENDPOINT) {
		//deliver to the MEHosto VirtualisationInfrastructure!
		send(originalPacket, "pppGate");
		return;
	}
	//end mec

	// destination is outside the LTE network
	send(originalPacket, "pppGate");
}
