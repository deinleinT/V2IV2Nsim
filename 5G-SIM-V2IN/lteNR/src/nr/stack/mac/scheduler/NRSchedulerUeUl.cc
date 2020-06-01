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

//    lcgScheduler_ = new LcgScheduler(mac);//TODO
	lcgScheduler_ = new NRLcgScheduler(mac);

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
	Direction dir = grant->getDirection();

	// get the nodeId of the mac owner node
	MacNodeId nodeId = mac_->getMacNodeId();

	//EV << NOW << " NRSchedulerUeUl::schedule - Scheduling node " << nodeId << endl;

	// retrieve Transmission parameters
//        const UserTxParams* txPar = grant->getUserTxParams();

//! MCW support in UL
	unsigned int codewords = grant->getCodewords();

	// TODO get the amount of granted data per codeword
	//unsigned int availableBytes = grant->getGrantedBytes();

	unsigned int availableBlocks = grant->getTotalGrantedBlocks();

	// TODO check if HARQ ACK messages should be subtracted from available bytes

	for (Codeword cw = 0; cw < codewords; ++cw) {
		unsigned int availableBytes = grant->getGrantedCwBytes(cw);

		if (availableBytes <= 4)
			continue;

		//EV << NOW << " NRSchedulerUeUl::schedule - Node " << mac_->getMacNodeId() << " available data from grant are " << " blocks " << availableBlocks << " [" << availableBytes << " - Bytes]  on codeword " << cw << endl;

		// per codeword LCP scheduler invocation

		// invoke the schedule() method of the attached LCP scheduler in order to schedule
		// the connections provided
		std::map<MacCid, std::pair<unsigned int, unsigned int>> &sdus = lcgScheduler_->schedule(availableBytes, dir);

		// TODO check if this jump is ok
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
				scheduleListWithSizes_[schedulePair].second = availableBytes;
			}

		} else {
			for (; it != et; ++it) {
				// set schedule list entry
				std::pair<MacCid, Codeword> schedulePair(it->first, cw);
				scheduleListWithSizes_[schedulePair].first = 1;
				scheduleListWithSizes_[schedulePair].second = availableBytes;
			}
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

		// TODO make use of above values
	}
	//std::cout << "NRSchedulerUeUl schedule end at " << simTime().dbl() << std::endl;

	return &scheduleListWithSizes_;
}
