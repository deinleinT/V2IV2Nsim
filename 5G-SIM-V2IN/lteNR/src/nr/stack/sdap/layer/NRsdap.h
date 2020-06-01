/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include <tuple>
#include <map>
#include "nr/common/NRCommon.h"
#include "common/LteControlInfo.h"
#include "nr/stack/sdap/packet/SdapPdu_m.h"
#include "nr/stack/sdap/utils/QosHandler.h"
#include "nr/stack/sdap/entity/NRSdapEntity.h"
#include "stack/mac/layer/LteMacEnb.h"


using namespace omnetpp;

typedef std::tuple<MacNodeId, MacNodeId, ApplicationType> AddressTuple;
typedef std::map<AddressTuple, NRSdapEntity*> NRSdapEntities;

//simplified SDAP Protocol, maps qfi to packets
class NRsdap: public cSimpleModule {
public:
    void deleteEntities(MacNodeId nodeId);
    NRSdapEntities & getEntities(){
        Enter_Method_Silent("getEntities");
        return entities;
    }

protected:
    simsignal_t rlcThroughput_;
    QosHandler * qosHandler;
    LteNodeType nodeType;
    cGate *upperLayer;
    cGate *lowerLayer;
    cGate *upperLayerIn;
    cGate *lowerLayerIn;
    simsignal_t fromUpperLayer;
    simsignal_t toUpperLayer;
    simsignal_t fromLowerLayer;
    simsignal_t toLowerLayer;
    simsignal_t pkdrop;
    MacNodeId nodeId_;
    NRSdapEntities entities;
    unsigned int hoErrorCount;

protected:
    virtual void initialize(int stage) = 0;
    virtual void handleMessage(cMessage *msg);
    virtual void handleSelfMessage(cMessage *msg);
    virtual void fromLowerToUpper(cMessage * msg);
    virtual void fromUpperToLower(cMessage * msg);
    virtual void setTrafficInformation(cPacket* pkt, FlowControlInfo* lteInfo);
    virtual int numInitStages() const { return inet::INITSTAGE_NETWORK_LAYER+1; }
    virtual NRSdapEntity * getEntity(MacNodeId sender, MacNodeId dest, ApplicationType appType);
    virtual void finish();
};
