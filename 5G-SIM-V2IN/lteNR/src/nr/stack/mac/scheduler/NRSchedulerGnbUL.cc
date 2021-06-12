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

#include "nr/stack/mac/scheduler/NRSchedulerGnbUL.h"

std::map<double, LteMacScheduleList>* NRSchedulerGnbUL::schedule() {
	//std::cout << "NRSchedulerGnbUL::schedule start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return NRSchedulerGnbUl::schedule();

	//std::cout << "NRSchedulerGnbUL::schedule end at " << simTime().dbl() << std::endl;

}

bool NRSchedulerGnbUL::racschedule(double carrierFrequency) {
	//std::cout << "NRSchedulerGnbUL::racschedule start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return NRSchedulerGnbUl::racschedule(carrierFrequency);
}

/**
 * changed the behaviour
 * racschedule is called after checking whether a rtx is needed
 */
bool NRSchedulerGnbUL::rtxschedule(double carrierFrequency, BandLimitVector * bandLim) {

	//std::cout << "NRSchedulerGnbUL::rtxschedule start at " << simTime().dbl() << std::endl;

	//if true, a new tx is scheduled first before a retransmission
	if (getSimulation()->getSystemModule()->hasPar("newTxbeforeRtx")) {
		if (getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue()) {
			if (racschedule(carrierFrequency))
				return true;
		}
	}

	try {

		// retrieving reference to HARQ entities
		HarqRxBuffers *harqQueues = mac_->getHarqRxBuffers(carrierFrequency);
		if (harqQueues != NULL) {
			HarqRxBuffers::iterator it = harqQueues->begin();
			HarqRxBuffers::iterator et = harqQueues->end();

			for (; it != et; ++it) {
				// get current nodeId
				MacNodeId nodeId = it->first;

				if (nodeId == 0) {
					// UE has left the simulation - erase queue and continue
					harqRxBuffers_->at(carrierFrequency).erase(nodeId);
					continue;
				}
				OmnetId id = binder_->getOmnetId(nodeId);
				if (id == 0) {
					harqRxBuffers_->at(carrierFrequency).erase(nodeId);
					continue;
				}

				LteHarqBufferRx *currHarq = it->second;

				// Get user transmission parameters
				const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency); // get the user info
				// TODO SK Get the number of codewords - FIX with correct mapping
				unsigned int codewords = txParams.getLayers().size();                // get the number of available codewords

				// get the number of HARQ processes
				unsigned int process = 0;
				unsigned int maxProcesses = currHarq->getProcesses();

				for (process = 0; process < maxProcesses; ++process) {
					// for each HARQ process
					LteHarqProcessRx *currProc = currHarq->getProcess(process);

					if (allocatedCws_[nodeId] == codewords)
						break;

					unsigned int allocatedBytes = 0;
					for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
						if (allocatedCws_[nodeId] == codewords)
							break;

						// skip processes which are not in rtx status
						if (currProc->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {

							continue;
						}

						unsigned int rtxBytes = 0;

						rtxBytes = schedulePerAcidRtx(nodeId, carrierFrequency, cw, process, bandLim);
						if (rtxBytes > 0) {
							--codewords;
							allocatedBytes += rtxBytes;
						}
					}
				}
			}
		}

		int availableBlocks = allocator_->computeTotalRbs();

		bool retValue = (availableBlocks == 0);
		if (!getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue()) {
			retValue = racschedule(carrierFrequency);
		}

		return retValue;
	}
	catch (std::exception &e) {
		throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxschedule(): %s", e.what());
	}

}

unsigned int NRSchedulerGnbUL::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
		std::vector<BandLimit> * bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbUL::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return NRSchedulerGnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim, antenna, limitBl);
}
