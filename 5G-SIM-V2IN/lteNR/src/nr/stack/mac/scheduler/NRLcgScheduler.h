/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "stack/mac/scheduler/LcgScheduler.h"
#include "stack/mac/buffer/LteMacBuffer.h"

//see inherit class for method description
class NRLcgScheduler : public LcgScheduler {
public:
    NRLcgScheduler(LteMacUe * mac):LcgScheduler(mac){};
    virtual ~NRLcgScheduler();
    virtual ScheduleListSizes& schedule(unsigned int availableBytes, Direction grantDir = UL);
};


