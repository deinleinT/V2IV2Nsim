/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "stack/mac/layer/LteMacUe.h"
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
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "corenetwork/binder/LteBinder.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"

class NRMacUe: public LteMacUe {

public:
    void resetScheduleList(){ scheduleListWithSizes_.clear();};


protected:

    LteMacScheduleListWithSizes scheduleListWithSizes_;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void fromPhy(cPacket *pkt);
    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual int macSduRequest();

    virtual void macHandleGrant(cPacket* pkt);

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(cPacket* pkt);

    /**
     * Flush Tx H-ARQ buffers for the user
     */
    virtual void flushHarqBuffers();

    /**
     * Main loop
     */
    virtual void handleSelfMessage();

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
    virtual void handleUpperMessage(cPacket* pkt);

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(cPacket* pkt);

    virtual void checkRAC();
};

