/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
 */

#pragma once

#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/buffer/LteMacBuffer.h"

//see inherit class for method description
class NRSchedulerGnbUl: public LteSchedulerEnbUl {
protected:


	virtual LteMacScheduleListWithSizes* schedule();

	virtual bool racschedule();

	virtual bool rtxschedule();

	virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid, std::vector<BandLimit> *bandLim = NULL, Remote antenna = MACRO, bool limitBl = false);

};
