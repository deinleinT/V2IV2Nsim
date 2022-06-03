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
    virtual void handleSelfMessage(cMessage *msg) = 0;
    virtual void fromLowerToUpper(cMessage * msg) = 0;
    virtual void fromUpperToLower(cMessage * msg) = 0;
    virtual void setTrafficInformation(cPacket* pkt, FlowControlInfo* lteInfo);
    virtual int numInitStages() const { return inet::INITSTAGE_NETWORK_LAYER+1; }
    virtual NRSdapEntity * getEntity(MacNodeId sender, MacNodeId dest, ApplicationType appType);
    virtual void finish();
};
