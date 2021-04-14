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

#include "nr/corenetwork/nrip/IP2NR.h"
#include "inet/common/ModuleAccess.h"

Define_Module(IP2NR);

void IP2NR::toIpEnb(cMessage * msg)
{
    //std::cout << "IP2NR::toIpEnb start at " << simTime().dbl() << std::endl;

    //EV << "IP2lte::toIpEnb - message from stack: send to IP layer" << endl;
	FlowControlInfo * lteInfo = check_and_cast<FlowControlInfo*>(msg->getControlInfo());
    if((ApplicationType)lteInfo->getApplication() == V2X && getSystemModule()->par("v2vMulticastFlag").boolValue()){
    	IPv4Datagram * packet = check_and_cast<IPv4Datagram*>(msg);
    	if(packet->getDestAddress().getInt() != lteInfo->getDstAddr()){
    		IPv4Address newAddress(lteInfo->getDstAddr());
    		packet->setDestAddress(newAddress);
    	}
    }

	send(msg,ipGateOut_);

    //std::cout << "IP2NR::toIpEnb end at " << simTime().dbl() << std::endl;
}

void IP2NR::registerInterface()
{
    //std::cout << "IP2NR::registerInterface start at " << simTime().dbl() << std::endl;

    InterfaceEntry * interfaceEntry;
    inet::IInterfaceTable *ift = inet::getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;
    interfaceEntry = new InterfaceEntry(this);
    interfaceEntry->setName("wlan");
    interfaceEntry->setBroadcast(true);
    interfaceEntry->setMulticast(true);
    //from inherit class
    //changed the setting of the mtu
    interfaceEntry->setMtu(getSystemModule()->par("mtu").intValue());
    ift->addInterface(interfaceEntry);


    //std::cout << "IP2NR::registerInterface end at " << simTime().dbl() << std::endl;
}

void IP2NR::handleMessage(cMessage *msg) {

    //std::cout << "IP2NR::handleMessage start at " << simTime().dbl() << std::endl;

	IP2lte::handleMessage(msg);

	//std::cout << "IP2NR::handleMessage end at " << simTime().dbl() << std::endl;
}

void IP2NR::toStackEnb(IPv4Datagram* datagram) {

    //std::cout << "IP2NR::toStackEnb start at " << simTime().dbl() << std::endl;

	// obtain the encapsulated transport packet
	cPacket * transportPacket = datagram->getEncapsulatedPacket();

	// 5-Tuple infos
	unsigned short srcPort = 0;
	unsigned short dstPort = 0;
	int transportProtocol = datagram->getTransportProtocol();
	IPv4Address srcAddr = datagram->getSrcAddress(), destAddr = datagram->getDestAddress();
	MacNodeId destId = binder_->getMacNodeId(destAddr);

	// if needed, create a new structure for the flow
	AddressPair pair(srcAddr, destAddr);
	if (seqNums_.find(pair) == seqNums_.end()) {
		std::pair<AddressPair, unsigned int> p(pair, 0);
		seqNums_.insert(p);
	}

	int headerSize = 0;

	switch (transportProtocol) {
		case IP_PROT_TCP:
			inet::tcp::TCPSegment* tcpseg;
			tcpseg = check_and_cast<inet::tcp::TCPSegment*>(transportPacket);
			srcPort = tcpseg->getSrcPort();
			dstPort = tcpseg->getDestPort();
			headerSize += tcpseg->getHeaderLength();
			break;
		case IP_PROT_UDP:
			inet::UDPPacket* udppacket;
			udppacket = check_and_cast<inet::UDPPacket*>(transportPacket);
			srcPort = udppacket->getSourcePort();
			dstPort = udppacket->getDestinationPort();
			headerSize += UDP_HEADER_BYTES;
			break;
	}

	FlowControlInfo * controlInfo = new FlowControlInfo();
	controlInfo->setSrcAddr(srcAddr.getInt());
	controlInfo->setDstAddr(destAddr.getInt());
	controlInfo->setSrcPort(srcPort);
	controlInfo->setDstPort(dstPort);

	controlInfo->setSequenceNumber(seqNums_[pair]++);
	controlInfo->setHeaderSize(headerSize);

	MacNodeId master = binder_->getNextHop(destId);

	controlInfo->setDestId(master);
	printControlInfo(controlInfo);
	datagram->setControlInfo(controlInfo);

	send(datagram, stackGateOut_);

	//std::cout << "IP2NR::toStackEnb end at " << simTime().dbl() << std::endl;
}

void IP2NR::toStackUe(IPv4Datagram * datagram) {

    //std::cout << "IP2NR::fromIpUe start at " << simTime().dbl() << std::endl;

	// Remove control info from IP datagram
	//delete (datagram->removeControlInfo());

	// obtain the encapsulated transport packet
	cPacket * transportPacket = datagram->getEncapsulatedPacket();

	// 5-Tuple infos
	unsigned short srcPort = 0;
	unsigned short dstPort = 0;
	int transportProtocol = datagram->getTransportProtocol();
	IPv4Address srcAddr = datagram->getSrcAddress(), destAddr = datagram->getDestAddress();

	// if needed, create a new structure for the flow
	AddressPair pair(srcAddr, destAddr);
	if (seqNums_.find(pair) == seqNums_.end()) {
		std::pair<AddressPair, unsigned int> p(pair, 0);
		seqNums_.insert(p);
	}

	int headerSize = datagram->getHeaderLength();

	// inspect packet depending on the transport protocol type
	switch (transportProtocol) {
		case IP_PROT_TCP:
			inet::tcp::TCPSegment* tcpseg;
			tcpseg = check_and_cast<inet::tcp::TCPSegment*>(transportPacket);
			srcPort = tcpseg->getSrcPort();
			dstPort = tcpseg->getDestPort();
			headerSize += tcpseg->getHeaderLength();
			break;
		case IP_PROT_UDP:
			UDPPacket* udppacket;
			udppacket = check_and_cast<UDPPacket*>(transportPacket);
			srcPort = udppacket->getSourcePort();
			dstPort = udppacket->getDestinationPort();
			headerSize += UDP_HEADER_BYTES;
			break;
	}

	FlowControlInfo * controlInfo = new FlowControlInfo();
	controlInfo->setSrcAddr(srcAddr.getInt());
	controlInfo->setDstAddr(destAddr.getInt());
	controlInfo->setSrcPort(srcPort);
	controlInfo->setDstPort(dstPort);
	controlInfo->setSequenceNumber(seqNums_[pair]++);
	controlInfo->setHeaderSize(headerSize);
	printControlInfo(controlInfo);

	datagram->setControlInfo(controlInfo);

	//** Send datagram to lte stack or LteIp peer **
	send(datagram, stackGateOut_);

	//std::cout << "IP2NR::fromIpUe end at " << simTime().dbl() << std::endl;
}
