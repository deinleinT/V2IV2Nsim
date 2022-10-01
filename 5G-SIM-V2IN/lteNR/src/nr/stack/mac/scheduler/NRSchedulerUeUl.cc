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

#include "nr/stack/mac/scheduler/NRSchedulerUeUl.h"
#include "stack/mac/packet/LteSchedulingGrant.h"

NRSchedulerUeUl::NRSchedulerUeUl(LteMacUe *mac, double carrierFrequency) :
        LteSchedulerUeUl(mac, carrierFrequency)
{
    //std::cout << "NRSchedulerUeUl start at " << simTime().dbl() << std::endl;

    mac_ = mac;
    delete lcgScheduler_;
    useQosModel = getSimulation()->getSystemModule()->par("useQosModel").boolValue();

    if (useQosModel) {
        lcgScheduler_ = new NRQoSModelScheduler(mac);
    }
    else {
        lcgScheduler_ = new NRLcgScheduler(mac);
    }


    //std::cout << "NRSchedulerUeUl end at " << simTime().dbl() << std::endl;
}

NRSchedulerUeUl::~NRSchedulerUeUl()
{
}

LteMacScheduleList* NRSchedulerUeUl::schedule()
{
    //std::cout << "NRSchedulerUeUl schedule start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    //return LteSchedulerUeUl::schedule();

    // 1) Environment Setup
    // clean up old scheduling decisions
    scheduleList_.clear();
    scheduledBytesList_.clear();

    // get the grant
    const LteSchedulingGrant *grant = mac_->getSchedulingGrant(carrierFrequency_);

    if (grant == NULL)
        return &scheduleList_;

    Direction dir = grant->getDirection();

    // get the nodeId of the mac owner node
    MacNodeId nodeId = mac_->getMacNodeId();

    //! MCW support in UL
    unsigned int codewords = grant->getCodewords();

    // TODO get the amount of granted data per codeword
    unsigned int availableBlocks = grant->getTotalGrantedBlocks();

    // TODO check if HARQ ACK messages should be subtracted from available bytes

    if (useQosModel) {
        for (Codeword cw = 0; cw < codewords; ++cw) {
            unsigned int availableBytes = grant->getGrantedCwBytes(cw);

            // per codeword LCP scheduler invocation

            // invoke the schedule() method of the attached LCP scheduler in order to schedule
            // the connections provided
            std::map<MacCid, unsigned int> &sdus = check_and_cast<NRQoSModelScheduler*>(lcgScheduler_)->schedule(grant->getMacCid(),availableBytes, dir);

            // get the amount of bytes scheduled for each connection
            std::map<MacCid, unsigned int> &bytes = check_and_cast<NRQoSModelScheduler*>(lcgScheduler_)->getScheduledBytesList();

            // TODO check if this jump is ok
            if (sdus.empty())
                continue;

            std::map<MacCid, unsigned int>::const_iterator it = sdus.begin(), et = sdus.end();
            for (; it != et; ++it) {
                // set schedule list entry
                std::pair<MacCid, Codeword> schedulePair(it->first, cw);
                scheduleList_[schedulePair] = it->second;
            }

            std::map<MacCid, unsigned int>::const_iterator bit = bytes.begin(), bet = bytes.end();
            for (; bit != bet; ++bit) {
                // set schedule list entry
                std::pair<MacCid, Codeword> schedulePair(bit->first, cw);
                scheduledBytesList_[schedulePair] = bit->second;
            }

            MacCid highestBackloggedFlow = 0;
            MacCid highestBackloggedPriority = 0;
            MacCid lowestBackloggedFlow = 0;
            MacCid lowestBackloggedPriority = 0;
            bool backlog = false;

            // get the highest backlogged flow id and priority
            backlog = mac_->getHighestBackloggedFlow(highestBackloggedFlow, highestBackloggedPriority);

            if (backlog) // at least one backlogged flow exists
            {
                // get the lowest backlogged flow id and priority
                mac_->getLowestBackloggedFlow(lowestBackloggedFlow, lowestBackloggedPriority);
            }

        }
        return &scheduleList_;
    }
    else {

        for (Codeword cw = 0; cw < codewords; ++cw) {
            unsigned int availableBytes = grant->getGrantedCwBytes(cw);

            // per codeword LCP scheduler invocation

            // invoke the schedule() method of the attached LCP scheduler in order to schedule
            // the connections provided
            std::map<MacCid, unsigned int> &sdus = lcgScheduler_->schedule(availableBytes, dir);

            // get the amount of bytes scheduled for each connection
            std::map<MacCid, unsigned int> &bytes = lcgScheduler_->getScheduledBytesList();

            // TODO check if this jump is ok
            if (sdus.empty())
                continue;

            std::map<MacCid, unsigned int>::const_iterator it = sdus.begin(), et = sdus.end();
            for (; it != et; ++it) {
                // set schedule list entry
                std::pair<MacCid, Codeword> schedulePair(it->first, cw);
                scheduleList_[schedulePair] = it->second;
            }

            std::map<MacCid, unsigned int>::const_iterator bit = bytes.begin(), bet = bytes.end();
            for (; bit != bet; ++bit) {
                // set schedule list entry
                std::pair<MacCid, Codeword> schedulePair(bit->first, cw);
                scheduledBytesList_[schedulePair] = bit->second;
            }

            MacCid highestBackloggedFlow = 0;
            MacCid highestBackloggedPriority = 0;
            MacCid lowestBackloggedFlow = 0;
            MacCid lowestBackloggedPriority = 0;
            bool backlog = false;

            // get the highest backlogged flow id and priority
            backlog = mac_->getHighestBackloggedFlow(highestBackloggedFlow, highestBackloggedPriority);

            if (backlog) // at least one backlogged flow exists
            {
                // get the lowest backlogged flow id and priority
                mac_->getLowestBackloggedFlow(lowestBackloggedFlow, lowestBackloggedPriority);
            }

        }
        return &scheduleList_;
    }
}
