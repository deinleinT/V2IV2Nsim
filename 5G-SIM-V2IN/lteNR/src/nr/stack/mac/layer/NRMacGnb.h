/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once


#include "stack/mac/layer/LteMacEnb.h"

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "nr/stack/mac/scheduler/NRSchedulerGnbDl.h"
#include "nr/stack/mac/scheduler/NRSchedulerGnbUl.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/mac/packet/LteRac_m.h"
#include "common/LteCommon.h"
#include "stack/mac/packet/LteMacSduRequest.h"

class NRMacGnb: public LteMacEnb {

public:


protected:

//    QosHandler * qosHandler;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage* msg);
    virtual void fromPhy(cPacket *pkt);
    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual void macSduRequest();

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List (stored after scheduling).
     * It sends them to H-ARQ
     */
    virtual void macPduMake(MacCid cid);

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(cPacket* pkt);

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(cPacket* pkt);

    /**
     * Main loop
     */
    virtual void handleSelfMessage();

    /**
     * Flush Tx H-ARQ buffers for all users
     */
    virtual void flushHarqBuffers();

    virtual void macPduUnmake(cPacket* pkt);

    virtual void sendGrants(LteMacScheduleListWithSizes* scheduleList);

public:

    NRMacGnb();
    virtual ~NRMacGnb();


};
