/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/epc/gtp/GtpUserNR.h"

Define_Module(GtpUserNR);

void GtpUserNR::initialize(int stage) {
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_ = getNRBinder();

    socket_.setOutputGate(gate("udpOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    //upfAddress_ = L3AddressResolver().resolve("upf");

    ownerType_ = selectOwnerType(getAncestorPar("nodeType"));

    if(ownerType_ == ENB || ownerType_ == GNB){
    	getParentModule()->gate("ppp$o")->getNextGate()->getOwnerModule()->getName();
    	upfAddress_ = L3AddressResolver().resolve(getParentModule()->gate("ppp$o")->getNextGate()->getOwnerModule()->getName());
    }else if(ownerType_ == USER_PLANE_FUNCTION){
    	upfAddress_ = L3AddressResolver().resolve(getParentModule()->getName());
    }
}

EpcNodeType GtpUserNR::selectOwnerType(const char * type) {

    //std::cout << "GtpUserNR::selectOwnerType start at " << simTime().dbl() << std::endl;

    //EV << "GtpUserNR::selectOwnerType - setting owner type to " << type << endl;
    if (strcmp(type, "GNODEB") == 0)
        return GNB;
    else if (strcmp(type, "UPF") == 0 || strcmp(type, "PGW") == 0)
        return USER_PLANE_FUNCTION;
    else if (strcmp(type, "ENODEB") == 0)
        return ENB;

    //std::cout << "GtpUserNR::selectOwnerType end at " << simTime().dbl() << std::endl;

    error("GtpUserNR::selectOwnerType - unknown owner type [%s]. Aborting...",
            type);
}

void GtpUserNR::handleMessage(cMessage *msg) {

    //std::cout << "GtpUserNR::handleMessage start at " << simTime().dbl() << std::endl;

    if (strcmp(msg->getArrivalGate()->getFullName(), "trafficFlowFilterGate")
            == 0) {
        //EV << "GtpUserNR::handleMessage - message from trafficFlowFilter" << endl;
        // obtain the encapsulated IPv4 datagram
        IPv4Datagram * datagram = check_and_cast<IPv4Datagram*>(msg);
        handleFromTrafficFlowFilter(datagram);
    } else if (strcmp(msg->getArrivalGate()->getFullName(), "udpIn") == 0) {
        //EV << "GtpUserNR::handleMessage - message from udp layer" << endl;

        GtpUserMsg * gtpMsg = check_and_cast<GtpUserMsg *>(msg);
        handleFromUdp(gtpMsg);
    }

    //std::cout << "GtpUserNR::handleMessage end at " << simTime().dbl() << std::endl;
}

void GtpUserNR::handleFromTrafficFlowFilter(IPv4Datagram * datagram) {

	//std::cout << "GtpUserNR::handleFromTrafficFlowFilter start at " << simTime().dbl() << std::endl;

	// extract control info from the datagram
	TftControlInfo * tftInfo = check_and_cast<TftControlInfo *>(datagram->removeControlInfo());
	TrafficFlowTemplateId flowId = tftInfo->getTft();
	datagram->setControlInfo(tftInfo);
	//delete (tftInfo);

	//EV << "GtpUserNR::handleFromTrafficFlowFilter - Received a tftMessage with flowId[" << flowId << "]" << endl;

	// If we are on the eNB and the flowId represents the ID of this eNB, forward the packet locally
	if (flowId == 0) {
		// local delivery
		send(datagram, "pppGate");
	} else {
		// create a new GtpUserSimplifiedMessage
		GtpUserMsg * gtpMsg = new GtpUserMsg();
		gtpMsg->setName("GtpUserMessage");

		// encapsulate the datagram within the GtpUserSimplifiedMessage
		gtpMsg->encapsulate(datagram);

		L3Address tunnelPeerAddress;
		if (flowId == -1) // send to the PGW
				{
			tunnelPeerAddress = upfAddress_;
		} else {
			// get the symbolic IP address of the tunnel destination ID
			// then obtain the address via IPvXAddressResolver
			const char* symbolicName = binder_->getModuleNameByMacNodeId(flowId);
			tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
		}
		socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
	}

	//std::cout << "GtpUserNR::handleFromTrafficFlowFilter end at " << simTime().dbl() << std::endl;

}

void GtpUserNR::handleFromUdp(GtpUserMsg * gtpMsg) {

	//std::cout << "GtpUserNR::handleFromUdp start at " << simTime().dbl() << std::endl;

	//EV << "GtpUserSimplified::handleFromUdp - Decapsulating and sending to local connection." << endl;

	// obtain the original IP datagram and send it to the local network
	IPv4Datagram * datagram = check_and_cast<IPv4Datagram*>(gtpMsg->decapsulate());
	delete (gtpMsg);

	if (ownerType_ == USER_PLANE_FUNCTION) {
		IPv4Address& destAddr = datagram->getDestAddress();
		MacNodeId destId = binder_->getMacNodeId(destAddr);
		if (destId != 0) {
			// create a new GtpUserSimplifiedMessage
			GtpUserMsg * gtpMsg = new GtpUserMsg();
			gtpMsg->setName("GtpUserMessage");

			// encapsulate the datagram within the GtpUserSimplifiedMessage
			gtpMsg->encapsulate(datagram);

			MacNodeId destMaster = binder_->getNextHop(destId);
			const char* symbolicName = binder_->getModuleNameByMacNodeId(destMaster);
			L3Address tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
			socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
			//EV << "GtpUserSimplified::handleFromUdp - Destination is a MEC server. Sending GTP packet to " << symbolicName << endl;
		} else {
			// destination is outside the network
			//EV << "GtpUserSimplified::handleFromUdp - Deliver datagram to the internet " << endl;
			send(datagram, "pppGate");
		}
	} else if (ownerType_ == ENB || ownerType_ == GNB) {
		IPv4Address& destAddr = datagram->getDestAddress();
		MacNodeId destId = binder_->getMacNodeId(destAddr);
		if (destId != 0) {
			MacNodeId gnbId = getAncestorPar("macNodeId");
			MacNodeId destMaster = binder_->getNextHop(destId);
			if (destMaster == gnbId) {
				EV << "GtpUserSimplified::handleFromUdp - Deliver datagram to the LTE NIC " << endl;
				send(datagram, "pppGate");
				return;
			}
		}
		// send the message to the correct eNB or to the internet, through the PGW
		// create a new GtpUserSimplifiedMessage
		GtpUserMsg * gtpMsg = new GtpUserMsg();
		gtpMsg->setName("GtpUserMessage");

		// encapsulate the datagram within the GtpUserSimplifiedMessage
		gtpMsg->encapsulate(datagram);

		socket_.sendTo(gtpMsg, upfAddress_, tunnelPeerPort_);

		EV << "GtpUserSimplified::handleFromUdp - Destination is not served by this eNodeB. Sending GTP packet to the PGW" << endl;
	}

	//std::cout << "GtpUserNR::handleFromUdp end at " << simTime().dbl() << std::endl;
}
