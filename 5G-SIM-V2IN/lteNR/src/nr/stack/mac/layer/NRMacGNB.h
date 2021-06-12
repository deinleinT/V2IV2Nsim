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


#include "nr/stack/mac/scheduler/NRSchedulerGnbUL.h"
#include "stack/mac/layer/NRMacGnb.h"

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "nr/stack/mac/scheduler/NRSchedulerGnbDl.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/NRAmc.h"
#include "stack/mac/amc/AmcPilotD2D.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/mac/packet/LteRac_m.h"
#include "common/LteCommon.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "nr/stack/sdap/utils/QosHandler.h"
#include "stack/mac/conflict_graph/DistanceBasedConflictGraph.h"
#include "stack/phy/layer/LtePhyBase.h"

class NRMacGNB: public NRMacGnb {

protected:

//    std::map<MacNodeId, std::unordered_map<unsigned short, RtxMapInfo>> rtxMap;
    QosHandler *qosHandler;
    int harqProcessesNR_;

    virtual void initialize(int stage);
    virtual void handleMessage(cMessage* msg);
    virtual void fromPhy(omnetpp::cPacket *pkt);;
    virtual void macSduRequest();
    virtual void macPduMake(MacCid cid);
    virtual bool bufferizePacket(cPacket* pkt);
    virtual void handleUpperMessage(cPacket* pkt);

    virtual void handleSelfMessage();
    virtual void flushHarqBuffers();

    virtual void macPduUnmake(omnetpp::cPacket* pkt);

    virtual void sendGrants(std::map<double, LteMacScheduleList>* scheduleList);


public:
    /*
     * for handover, delete the node from qosHandler
     */
    virtual void deleteNodeFromQosHandler(unsigned int nodeId) {
        qosHandler->deleteNode(nodeId);
    }

    NRMacGNB();
    virtual ~NRMacGNB();


};
