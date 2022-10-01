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

#include "stack/mac/scheduler/NRSchedulerGnbUl.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "nr/common/NRCommon.h"
#include "nr/stack/sdap/utils/QosHandler.h"
#include "nr/stack/mac/scheduling_modules/NRQoSModel.h"


//see inherit class for method description
class NRSchedulerGnbUL: public NRSchedulerGnbUl {
public:
    NRSchedulerGnbUL() : NRSchedulerGnbUl(){

    }

    virtual void removePendingRac(MacNodeId nodeId);

protected:

    bool fairSchedule;

    bool newTxbeforeRtx;

    bool useQosModel;

    bool combineQosWithRac;

    std::set<MacNodeId> schedulingNodeSet;

    virtual std::map<double, LteMacScheduleList>* schedule();
	virtual bool racschedule(double carrierFrequency);
	virtual bool rtxschedule(double carrierFrequency, BandLimitVector* bandLim = NULL);

	virtual void qosModelSchedule(double carrierFrequency);

	virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
	        std::vector<BandLimit>* bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);

    virtual void initialize(Direction dir, LteMacEnb* mac);

};
