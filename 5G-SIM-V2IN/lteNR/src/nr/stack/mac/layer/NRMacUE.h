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

#include "stack/mac/layer/NRMacUe.h"

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "nr/stack/mac/scheduler/NRSchedulerUeUl.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/amc/UserTxParams.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/ModuleAccess.h"

#include "common/binder/Binder.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"

class NRMacUE: public NRMacUe {

public:
    /*
     * for handover, delete the node from qosHandler
     */
    virtual void deleteNodeFromQosHandler(unsigned int nodeId) {
        qosHandler->deleteNode(nodeId);
    }

    virtual void resetParamsOnHandover(){
        firstTx = false;
        currentHarq_ = 0;
        racRequested_ = false;
        bsrTriggered_ = false;
        requestedSdus_ = 0;
        debugHarq_ = false;
        racBackoffTimer_ = 0;
        maxRacTryouts_ = 0;
        currentRacTry_ = 0;
        minRacBackoff_ = 0;
        maxRacBackoff_ = 1;
        raRespTimer_ = 0;
        raRespWinStart_ = 3;
    }

protected:
    bool useQosModel;
    unsigned int harqProcessesNR_;
	virtual void initialize(int stage);
	virtual void handleMessage(cMessage *msg);
	virtual void fromPhy(omnetpp::cPacket *pkt);
	/**
	 * macSduRequest() sends a message to the RLC layer
	 * requesting MAC SDUs (one for each CID),
	 * according to the Schedule List.
	 */
	virtual int macSduRequest();

	virtual void macHandleGrant(cPacket *pktAux);

	/**
	 * macPduUnmake() extracts SDUs from a received MAC
	 * PDU and sends them to the upper layer.
	 *
	 * @param pkt container packet
	 */
	virtual void macPduUnmake(cPacket *pkt);

	/**
	 * Flush Tx H-ARQ buffers for the user
	 */
	virtual void flushHarqBuffers();

	/**
	 * Main loop
	 */
	virtual void handleSelfMessage();
//	virtual void handleSelfMessageWithNRHarq();

	/**
	 * macPduMake() creates MAC PDUs (one for each CID)
	 * by extracting SDUs from Real Mac Buffers according
	 * to the Schedule List.
	 * It sends them to H-ARQ (at the moment lower layer)
	 *
	 * On UE it also adds a BSR control element to the MAC PDU
	 * containing the size of its buffer (for that CID)
	 */
	virtual void macPduMake(MacCid cid = 0);

	/**
	 * handleUpperMessage() is called every time a packet is
	 * received from the upper layer
	 */
	virtual void handleUpperMessage(cPacket *pkt);

	/**
	 * bufferizePacket() is called every time a packet is
	 * received from the upper layer
	 */
	virtual bool bufferizePacket(omnetpp::cPacket* pkt);

	virtual void checkRAC();

	virtual void checkRACQoSModel();
};

