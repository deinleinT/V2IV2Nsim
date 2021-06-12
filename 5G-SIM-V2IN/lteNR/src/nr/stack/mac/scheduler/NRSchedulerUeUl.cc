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

NRSchedulerUeUl::NRSchedulerUeUl(LteMacUe * mac, double carrierFrequency) : LteSchedulerUeUl(mac, carrierFrequency) {
	//std::cout << "NRSchedulerUeUl start at " << simTime().dbl() << std::endl;

	mac_ = mac;

	delete lcgScheduler_;
	lcgScheduler_ = new NRLcgScheduler(mac);

	//std::cout << "NRSchedulerUeUl end at " << simTime().dbl() << std::endl;
}

NRSchedulerUeUl::~NRSchedulerUeUl() {
}

LteMacScheduleList* NRSchedulerUeUl::schedule() {
	//std::cout << "NRSchedulerUeUl schedule start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return LteSchedulerUeUl::schedule();

}
