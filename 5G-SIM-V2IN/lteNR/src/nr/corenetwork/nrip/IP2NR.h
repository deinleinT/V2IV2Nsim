/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "corenetwork/lteip/IP2lte.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"
#include "inet/transportlayer/udp/UDPPacket.h"
#include "inet/transportlayer/udp/UDP.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/configurator/ipv4/IPv4NetworkConfigurator.h"
#include "inet/networklayer/ipv4/IIPv4RoutingTable.h"

class IP2NR: public IP2lte {
protected:
	virtual void handleMessage(cMessage *msg);
	virtual void toStackEnb(IPv4Datagram * datagram);
	virtual void toIpEnb(cMessage * msg);
	//virtual void fromIpUe(IPv4Datagram * datagram);

	virtual void toStackUe(IPv4Datagram* datagram);
};
