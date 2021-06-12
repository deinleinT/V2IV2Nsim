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

#include "common/LteCommon.h"
#include "nr/stack/mac/scheduler/NRSchedulerGnbDl.h"

NRSchedulerGnbDl::NRSchedulerGnbDl() {
}

NRSchedulerGnbDl::~NRSchedulerGnbDl() {
}

std::map<double, LteMacScheduleList>* NRSchedulerGnbDl::schedule() {
	//std::cout << "NRSchedulerGnbDl::schedule start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return LteSchedulerEnbDl::schedule();

}

bool NRSchedulerGnbDl::rtxschedule(double carrierFrequency, BandLimitVector * bandLim) {
	//std::cout << "NRSchedulerGnbDl::rtxschedule start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return LteSchedulerEnbDl::rtxschedule(carrierFrequency, bandLim);

}

unsigned int NRSchedulerGnbDl::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
		std::vector<BandLimit> * bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbDl::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return LteSchedulerEnbDl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim, antenna, limitBl);

}

unsigned int NRSchedulerGnbDl::scheduleGrant(MacCid cid, unsigned int bytes, bool & terminate, bool & active, bool & eligible,
		double carrierFrequency, BandLimitVector * bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbDl::scheduleGrant start at " << simTime().dbl() << std::endl;
	//TODO check if code has to be overwritten
	return LteSchedulerEnbDl::scheduleGrant(cid, bytes, terminate, active, eligible, carrierFrequency, bandLim, antenna, limitBl);

}
