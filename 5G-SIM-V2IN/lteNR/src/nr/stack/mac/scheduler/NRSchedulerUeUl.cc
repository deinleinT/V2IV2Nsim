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

NRSchedulerUeUl::NRSchedulerUeUl(LteMacUe *mac) :
		LteSchedulerUeUl(mac) {
	//std::cout << "NRSchedulerUeUl start at " << simTime().dbl() << std::endl;

	mac_ = mac;

	useQosModel = getSimulation()->getSystemModule()->par("useQosModel").boolValue();

	if (useQosModel) {
		lcgScheduler_ = new NRQoSModelScheduler(mac);
	} else {
		lcgScheduler_ = new NRLcgScheduler(mac);
	}

	//std::cout << "NRSchedulerUeUl end at " << simTime().dbl() << std::endl;
}

NRSchedulerUeUl::~NRSchedulerUeUl() {
	//delete lcgScheduler_;
}

LteMacScheduleListWithSizes* NRSchedulerUeUl::schedule() {
	//std::cout << "NRSchedulerUeUl schedule start at " << simTime().dbl() << std::endl;

	// 1) Environment Setup

	// clean up old scheduling decisions
	scheduleListWithSizes_.clear();

	// get the grant
	const LteSchedulingGrant *grant = mac_->getSchedulingGrant();
	UserControlInfo *userInfo = check_and_cast<UserControlInfo*>(grant->getControlInfo());

	Direction dir = grant->getDirection();

	// get the nodeId of the mac owner node
	MacNodeId nodeId = mac_->getMacNodeId();

	unsigned int codewords = grant->getCodewords();

	unsigned int availableBlocks = grant->getTotalGrantedBlocks();

	if (useQosModel) {
		for (Codeword cw = 0; cw < codewords; ++cw) {
			unsigned int availableBytes = grant->getGrantedCwBytes(cw);

			if (availableBytes <= 4)
				continue;

			// invoke the schedule() method of the attached LCP scheduler in order to schedule
			// the connections provided
			std::map<MacCid, std::pair<unsigned int, unsigned int>> &sdus = check_and_cast<NRQoSModelScheduler*>(lcgScheduler_)->schedule(idToMacCid(nodeId, userInfo->getLcid()), availableBytes, dir);

			if (sdus.empty())
				continue;

			std::map<MacCid, std::pair<unsigned int, unsigned int>>::const_iterator it = sdus.begin(), et = sdus.end();
			if (sdus.size() > 1) {
				for (auto &var : sdus) {
					if (var.second.second == 0)
						sdus.erase(var.first);
				}
				for (auto &var : sdus) {
					std::pair<MacCid, Codeword> schedulePair(var.first, cw);
					scheduleListWithSizes_[schedulePair].first = 1;
					scheduleListWithSizes_[schedulePair].second = var.second.second;
				}

			} else {
				for (; it != et; ++it) {
					// set schedule list entry
					std::pair<MacCid, Codeword> schedulePair(it->first, cw);
					scheduleListWithSizes_[schedulePair].first = 1;
					scheduleListWithSizes_[schedulePair].second = availableBytes;
				}
			}

		}
		//std::cout << "NRSchedulerUeUl schedule end at " << simTime().dbl() << std::endl;

		return &scheduleListWithSizes_;
	} else {

		for (Codeword cw = 0; cw < codewords; ++cw) {
			unsigned int availableBytes = grant->getGrantedCwBytes(cw);

			if (availableBytes <= 4)
				continue;

			// invoke the schedule() method of the attached LCP scheduler in order to schedule
			// the connections provided
			std::map<MacCid, std::pair<unsigned int, unsigned int>> &sdus = lcgScheduler_->schedule(availableBytes, dir);

			if (sdus.empty())
				continue;

			std::map<MacCid, std::pair<unsigned int, unsigned int>>::const_iterator it = sdus.begin(), et = sdus.end();
			if (sdus.size() > 1) {
				for (auto &var : sdus) {
					if (var.second.second == 0)
						sdus.erase(var.first);
				}
				for (auto &var : sdus) {
					std::pair<MacCid, Codeword> schedulePair(var.first, cw);
					scheduleListWithSizes_[schedulePair].first = 1;
					scheduleListWithSizes_[schedulePair].second = var.second.second;
				}

			} else {
				for (; it != et; ++it) {
					// set schedule list entry
					std::pair<MacCid, Codeword> schedulePair(it->first, cw);
					scheduleListWithSizes_[schedulePair].first = 1;
					scheduleListWithSizes_[schedulePair].second = availableBytes;
				}
			}
		}
		//std::cout << "NRSchedulerUeUl schedule end at " << simTime().dbl() << std::endl;

		return &scheduleListWithSizes_;
	}
}
