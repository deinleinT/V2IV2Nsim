/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include <map>
#include <iostream>
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "epc/gtp/TftControlInfo.h"
#include "epc/gtp/GtpUserMsg_m.h"
#include "epc/gtp_common.h"
#include "nr/corenetwork/binder/NRBinder.h"
#include "nr/common/NRCommon.h"

class GtpUserNR : public cSimpleModule
{
    UDPSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    NRBinder* binder_;

    std::map<TrafficFlowTemplateId, IPv4Address> tftTable_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

    // IP address of the PGW
    L3Address upfAddress_;

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    EpcNodeType ownerType_;

    EpcNodeType selectOwnerType(const char * type);

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    // receive and IP Datagram from the traffic filter, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromTrafficFlowFilter(IPv4Datagram * datagram);

    // receive a GTP-U packet from UDP, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(GtpUserMsg * gtpMsg);
};
