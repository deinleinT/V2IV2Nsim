/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "nr/stack/sdap/utils/QosHandler.h"

// see inherit class for method description
class NRPdcpRrcGnb : public LtePdcpRrcEnb {
public:
    virtual void resetConnectionTable(MacNodeId masterId, MacNodeId nodeId);
    virtual void exchangeConnection(MacNodeId nodeId, MacNodeId oldMasterId, MacNodeId newMasterId,NRPdcpRrcGnb *oldMasterPdcp, NRPdcpRrcGnb *newMasterPdcp);

protected:
    QosHandler * qosHandler;
    void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void fromDataPort(cPacket *pkt);
    virtual void toDataPort(cPacket * pkt);
};
