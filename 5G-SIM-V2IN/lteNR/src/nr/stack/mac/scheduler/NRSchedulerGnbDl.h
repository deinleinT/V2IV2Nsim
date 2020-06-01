/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "common/LteCommon.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/scheduler/LteSchedulerEnbDl.h"
#include "stack/mac/scheduler/LteScheduler.h"

//see inherit class for method description
class NRSchedulerGnbDl : public LteSchedulerEnbDl
{

public:
	NRSchedulerGnbDl();
    virtual ~NRSchedulerGnbDl();

    virtual unsigned int scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible,
        std::vector<BandLimit>* bandLim = NULL, Remote antenna = MACRO, bool limitBl = false);

    virtual LteMacScheduleListWithSizes* schedule();

    virtual bool rtxschedule();

    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = NULL, Remote antenna = MACRO, bool limitBl = false);

};
