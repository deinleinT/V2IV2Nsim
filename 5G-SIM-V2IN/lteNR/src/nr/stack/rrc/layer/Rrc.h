/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "nr/common/NRCommon.h"
#include "nr/stack/rrc/packet/RrcMsg_m.h"

using namespace omnetpp;

//simplified implementation of rrc protocol
//currently not in usage

class Rrc : public cSimpleModule
{
protected:
    LteNodeType nodeType;
    cGate * incomingGate;
    cGate * outgoingGate;

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void incomingRRC(cMessage *& msg);
    virtual void handleSelfMessage(cMessage *msg);
};
