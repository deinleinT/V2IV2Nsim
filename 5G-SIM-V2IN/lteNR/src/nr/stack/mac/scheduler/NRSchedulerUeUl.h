/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "common/LteCommon.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "stack/mac/layer/LteMacUe.h"
#include "nr/stack/mac/scheduler/NRLcgScheduler.h"

//see inherit class for method description
class NRSchedulerUeUl : public LteSchedulerUeUl
{

  public:

    /* Performs the standard LCG scheduling algorithm
     * @returns reference to scheduling list
     */

    virtual LteMacScheduleListWithSizes* schedule();

    /*
     * constructor
     */
    NRSchedulerUeUl(LteMacUe * mac);
    /*
     * destructor
     */
    virtual ~NRSchedulerUeUl();
};
