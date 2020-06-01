/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "epc/gtp/TftControlInfo.h"
#include "epc/gtp_common.h"
#include "nr/common/NRCommon.h"
#include "nr/corenetwork/binder/NRBinder.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "nr/stack/sdap/utils/QosHandler.h"


class TrafficFlowFilterNR : public cSimpleModule
{

  protected:
    QosHandler * qosHandler;
    // specifies the type of the node that contains this filter (it can be ENB or PGW
    // the filterTable_ will be indexed differently depending on this parameter
    EpcNodeType ownerType_;

    // reference to the LTE Binder module
    NRBinder* binder_;

    // if this flag is set, each packet received from the radio network, having the same radio network as destination
    // must be re-sent down without going through the Internet
    bool fastForwarding_;

    TrafficFilterTemplateTable filterTable_;

    EpcNodeType selectOwnerType(const char * type);
    virtual int numInitStages() const { return INITSTAGE_LAST+1; }
    virtual void initialize(int stage);

    // TrafficFlowFilter module may receive messages only from the input interface of its compound module
    virtual void handleMessage(cMessage *msg);

    // functions for managing filter tables
    TrafficFlowTemplateId findTrafficFlow(L3Address srcAddress, L3Address destAddress);

};
