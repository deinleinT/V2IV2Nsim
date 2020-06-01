/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "nr/stack/sdap/utils/QosHandler.h"

//see inherit class for method description
class NRPdcpRrcUe: public LtePdcpRrcUe {
protected:
    QosHandler * qosHandler;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void fromDataPort(cPacket *pkt);
    virtual void toDataPort(cPacket * pkt);
};

