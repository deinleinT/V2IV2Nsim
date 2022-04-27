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

#include "nr/stack/mac/scheduler/NRSchedulerGnbDl.h"

NRSchedulerGnbDl::NRSchedulerGnbDl() {

}

NRSchedulerGnbDl::~NRSchedulerGnbDl() {

}

LteMacScheduleListWithSizes* NRSchedulerGnbDl::schedule() {
	//std::cout << "NRSchedulerGnbDl::schedule start at " << simTime().dbl() << std::endl;

	//EV << "NRSchedulerGnbDl::schedule performed by Node: " << mac_->getMacNodeId() << endl;

	// clearing structures for new scheduling
	scheduleList_.clear();
	schedulingNodeSet.clear();
	allocatedCws_.clear(); //allocatedCwsNodeCid_.clear();

	// clean the allocator
	initAndResetAllocator();
	//reset AMC structures
	mac_->getAmc()->cleanAmcStructures(direction_, scheduler_->readActiveSet());

	// scheduling of retransmission and transmission
	//EV << "___________________________start RTX __________________________________" << endl;
	if (!(scheduler_->scheduleRetransmissions())) {
		//EV << "____________________________ end RTX __________________________________" << endl;
		//EV << "___________________________start SCHED ________________________________" << endl;
		scheduler_->updateSchedulingInfo();
		scheduler_->schedule();

		//EV << "____________________________ end SCHED ________________________________" << endl;
	}

	// record assigned resource blocks statistics
	resourceBlockStatistics();

	//std::cout << "NRSchedulerGnbDl::schedule end at " << simTime().dbl() << std::endl;

	return &scheduleList_;
}

void NRSchedulerGnbDl::qosModelSchedule() {

	//std::cout << "NRSchedulerGnbDl::qosModelSchedule start at " << simTime().dbl() << std::endl;

	//use code from rtxschedule to find out the harqprocesses which need a rtx
	// retrieving reference to HARQ entities
	HarqTxBuffers *harqQueues = mac_->getHarqTxBuffers();
	HarqTxBuffers::const_iterator it = harqQueues->begin();
	HarqTxBuffers::const_iterator et = harqQueues->end();

	//retrieve the lambdaValues from the ini file
	double lambdaPriority = getSimulation()->getSystemModule()->par("lambdaPriority").doubleValue();
	double lambdaRemainDelayBudget = getSimulation()->getSystemModule()->par("lambdaRemainDelayBudget").doubleValue();
	double lambdaCqi = getSimulation()->getSystemModule()->par("lambdaCqi").doubleValue();
	double lambdaRtx = getSimulation()->getSystemModule()->par("lambdaRtx").doubleValue();
	double lambdaByteSize = getSimulation()->getSystemModule()->par("lambdaByteSize").doubleValue();	//consider also the size of one packet
	//

	std::map<double, std::vector<ScheduleInfo>> combinedMap;
	//std::vector<MacNodeId> neglectUe;

	//PART 1: gather all relevant information about the harqs
	for (; it != et; ++it) {
		// For each UE
		MacNodeId nodeId = it->first;

		OmnetId id = binder_->getOmnetId(nodeId);
		if (id == 0) {
			// UE has left the simulation, erase HARQ-queue
			it = harqQueues->erase(it);
			if (it == et)
				break;
			else
				continue;
		}
		LteHarqBufferTx *currHarq = it->second;
		std::vector<LteHarqProcessTx*> *processes = currHarq->getHarqProcesses();

		// Get user transmission parameters
		const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info
		unsigned int codewords = txParams.getLayers().size();        // get the number of available codewords
		unsigned int process = 0;
		unsigned int maxProcesses = currHarq->getNumProcesses();
		//stores all harq processes with its priority and still available delay budget

		//bool breakFlag = false; //used for break if a process for rtx was found

		//
//		for (process = 0; process < maxProcesses; ++process) {
//			// for each HARQ process
//			LteHarqProcessTx *currProc = (*processes)[process];
//
//			if (currProc->getUnitStatus(0) == TXHARQ_PDU_WAITING) {
//				neglectUe.push_back(nodeId);
//				breakFlag = true;
//				break;
//			}
//		}
//		if(breakFlag)
//			continue;
		//

		for (process = 0; process < maxProcesses; ++process) {
			// for each HARQ process
			LteHarqProcessTx *currProc = (*processes)[process];

			if (allocatedCws_[nodeId] == codewords)
				break;
			for (Codeword cw = 0; cw < codewords; ++cw) {
				if (allocatedCws_[nodeId] == codewords)
					break;

				// skip processes which are not in rtx status
				if (currProc->getUnitStatus(cw) == TXHARQ_PDU_BUFFERED) {
					LteMacPdu *pdu = currProc->getPdu(cw);
					UserControlInfo *lteInfo = check_and_cast<UserControlInfo*>(pdu->getControlInfo());
					MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());
					unsigned short qfi = lteInfo->getQfi();
					unsigned short _5qi = mac_->getQosHandler()->get5Qi(qfi);
					double prio = mac_->getQosHandler()->getPriority(_5qi);
					double pdb = mac_->getQosHandler()->getPdb(_5qi);
					ASSERT(pdu->getCreationTime() != 0);
					simtime_t delay = NOW - pdu->getCreationTime();
					simtime_t remainDelayBudget = SimTime(pdb) - delay;

					//create a scheduleInfo
					ScheduleInfo tmp;
					tmp.nodeId = nodeId;
					tmp.remainDelayBudget = remainDelayBudget;
					tmp.category = "rtx";
					tmp.cid = cid;
					tmp.codeword = cw;
					tmp.harqProcessTx = currProc;
					tmp.pdb = pdb;
					tmp.priority = prio;
					tmp.numberRtx = currProc->getTransmissions(cw);
					tmp.nodeId = nodeId;
					tmp.process = process;
					tmp.sizeOnePacketDL = currProc->getPdu(cw)->getByteLength();

					double calcPrio = lambdaPriority * (prio / 90.0) + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget) + lambdaCqi * (1.0 / (txParams.readCqiVector().at(0) + 1.0))
							+ (lambdaRtx * (1.0 / (1.0 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketDL) * lambdaByteSize);

					combinedMap[calcPrio].push_back(tmp);
					//breakFlag = true;
				} else {
					continue;
				}
//				if (breakFlag) {
//					break;
//				}
			}
//			if (breakFlag) {
//				break;
//			}
		}
	}

	//Look into the macBuffers and find out all active connections
	for (auto &var : *mac_->getMacBuffers()) {
		if (!var.second->isEmpty()) {
			backlog(var.first);
		}
	}

	//here we consider the active connections which want to send new data
	//use code from QosModel to find out most prio cid
	//get the activeSet of connections
	ActiveSet activeConnectionTempSet_ = scheduler_->getActiveConnectionSet();

	//find out which cid has highest priority via qosHandler
	std::map<double, std::vector<QosInfo>> sortedCids = mac_->getQosHandler()->getEqualPriorityMap(DL);

	//create a map with cids sorted by priority
	std::map<double, std::vector<QosInfo>> activeCids;
	for (auto &var : sortedCids) {
		for (auto &qosinfo : var.second) {
			for (auto &cid : activeConnectionTempSet_) {

				MacNodeId nodeId = MacCidToNodeId(cid);

				//
//				bool conFlag = false;
//				for(auto & ueId : neglectUe){
//					if(ueId == nodeId){
//						conFlag = true;
//						break;
//					}
//				}
//				if(conFlag){
//					continue;
//				}
				//

				OmnetId id = binder_->getOmnetId(nodeId);

				if (nodeId == 0 || id == 0) {
					// node has left the simulation - erase corresponding CIDs
					//activeConnectionTempSet_.erase(cid);
					continue;
				}

				// check if node is still a valid node in the simulation - might have been dynamically removed
				if (getBinder()->getOmnetId(nodeId) == 0) {
					//activeConnectionTempSet_.erase(cid);
					continue;
				}

				MacCid realCidQosInfo;
				realCidQosInfo = idToMacCid(qosinfo.destNodeId, qosinfo.lcid);
				if (realCidQosInfo == cid) {
					//we have the QosInfos
					activeCids[var.first].push_back(qosinfo);
				}
			}
		}
	}

	for (auto &var : activeCids) {

		//loop over all qosinfos with same prio

		for (auto &qosinfos : var.second) {
			MacCid cid = idToMacCid(qosinfos.destNodeId, qosinfos.lcid);
			MacNodeId nodeId = MacCidToNodeId(cid);

			//access the macbuffer to retrieve the creationTime of the pdu
			if (mac_->getMacBuffers()->find(cid) != mac_->getMacBuffers()->end()) {
				LteMacBuffer *macBuffer = mac_->getMacBuffers()->at(cid);

				if (macBuffer->getQueueLength() <= 0) {
					continue;
				}

				const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);

				//this is the creationTime of the first packet in the buffer
				simtime_t creationTimeFirstPacket = macBuffer->getHolTimestamp();
				ASSERT(creationTimeFirstPacket != 0);
				//delay of this packet
				simtime_t delay = NOW - creationTimeFirstPacket;

				//get the pdb and prio of this cid
				unsigned short qfi = qosinfos.qfi;
				unsigned short _5qi = mac_->getQosHandler()->get5Qi(qfi);
				double prio = mac_->getQosHandler()->getPriority(_5qi);
				double pdb = mac_->getQosHandler()->getPdb(_5qi);

				simtime_t remainDelayBudget = SimTime(pdb) - delay;

				ScheduleInfo tmp;
				tmp.nodeId = nodeId;
				tmp.remainDelayBudget = remainDelayBudget;
				tmp.category = "newTx";
				tmp.cid = cid;
				tmp.pdb = pdb;
				tmp.priority = prio;
				tmp.numberRtx = 0;
				tmp.nodeId = qosinfos.destNodeId;

				tmp.bytesizeDL = macBuffer->getQueueOccupancy();
				tmp.sizeOnePacketDL = macBuffer->front().first;

				if (!getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
					tmp.sizeOnePacketDL = tmp.bytesizeDL;
				}

				double calcPrio = lambdaPriority * (prio / 90.0) + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget) + lambdaCqi * (1 / (txParams.readCqiVector().at(0) + 1))
						+ (lambdaRtx * (1 / (1 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketDL) * lambdaByteSize);

				combinedMap[calcPrio].push_back(tmp);
			}
		}
	}

	//go through the combinedMap and find out the most prior rtx or newTx
	//is sorted by calculated priority

	std::map<MacNodeId, std::vector<ScheduledInfo>> scheduledInfoMap;

	//considering the available resources
	if (getSimulation()->getSystemModule()->par("combineQosWithRac").boolValue()) {

		//how many resources available for all?
		//how many resources needed per priority?
		//ensure that highest priority is fully satisfied
		//go to next prio
		//key --> calculated weight
		for (auto &prio : combinedMap) {

			int countedRtx = 0;

			//schedule only the first rtx (all have same prio)
			for (auto &schedInfo : prio.second) {

				//only the connection with the highest Prio of the same UE gets resources
				if (scheduledInfoMap[schedInfo.nodeId].size() == 1) {
					continue;
				}

				if (schedInfo.category == "rtx") {
					unsigned int rtxBytes = schedulePerAcidRtx(schedInfo.nodeId, schedInfo.codeword, schedInfo.process);

					ScheduledInfo tmp;
					tmp.nodeId = schedInfo.nodeId;
					tmp.info = schedInfo;
					scheduledInfoMap[schedInfo.nodeId].push_back(tmp);
					countedRtx++;
				}

			}

			int blocks = allocator_->computeTotalRbs();
			//values --> vector with ScheduleInfo with the same weight, mix of rtx and newTx
			int numberOfSchedInfosWithSamePrio = prio.second.size();
			int newTxWithEqualPrio = numberOfSchedInfosWithSamePrio - countedRtx;

			if (newTxWithEqualPrio <= 0) {
				continue;
			}

			//share the available blocks in a fair way
			int blocksPerSchedInfo = blocks / newTxWithEqualPrio;
//			if(blocks != blocksPerSchedInfo){
//				std::cout << std::endl;
//			}

			//than schedule newRtx
			for (auto &schedInfo : prio.second) {

				//only the connection with the highest Prio of the same UE gets resources
				if (scheduledInfoMap[schedInfo.nodeId].size() == 1) {
					continue;
				}

				if (schedInfo.category == "newTx") {

					MacCid cid = schedInfo.cid;
					MacNodeId nodeId = schedInfo.nodeId;
					unsigned int numBands = mac_->getCellInfo()->getNumBands();
					const unsigned int cw = 0;
					bool allocation = false;
					//blocks = allocator_->computeTotalRbs();
					unsigned int bytesize = schedInfo.bytesizeDL;
					unsigned int sizePerPacket = schedInfo.sizeOnePacketDL;
					unsigned int reqBlocks = 0;
					unsigned int bytes = 0;
					int schedBlocks = 0;
					int schedBytesPerSchedInfo = 0;

					for (Band b = 0; b < mac_->getCellInfo()->getNumBands(); ++b) {

						if (blocks > 0) {

							//real required blocks
							if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
								reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, sizePerPacket, DL, blocks); //required Blocks --> for Packet
							} else {
								reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, DL, blocks); //required Blocks --> for Queue
							}

							//real required bytes
							bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, reqBlocks, DL); //required Bytes
							//shared available blocks
							schedBytesPerSchedInfo = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, blocksPerSchedInfo, DL); //required Bytes
							if(newTxWithEqualPrio == 1){
								schedBytesPerSchedInfo = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, allocator_->availableBlocks(nodeId, MACRO, b), UL);
							}

							// Grant data to that connection.
							bool terminate = false;
							bool active = true;
							bool eligible = true;

							//required blocks fit the size of the shared blocks
							if (reqBlocks <= blocksPerSchedInfo) {

								unsigned int granted = scheduleGrant(schedInfo.cid, bytes, terminate, active, eligible);

							} else {
								//required blocks larger, schedule only the shared blocks
								unsigned int granted = scheduleGrant(schedInfo.cid, schedBytesPerSchedInfo, terminate, active, eligible);
							}

							// Exit immediately if the terminate flag is set.
							if (terminate) {
								break;
							}

							if (!active) {
								activeConnectionTempSet_.erase(schedInfo.cid);
							}

						}
					}

					ScheduledInfo tmp;
					tmp.nodeId = schedInfo.nodeId;
					tmp.info = schedInfo;
					scheduledInfoMap[schedInfo.nodeId].push_back(tmp);
					newTxWithEqualPrio--;
				}

			}
		}

	} else {
		//key --> calculated prio
		for (auto &var : combinedMap) {

			//values --> vector with ScheduleInfo with the same weight
			for (auto &vec : var.second) {

				//only the connection with the highest Prio of the same UE get resources
				if (scheduledInfoMap[vec.nodeId].size() == 1) {
					ASSERT(vec.nodeId == scheduledInfoMap[vec.nodeId].at(0).nodeId);
					continue;
				}

				if (vec.category == "rtx") {

					unsigned int bytes = schedulePerAcidRtx(vec.nodeId, vec.codeword, vec.process);

				} else if (vec.category == "newTx") {

					// compute available blocks for the current user
					const UserTxParams &info = mac_->getAmc()->computeTxParams(vec.nodeId, DL);
					const std::set<Band> &bands = info.readBands();
					unsigned int codeword = info.getLayers().size();
					if (allocatedCws(vec.nodeId) == codeword)
						continue;
					std::set<Band>::const_iterator it = bands.begin(), et = bands.end();

					std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt = info.readAntennaSet().end();

					bool cqiNull = false;
					for (unsigned int i = 0; i < codeword; i++) {
						if (info.readCqiVector()[i] == 0)
							cqiNull = true;
					}
					if (cqiNull)
						continue;
					// compute score based on total available bytes
					unsigned int availableBlocks = 0;
					unsigned int availableBytes = 0;
					// for each antenna
					for (; antennaIt != antennaEt; ++antennaIt) {
						// for each logical band
						for (; it != et; ++it) {
							availableBlocks += readAvailableRbs(vec.nodeId, *antennaIt, *it);
							availableBytes += mac_->getAmc()->computeBytesOnNRbs(vec.nodeId, *it, availableBlocks, direction_);
						}
					}

					// Grant data to that connection.
					bool terminate = false;
					bool active = true;
					bool eligible = true;

					unsigned int bytesize = vec.bytesizeDL;
					unsigned int sizePerPacket = vec.sizeOnePacketDL;

					unsigned int granted = 0;
					if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
						granted = scheduleGrant(vec.cid, sizePerPacket, terminate, active, eligible); // for Packet
					} else {
						granted = scheduleGrant(vec.cid, bytesize, terminate, active, eligible); // for queue
					}

					// Exit immediately if the terminate flag is set.
					if (terminate) {
						break;
					}

					if (!active) {
						activeConnectionTempSet_.erase(vec.cid);
					}
				} else {
					throw cRuntimeError("Error");
				}

				ScheduledInfo stmp;
				stmp.nodeId = vec.nodeId;
				stmp.info = vec;
				scheduledInfoMap[vec.nodeId].push_back(stmp);
			}
		}
	}

	//clean up
	scheduler_->getActiveConnectionSet() = activeConnectionTempSet_;
	combinedMap.clear();

	//std::cout << "NRSchedulerGnbDl::qosModelSchedule start at " << simTime().dbl() << std::endl;
}

bool NRSchedulerGnbDl::rtxschedule() {
	//std::cout << "NRSchedulerGnbDl::rtxschedule start at " << simTime().dbl() << std::endl;

	//
	if (getSimulation()->getSystemModule()->hasPar("useQosModel")) {
		if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
			qosModelSchedule();
			//return always true to avoid calling other scheduling functions
			return true;
		}
	}
	//

	// retrieving reference to HARQ entities
	HarqTxBuffers *harqQueues = mac_->getHarqTxBuffers();

	HarqTxBuffers::const_iterator it = harqQueues->begin();
	HarqTxBuffers::const_iterator et = harqQueues->end();

	std::vector<BandLimit> usableBands;

	// examination of HARQ process in rtx status, adding them to scheduling list
	for (; it != et; ++it) {
		// For each UE
		MacNodeId nodeId = it->first;

		if (schedulingNodeSet.find(nodeId) != schedulingNodeSet.end()) {
			//already one cid scheduled for this node
			continue;
		}

		OmnetId id = binder_->getOmnetId(nodeId);
		if (id == 0) {
			// UE has left the simulation, erase HARQ-queue
			it = harqQueues->erase(it);
			if (it == et)
				break;
			else
				continue;
		}
		LteHarqBufferTx *currHarq = it->second;
		std::vector<LteHarqProcessTx*> *processes = currHarq->getHarqProcesses();

		// Get user transmission parameters
		const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info

		unsigned int codewords = txParams.getLayers().size();        // get the number of available codewords

		//EV << NOW << " NRSchedulerGnbDl::rtxschedule  UE: " << nodeId << endl;
		//EV << NOW << " NRSchedulerGnbDl::rtxschedule Number of codewords: " << codewords << endl;
		unsigned int process = 0;
		unsigned int maxProcesses = currHarq->getNumProcesses();
		std::map<simtime_t, unsigned int> rtxProcesses;

		//find longest waiting process
		for (process = 0; process < maxProcesses; ++process) {
			// for each HARQ process
			LteHarqProcessTx *currProc = (*processes)[process];

			if (allocatedCws_[nodeId] == codewords)
				break;
			for (Codeword cw = 0; cw < codewords; ++cw) {
				if (allocatedCws_[nodeId] == codewords)
					break;

				// skip processes which are not in rtx status
				if (currProc->getUnitStatus(cw) == TXHARQ_PDU_BUFFERED) {
					rtxProcesses[currProc->getOldestUnitTxTime()] = process;
				} else {
					continue;
				}
			}
		}

		//		for (process = 0; process < maxProcesses; ++process) {
		for (auto &process : rtxProcesses) {
			// for each HARQ process
			LteHarqProcessTx *currProc = (*processes)[process.second];

			if (allocatedCws_[nodeId] == codewords)
				break;
			for (Codeword cw = 0; cw < codewords; ++cw) {
				if (allocatedCws_[nodeId] == codewords)
					break;
				//EV << NOW << " NRSchedulerGnbDl::rtxschedule process " << process << endl;
				//EV << NOW << " NRSchedulerGnbDl::rtxschedule ------- CODEWORD " << cw << endl;

				// skip processes which are not in rtx status
				if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED) {
					//EV << NOW << " NRSchedulerGnbDl::rtxschedule detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
					continue;
				}

				//EV << NOW << " NRSchedulerGnbDl::rtxschedule " << endl;
				//EV << NOW << " NRSchedulerGnbDl::rtxschedule detected RTX Acid: " << process << endl;

				// Get the bandLimit for the current user
				std::vector<BandLimit> *bandLim;
				bool ret = getBandLimit(&usableBands, nodeId);
				if (!ret)
					bandLim = NULL;
				else
					bandLim = &usableBands;

				// perform the retransmission
				unsigned int bytes = schedulePerAcidRtx(nodeId, cw, process.second, bandLim);

				schedulingNodeSet.insert(nodeId);

				// if a value different from zero is returned, there was a service
				if (bytes > 0) {
					//EV << NOW << " NRSchedulerGnbDl::rtxschedule CODEWORD IS NOW BUSY!!!" << endl;
					// do not process this HARQ process anymore
					// go to next codeword
					break;
				}
			}
		}
	}

	unsigned int availableBlocks = allocator_->computeTotalRbs();

	//std::cout << "NRSchedulerGnbDl::rtxschedule end at " << simTime().dbl() << std::endl;

	return (availableBlocks == 0);
}

unsigned int NRSchedulerGnbDl::schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbDl::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	// Get user transmission parameters
	const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);    // get the user info

	unsigned int codewords = txParams.getLayers().size();                // get the number of available codewords

	std::string bands_msg = "BAND_LIMIT_SPECIFIED";

	std::vector<BandLimit> tempBandLim;

	Codeword remappedCw = (codewords == 1) ? 0 : cw;

	if (bandLim == NULL) {
		bands_msg = "NO_BAND_SPECIFIED";
		// Create a vector of band limit using all bands
		bandLim = &tempBandLim;

		unsigned int numBands = mac_->getCellInfo()->getNumBands();
		// for each band of the band vector provided
		for (unsigned int i = 0; i < numBands; i++) {
			BandLimit elem;
			// copy the band
			elem.band_ = Band(i);
			//EV << "Putting band " << i << endl;
			// mark as unlimited
			for (Codeword i = 0; i < MAX_CODEWORDS; ++i) {
				elem.limit_.push_back(-1);
			}
			bandLim->push_back(elem);
		}
	}
	//EV << NOW << "NRSchedulerGnbDl::rtxAcid - Node [" << mac_->getMacNodeId() << "], User[" << nodeId << "],  Codeword [" << cw << "]  of [" << codewords << "] , ACID [" << (int)acid << "] " << endl;
	//! \test REALISTIC!!!  Multi User MIMO support
	if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER)) {
		// request amc for MU_MIMO pairing
		MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId);
		if (peer != nodeId) {
			// this user has a valid pairing
			//1) register pairing  - if pairing is already registered false is returned
			if (allocator_->configureMuMimoPeering(nodeId, peer)) {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
			} else {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
			}
		} else {
			//EV << "NRSchedulerGnbDl::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
		}
	}
	//!\test experimental DAS support
	// registering DAS spaces to the allocator
	Plane plane = allocator_->getOFDMPlane(nodeId);
	allocator_->setRemoteAntenna(plane, antenna);

	// blocks to allocate for each band
	std::vector<unsigned int> assignedBlocks;
	// bytes which blocks from the preceding vector are supposed to satisfy
	std::vector<unsigned int> assignedBytes;
	LteHarqBufferTx *currHarq = mac_->getHarqTxBuffers()->at(nodeId);

	// bytes to serve
	unsigned int bytes = currHarq->pduLength(acid, cw);

	// check selected process status.
	std::vector<UnitStatus> pStatus = currHarq->getProcess(acid)->getProcessStatus();
	std::vector<UnitStatus>::iterator vit = pStatus.begin(), vet = pStatus.end();

	Codeword allocatedCw = 0;
	// search for already allocated codeword

	if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
		allocatedCw = allocatedCws_.at(nodeId);

	}
	// for each band
	unsigned int size = bandLim->size();

	for (unsigned int i = 0; i < size; ++i) {
		// save the band and the relative limit
		Band b = bandLim->at(i).band_;
		int limit = bandLim->at(i).limit_.at(remappedCw);

		//EV << "NRSchedulerGnbDl::schedulePerAcidRtx --- BAND " << b << " LIMIT " << limit << "---" << endl;
		// if the limit flag is set to skip, jump off
		if (limit == -2) {
			//EV << "LteSchedulerEnbDl::schedulePerAcidRtx - skipping logical band according to limit value" << endl;
			continue;
		}

		unsigned int available = 0;
		// if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
		if ((allocatedCw != 0)) {
			// get band allocated blocks
			int b1 = allocator_->getBlocks(antenna, b, nodeId);

			// limit eventually allocated blocks on other codeword to limit for current cw
			//b1 = (limitBl ? (b1>limit?limit:b1) : b1);
			available = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, remappedCw, b1, direction_);
		} else
			available = availableBytes(nodeId, antenna, b, remappedCw, direction_, (limitBl) ? limit : -1);    // available space

		// use the provided limit as cap for available bytes, if it is not set to unlimited
		if (limit >= 0 && !limitBl)
			available = limit < (int) available ? limit : available;

		//EV << NOW << "NRSchedulerGnbDl::rtxAcid ----- BAND " << b << "-----" << endl;
		//EV << NOW << "NRSchedulerGnbDl::rtxAcid To serve: " << bytes << " bytes" << endl;
		//EV << NOW << "NRSchedulerGnbDl::rtxAcid Available: " << available << " bytes" << endl;

		unsigned int allocation = 0;
		if (available < bytes) {
			allocation = available;
			bytes -= available;
		} else {
			allocation = bytes;
			bytes = 0;
		}

		if ((allocatedCw == 0)) {
			//unsigned int blocks = mac_->getAmc()->computeReqRbs(nodeId, b, remappedCw, allocation, direction_, allocator_->getBlocks(antenna, b, nodeId));
			unsigned int blocks = mac_->getAmc()->computeReqRbs(nodeId, b, remappedCw, allocation, direction_, allocator_->availableBlocks(nodeId, antenna, b));

			//EV << NOW << "NRSchedulerGnbDl::rtxAcid Assigned blocks: " << blocks << "  blocks" << endl;

			// assign only on the first codeword
			assignedBlocks.push_back(blocks);
			assignedBytes.push_back(allocation);
		}

		if (bytes == 0)
			break;
	}

	if (bytes > 0) {
		// process couldn't be served
		//EV << NOW << "NRSchedulerGnbDl::rtxAcid Cannot serve HARQ Process" << acid << endl;
		return 0;
	}

	// record the allocation if performed
	size = assignedBlocks.size();
	// For each LB with assigned blocks
	for (unsigned int i = 0; i < size; ++i) {
		if (allocatedCw == 0) {
			// allocate the blocks
			allocator_->addBlocks(antenna, bandLim->at(i).band_, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
		}
		// store the amount
		bandLim->at(i).limit_.at(remappedCw) = assignedBytes.at(i);
	}

	UnitList signal;
	signal.first = acid;
	signal.second.push_back(cw);

	//EV << NOW << " NRSchedulerGnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << " marking for retransmission " << endl;

	// if allocated codewords is not MAX_CODEWORDS, then there's another allocated codeword , update the codewords variable :

	if (allocatedCw != 0) {

		--codewords;
		if (codewords <= 0)
			throw cRuntimeError("LteSchedulerEnbDl::rtxAcid(): erroneus codeword count %d", codewords);
	}

	// signal a retransmission
	currHarq->markSelected(signal, codewords);

	// mark codeword as used
	if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
		allocatedCws_.at(nodeId)++;}
	else {
		allocatedCws_[nodeId] = 1;
	}

	bytes = currHarq->pduLength(acid, cw);

	//EV << NOW << " NRSchedulerGnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << ", " << bytes << " bytes served!" << endl;

	//std::cout << "NRSchedulerGnbDl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

	return bytes;
}

unsigned int NRSchedulerGnbDl::scheduleGrant(MacCid cid, unsigned int bytes, bool &terminate, bool &active, bool &eligible, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbDl::scheduleGrant start at " << simTime().dbl() << std::endl;

	// Get the node ID and logical connection ID
	MacNodeId nodeId = MacCidToNodeId(cid);

	if (schedulingNodeSet.find(nodeId) != schedulingNodeSet.end()) {
		//already one cid scheduled for this node
		active = false;
		return 0;
	}

	LogicalCid flowId = MacCidToLcid(cid);
	// Get user transmission parameters
	const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);
	//get the number of codewords
	unsigned int numCodewords = txParams.getLayers().size();

	std::string bands_msg = "BAND_LIMIT_SPECIFIED";
	std::vector<BandLimit> tempBandLim;
	if (bandLim == NULL) {
		bands_msg = "NO_BAND_SPECIFIED";

		txParams.print("grant()");
		// Create a vector of band limit using all bands
		if (emptyBandLim_.empty()) {
			unsigned int numBands = mac_->getCellInfo()->getNumBands();
			// for each band of the band vector provided
			for (unsigned int i = 0; i < numBands; i++) {
				BandLimit elem;
				// copy the band
				elem.band_ = Band(i);
				EV << "Putting band " << i << endl;
				// mark as unlimited
				for (unsigned int j = 0; j < numCodewords; j++) {
					//EV << "- Codeword " << j << endl;
					elem.limit_.push_back(-1);
				}
				emptyBandLim_.push_back(elem);
			}
		}
		tempBandLim = emptyBandLim_;
		bandLim = &tempBandLim;
	}
	//EV << "NRSchedulerGnbDl::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

	unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
	unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)

	// === Perform normal operation for grant === //

	//! Multiuser MIMO support
	if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER)) {
		// request AMC for MU_MIMO pairing
		MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, direction_);
		if (peer != nodeId) {
			// this user has a valid pairing
			//1) register pairing  - if pairing is already registered false is returned
			if (allocator_->configureMuMimoPeering(nodeId, peer)) {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
			} else {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
			}
		} else {
			//EV << "NRSchedulerGnbDl::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
		}
	}

	// registering DAS spaces to the allocator
	Plane plane = allocator_->getOFDMPlane(nodeId);
	allocator_->setRemoteAntenna(plane, antenna);

	// search for already allocated codeword
	unsigned int cwAlreadyAllocated = 0;
	if (allocatedCws_.find(nodeId) != allocatedCws_.end())
		cwAlreadyAllocated = allocatedCws_.at(nodeId);

	// Check OFDM space
	// OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
	// if we are tryang to allocate a peer user in mu_mimo plane
	if (allocator_->computeTotalRbs() == 0
			&& (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING && txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlreadyAllocated == 0)
					&& (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE))) {
		terminate = true; // ODFM space ended, issuing terminate flag
		//EV << "NRSchedulerGnbDl::grant Space ended, no schedulation." << endl;
		return 0;
	}

	// TODO This is just a BAD patch
	// check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
	// otherwise check why an UE is stopped being scheduled while its buffer is not empty
	if (cwAlreadyAllocated > 0) {
		terminate = true;
		return 0;
	}

	// ===== DEBUG OUTPUT ===== //
	//	bool debug = false; // TODO: make this configurable
	//	if (debug) {
	//		if (limitBl)
	//			EV << "LteSchedulerEnb::grant blocks: " << bytes << endl;
	//		else
	//			EV << "LteSchedulerEnb::grant Bytes: " << bytes << endl;
	//		EV << "LteSchedulerEnb::grant Bands: {";
	//		unsigned int size = (*bandLim).size();
	//		if (size > 0) {
	//			EV << (*bandLim).at(0).band_;
	//			for (unsigned int i = 1; i < size; i++)
	//				EV << ", " << (*bandLim).at(i).band_;
	//		}
	//		EV << "}\n";
	//	}
	// ===== END DEBUG OUTPUT ===== //

	// Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
	Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function
	if (!checkEligibility(nodeId, cw) || cw >= numCodewords) {
		eligible = false;

		return totalAllocatedBytes; // return the total number of served bytes
	}

	// Get virtual buffer reference
	LteMacBuffer *conn = ((direction_ == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

	// get the buffer size
	unsigned int queueLength = conn->getQueueOccupancy(); // in bytes
	if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
		queueLength = (conn->getQueueOccupancy() == 0) ? 0 : conn->getQueueOccupancy() / conn->getQueueLength(); // in bytes
	}

	if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
		if (getSimulation()->getSystemModule()->par("combineQosWithRac").boolValue()) {
			queueLength = bytes;
		}
	}

	if (queueLength == 0) {
		active = false;

		return totalAllocatedBytes;
	}

	bool stop = false;
	unsigned int toServe = 0;
	for (; cw < numCodewords; ++cw) {

		queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
		toServe = queueLength;

		unsigned int cwAllocatedBytes = 0;  // per codeword allocated bytes
		unsigned int cwAllocatedBlocks = 0; // used by uplink only, for signaling cw blocks usage to schedule list
		unsigned int vQueueItemCounter = 0; // per codeword MAC SDUs counter

		unsigned int allocatedCws = 0;
		unsigned int size = (*bandLim).size();
		for (unsigned int i = 0; i < size; ++i) // for each band
				{
			// save the band and the relative limit
			Band b = (*bandLim).at(i).band_;
			int limit = (*bandLim).at(i).limit_.at(cw);

			// if the limit flag is set to skip, jump off
			if (limit == -2) {
				continue;
			}

			// search for already allocated codeword
			if (allocatedCws_.find(nodeId) != allocatedCws_.end())
				allocatedCws = allocatedCws_.at(nodeId);

			unsigned int bandAvailableBytes = 0;
			unsigned int bandAvailableBlocks = 0;
			// if there is a previous blocks allocation on the first codeword, blocks allocation is already available
			if (allocatedCws != 0) {
				// get band allocated blocks
				int b1 = allocator_->getBlocks(antenna, b, nodeId);
				// limit eventually allocated blocks on other codeword to limit for current cw
				bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);
				bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, direction_);
			} else // if limit is expressed in blocks, limit value must be passed to availableBytes function
			{
				bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_, (limitBl) ? limit : -1); // available space (in bytes)
				bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
			}

			// if no allocation can be performed, notify to skip the band on next processing (if any)
			if (bandAvailableBytes == 0) {
				//EV << "NRSchedulerGnbDl::grant Band " << b << "will be skipped since it has no space left." << endl;
				(*bandLim).at(i).limit_.at(cw) = -2;
				continue;
			}
			//if bandLimit is expressed in bytes
			if (!limitBl) {
				// use the provided limit as cap for available bytes, if it is not set to unlimited
				if (limit >= 0 && limit < (int) bandAvailableBytes) {
					bandAvailableBytes = limit;

				}
			} else {
				// if bandLimit is expressed in blocks
				if (limit >= 0 && limit < (int) bandAvailableBlocks) {
					bandAvailableBlocks = limit;

				}
			}

			unsigned int uBytes = (bandAvailableBytes > queueLength) ? queueLength : bandAvailableBytes;
			unsigned int uBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, uBytes, direction_, bandAvailableBlocks);

			// allocate resources on this band
			if (allocatedCws == 0) {
				// mark here allocation
				allocator_->addBlocks(antenna, b, nodeId, uBlocks, uBytes);
				// add allocated blocks for this codeword
				cwAllocatedBlocks += uBlocks;
				totalAllocatedBlocks += uBlocks;
				cwAllocatedBytes += uBytes;
			}

			// update limit
			if (uBlocks > 0 && (*bandLim).at(i).limit_.at(cw) > 0) {
				(*bandLim).at(i).limit_.at(cw) -= uBlocks;
				if ((*bandLim).at(i).limit_.at(cw) < 0)
					throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ", b, (*bandLim).at(i).limit_.at(cw), uBlocks);
			}

			// update the counter of bytes to be served
			toServe = (uBytes > toServe) ? 0 : toServe - uBytes;
			if (toServe == 0) {
				// all bytes booked, go to allocation
				stop = true;
				active = false;
				break;
			}
			// continue allocating (if there are available bands)
		} // Closes loop on bands

		if (cwAllocatedBytes > 0)
			vQueueItemCounter++;  // increase counter of served SDU

		// === update virtual buffer === //

		// number of bytes to be consumed from the virtual buffer
		unsigned int consumedBytes = cwAllocatedBytes - (MAC_HEADER + RLC_HEADER_UM);  // TODO RLC may be either UM or AM
		while (!conn->isEmpty() && consumedBytes > 0) {
			unsigned int vPktSize = conn->front().first;
			if (vPktSize <= consumedBytes) {
				// serve the entire vPkt, remove pkt info
				conn->popFront();
				consumedBytes -= vPktSize;

			} else {
				// serve partial vPkt, update pkt info
				PacketInfo newPktInfo = conn->popFront();
				newPktInfo.first = newPktInfo.first - consumedBytes;
				conn->pushFront(newPktInfo);
				consumedBytes = 0;

			}
		}

		if (cwAllocatedBytes > 0) {
			// mark codeword as used
			if (allocatedCws_.find(nodeId) != allocatedCws_.end())
				allocatedCws_.at(nodeId)++;else
				allocatedCws_[nodeId] = 1;

			totalAllocatedBytes += cwAllocatedBytes;

			// create entry in the schedule list
			std::pair<unsigned int, Codeword> scListId(cid, cw);
			if (scheduleList_.find(scListId) == scheduleList_.end())
				scheduleList_[scListId].first = 0;

			// if direction is DL , then schedule list contains number of to-be-trasmitted SDUs ,
			// otherwise it contains number of granted blocks
			//scheduleList_[scListId] += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);
			scheduleList_[scListId].first += ((direction_ == DL) ? vQueueItemCounter : cwAllocatedBlocks);
			scheduleList_[scListId].second = totalAllocatedBytes;

			//to ensure that just one cid is scheduled for each node
			schedulingNodeSet.insert(nodeId);

			if (allocatedCws_.at(nodeId) == MAX_CODEWORDS) {
				eligible = false;
				stop = true;
			}
		} else {

			eligible = false;
			stop = true;
		}
		if (stop)
			break;
	} // Closes loop on Codewords

	return totalAllocatedBytes;

}

void NRSchedulerGnbDl::initialize(Direction dir, LteMacEnb *mac) {
	LteSchedulerEnb::initialize(dir, mac);

	if (getSimulation()->getSystemModule()->hasPar("useQosModel")) {
		if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
			if (scheduler_)
				delete scheduler_;
			scheduler_ = new NRQoSModel(DL);
			scheduler_->setEnbScheduler(this);
		}
	}
}
