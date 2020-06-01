/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "nr/stack/sdap/layer/NRsdap.h"
#include "nr/common/NRCommon.h"
#include "common/LteControlInfo.h"
#include "nr/stack/sdap/packet/SdapPdu_m.h"
#include "nr/stack/sdap/utils/QosHandler.h"

using namespace omnetpp;

class NRsdapUE: public NRsdap {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void handleSelfMessage(cMessage *msg);
    virtual void fromLowerToUpper(cMessage * msg);
    virtual void fromUpperToLower(cMessage * msg);

};
