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

class NRMacGNB : public NRMacGnb
{

public:
    virtual void insertRtxMap(MacNodeId ueid, unsigned char currentProcess, Codeword codeword)
    {
        if (rtxMap[ueid].size() == 0) {
            unsigned short order = rtxMap[ueid].size() + 1;
            RtxMapInfo temp(codeword, currentProcess, NOW, order);
            std::unordered_map<unsigned short, RtxMapInfo> mapInfo = rtxMap[ueid];
            mapInfo.insert(std::make_pair(order, temp));
            rtxMap[ueid] = mapInfo;
        }
        else {
            unsigned short nextOrder = 17;
            for (auto &var : rtxMap[ueid]) {
                unsigned short order = var.second.order + 1;
                if (order < nextOrder) {
                    nextOrder = order;
                }
                break;
            }

            RtxMapInfo temp(codeword, currentProcess, NOW, nextOrder);
            std::unordered_map<unsigned short, RtxMapInfo> mapInfo = rtxMap[ueid];
            mapInfo.insert(std::make_pair(nextOrder, temp));
            rtxMap[ueid] = mapInfo;
        }
    }

    void deleteFromRtxMap(MacNodeId ueid)
    {
        rtxMap.erase(ueid);
    }

protected:

    std::map<MacNodeId, std::unordered_map<unsigned short, RtxMapInfo>> rtxMap;
    QosHandler *qosHandler;
    int harqProcessesNR_;

    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void fromPhy(omnetpp::cPacket *pkt);
    ;
    virtual void macSduRequest();
    virtual void macPduMake(MacCid cid);
    virtual bool bufferizePacket(cPacket *pkt);
    virtual void handleUpperMessage(cPacket *pkt);

    virtual void handleSelfMessage();

    virtual void macPduUnmake(omnetpp::cPacket *pkt);

    virtual void sendGrants(std::map<double, LteMacScheduleList> *scheduleList);

    virtual void macHandleRac(cPacket *pkt);

public:
    /*
     * for handover, delete the node from qosHandler
     */
    virtual void deleteNodeFromQosHandler(unsigned int nodeId)
    {
        qosHandler->deleteNode(nodeId);
    }

    virtual void deleteQueues(MacNodeId nodeId)
    {
        LteMacBase::deleteQueues(nodeId);

        LteMacBufferMap::iterator bit;
        for (bit = bsrbuf_.begin(); bit != bsrbuf_.end();)
        {
            if (MacCidToNodeId(bit->first) == nodeId)
            {
                delete bit->second; // Delete Queue
                bsrbuf_.erase(bit++); // Delete Elem
            }
            else
            {
                ++bit;
            }
        }

        //update harq status in schedulers
    //    enbSchedulerDl_->updateHarqDescs();
    //    enbSchedulerUl_->updateHarqDescs();

        // remove active connections from the schedulers
        enbSchedulerDl_->removeActiveConnections(nodeId);
        enbSchedulerUl_->removeActiveConnections(nodeId);

        // remove pending RAC requests
        enbSchedulerUl_->removePendingRac(nodeId);
    }

    NRMacGNB();
    virtual ~NRMacGNB();

};
