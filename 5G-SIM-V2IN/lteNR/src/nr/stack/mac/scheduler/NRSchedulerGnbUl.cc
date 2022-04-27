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

#include "nr/stack/mac/scheduler/NRSchedulerGnbUl.h"
#include "nr/stack/mac/layer/NRMacGnb.h"

LteMacScheduleListWithSizes* NRSchedulerGnbUl::schedule() {
	//std::cout << "NRSchedulerGnbUl::schedule start at " << simTime().dbl() << std::endl;

	//EV << "NRSchedulerGnbUl::schedule performed by Node: " << mac_->getMacNodeId() << endl;

	// clearing structures for new scheduling
	scheduleList_.clear();
	allocatedCws_.clear(); //allocatedCwsNodeCid_.clear();
	schedulingNodeSet.clear();

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

	//std::cout << "NRSchedulerGnbUl::schedule end at " << simTime().dbl() << std::endl;

	return &scheduleList_;
}

bool NRSchedulerGnbUl::racschedule() {
	//std::cout << "NRSchedulerGnbUl::racschedule start at " << simTime().dbl() << std::endl;

	//EV << NOW << " NRSchedulerGnbUl::racschedule --------------------::[ START RAC-SCHEDULE ]::--------------------" << endl;
	//EV << NOW << " NRSchedulerGnbUl::racschedule eNodeB: " << mac_->getMacCellId() << endl;
	//EV << NOW << " NRSchedulerGnbUl::racschedule Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

	RacStatus::iterator it = racStatus_.begin(), et = racStatus_.end();
	unsigned int numberOfNodes = racStatus_.size();

	bool fairSchedule = getSimulation()->getSystemModule()->par("fairRacScheduleInUL").boolValue();		//see GeneralParameters.ned
	for (; it != et; ++it) {

		// get current nodeId
		MacNodeId nodeId = it->first;
		if (schedulingNodeSet.find(nodeId) != schedulingNodeSet.end()) {
			//already one cid scheduled for this node
			continue;
		}

//		if(numberOfNodes>1){
//			std::cout << std::endl;
//		}
		//EV << NOW << " NRSchedulerGnbUl::racschedule handling RAC for node " << nodeId << endl;

		// Get number of logical bands
		unsigned int numBands = mac_->getCellInfo()->getNumBands();
		const unsigned int cw = 0;
		bool allocation = false;
		unsigned int blocks = 0;
		unsigned int reqBlocks = 0;
		unsigned int sumReqBlocks = 0;
		unsigned int bytes = 0;
		unsigned int sumBytes = 0;
		unsigned int bytesize = racStatusInfo_[nodeId]->getBytesize();    //bytes UE want to send
		int restBytes = bytesize;

		//					//default behavior from simuLTE
		//					blocks = 1;
		//					for (Band b = 0; b < numBands; ++b) {
		//						if (allocator_->availableBlocks(nodeId, MACRO, b) > 0) {
		//							unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, blocks, UL);
		//							if (bytes > 0) {
		//								allocator_->addBlocks(MACRO, b, nodeId, 1, bytes);
		//								sumReqBlocks = 1;
		//								sumBytes = bytes;
		//
		//								allocation = true;
		//								break;
		//							}
		//						}
		//					}

		if (!fairSchedule) {
			for (Band b = 0; b < numBands; ++b) {
				blocks = allocator_->availableBlocks(nodeId, MACRO, b); //in this band available blocks

				if (blocks > 0) {
					reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, UL, blocks); //required Blocks
					sumReqBlocks += reqBlocks;
					bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, reqBlocks, UL); //required Bytes
					sumBytes += bytes;

					if (bytes > 0) {

						allocator_->addBlocks(MACRO, b, nodeId, reqBlocks, bytes);

						restBytes -= bytes;
						if (restBytes <= 0 || reqBlocks == blocks) {
							allocation = true;
							break;
						}
					}
				}
			}
		} else {

			for (Band b = 0; b < numBands; ++b) {
				blocks = allocator_->availableBlocks(nodeId, MACRO, b); //in this band available blocks
				if (blocks > 0) {
					unsigned int distrBlock = floor(blocks / numberOfNodes);
					if (distrBlock <= 0) {
						break;
					}

					reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, UL, distrBlock); //required Blocks
					sumReqBlocks += reqBlocks;
					bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, reqBlocks, UL); //required Bytes
					sumBytes += bytes;

					if (bytes > 0) {

						allocator_->addBlocks(MACRO, b, nodeId, reqBlocks, bytes);
						if(numberOfNodes != 1){
							numberOfNodes--;
						}

						restBytes -= bytes;
						if (restBytes <= 0 || reqBlocks == distrBlock) {
							allocation = true;
							break;
						}
					}
				}
			}

		}

		if (allocation) {
			// create scList id for current cid/codeword
			MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
			// we use the LCID corresponding to the SHORT_BSR
			std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
			scheduleList_[scListId].first = sumReqBlocks;
			scheduleList_[scListId].second = sumBytes;

			schedulingNodeSet.insert(nodeId);
		}
	}

	// clean up all requests
	for (auto var : racStatusInfo_) {
		delete var.second;
	}
	racStatus_.clear();
	racStatusInfo_.clear();

	//EV << NOW << " NRSchedulerGnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

	int availableBlocks = allocator_->computeTotalRbs();

	//std::cout << "NRSchedulerGnbUl::racschedule end at " << simTime().dbl() << std::endl;

	return (availableBlocks == 0);
}

void NRSchedulerGnbUl::qosModelSchedule() {

	//std::cout << "NRSchedulerGnbUl::qosModelSchedule start at " << simTime().dbl() << std::endl;

	//retrieve the lambdaValues from the ini file
	double lambdaPriority = getSimulation()->getSystemModule()->par("lambdaPriority").doubleValue();
	double lambdaRemainDelayBudget = getSimulation()->getSystemModule()->par("lambdaRemainDelayBudget").doubleValue();
	double lambdaCqi = getSimulation()->getSystemModule()->par("lambdaCqi").doubleValue();
	double lambdaRtx = getSimulation()->getSystemModule()->par("lambdaRtx").doubleValue();
	double lambdaByteSize = getSimulation()->getSystemModule()->par("lambdaByteSize").doubleValue();			//consider also the size of one packet
	//

	std::map<double, std::vector<ScheduleInfo>> combinedMap;
	//std::vector<MacNodeId> neglectUe;

	// use code from rtxschedule to find out the harqprocesses which need a rtx
	// retrieving reference to HARQ entities

	harqRxBuffers_ = mac_->getHarqRxBuffers();
	HarqRxBuffers::iterator harqIt = harqRxBuffers_->begin(), et = harqRxBuffers_->end();

	for (; harqIt != et; ++harqIt) {

		// get current nodeId
		MacNodeId nodeId = harqIt->first;

		if (nodeId == 0) {
			// UE has left the simulation - erase queue and continue
			harqRxBuffers_->erase(nodeId);
			continue;
		}
		OmnetId id = binder_->getOmnetId(nodeId);
		if (id == 0) {
			harqRxBuffers_->erase(nodeId);
			continue;
		}

		LteHarqBufferRx *currHarq = harqIt->second;
		std::vector<LteHarqProcessRx*> processes = currHarq->getProcessesRx();

		unsigned short process = 0;
		unsigned int maxProcesses = currHarq->getNumHarqProcesses();
		const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info
//		bool breakFlag = false; // used for break if one process was selected
//
		//
//		for (process = 0; process < maxProcesses; ++process) {
//			// for each HARQ process
//			LteHarqProcessRx *currentProcess = harqIt->second->getProcess(process);
//
//			std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
//			std::vector<RxUnitStatus>::iterator pit = procStatus.begin();
//
//			for (; pit != procStatus.end(); ++pit) {
//				if (pit->second == RXHARQ_PDU_EVALUATING) {
//					neglectUe.push_back(nodeId);
//					breakFlag = true;
//					break;
//				}
//			}
//		}
//		if(breakFlag)
//			continue;
		//

		for (process = 0; process < maxProcesses; ++process) {
			LteHarqProcessRx *currentProcess = harqIt->second->getProcess(process);
			//first is codeword
			//second is status
			std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
			std::vector<RxUnitStatus>::iterator pit = procStatus.begin();
			for (; pit != procStatus.end(); ++pit) {
				if (pit->second == RXHARQ_PDU_CORRUPTED) {
					LteMacPdu *pdu = currentProcess->getPdu(pit->first);
					UserControlInfo *lteInfo = check_and_cast<UserControlInfo*>(pdu->getControlInfo());
					MacCid cid = idToMacCid(lteInfo->getSourceId(), lteInfo->getLcid());
					unsigned short qfi = lteInfo->getQfi();
					unsigned short _5qi = mac_->getQosHandler()->get5Qi(qfi);
					double prio = mac_->getQosHandler()->getPriority(_5qi);
					double pdb = mac_->getQosHandler()->getPdb(_5qi);
					ASSERT(pdu->getCreationTime() != 0);
					simtime_t delay = NOW - pdu->getCreationTime();
					simtime_t remainDelayBudget = SimTime(pdb) - delay;

					//create a scheduleInfo
					ScheduleInfo tmp;
					tmp.remainDelayBudget = remainDelayBudget;
					tmp.category = "rtx";
					tmp.cid = cid;
					tmp.codeword = pit->first;
					tmp.harqProcessRx = currentProcess;
					tmp.pdb = pdb;
					tmp.priority = prio;
					tmp.numberRtx = currentProcess->getTransmissions();
					tmp.nodeId = nodeId;
					tmp.process = process;
					tmp.sizeOnePacketUL = pdu->getByteLength();

					double calcPrio = lambdaPriority * (prio / 90.0) + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget) + lambdaCqi * (1.0 / (txParams.readCqiVector().at(0) + 1))
							+ (lambdaRtx * (1.0 / (1.0 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketUL) * lambdaByteSize);

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
		//
	}

	//here we consider the active connections which want to send new data
	//use code from QosModel to find out most prio cid
	//get the activeSet of connections
	//find out which cid has highest priority via qosHandler
	std::map<double, std::vector<QosInfo>> sortedCids = mac_->getQosHandler()->getEqualPriorityMap(UL);

	//create a map with cids which are in the racStatusMap
	std::map<double, std::vector<QosInfo>> racCids;
	ASSERT(racStatusInfo_.size() == racStatus_.size());
	//
	for (auto &var : sortedCids) {
		//qosinfo
		for (auto &qosinfo : var.second) {

			//find the qosinfo sortedCids and the corresponding racStatusInfo
			//racstatusinfo is a map, key is the nodeid of the ue!
			for (auto &racs : racStatusInfo_) {

				//TODO
//				bool conFlag = false;
//				for (auto &ueId : neglectUe) {
//					if (ueId == qosinfo.senderNodeId) {
//						conFlag = true;
//						break;
//					}
//				}
//				if (conFlag) {
//					continue;
//				}
				//

				MacCid realCidRac = idToMacCid(racs.second->getSourceId(), racs.second->getLcid());
				MacCid realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid);
				if (realCidRac == realCidQosInfo) {
					//we have the QosInfos and the cids in the racStatus
					racCids[var.first].push_back(qosinfo);
				}

			}
		}
	}

	std::map<double, std::vector<QosInfo>>::iterator it = racCids.begin();
	//sorted by priority, all active cids which are compete in rac
	for (; it != racCids.end(); ++it) {

		//iterate over all cids with the same priority, find out available delay budget
		std::vector<QosInfo>::iterator qosinfoit = it->second.begin();
		for (; qosinfoit != it->second.end(); ++qosinfoit) {

			// get current nodeId
			MacCid cid = qosinfoit->cid;
			MacNodeId nodeId = MacCidToNodeId(cid);

			//
//			bool conFlag = false;
//			for (auto &ueId : neglectUe) {
//				if (ueId == nodeId) {
//					conFlag = true;
//					break;
//				}
//			}
//			if (conFlag) {
//				continue;
//			}
			//

			MacCid realCid = idToMacCid(nodeId, qosinfoit->lcid);

			unsigned int bytesize = racStatusInfo_[nodeId]->getBytesize();			// the bytesize of the whole queue or single packet

			//this is the creationTime of the first packet in the buffer
			simtime_t creationTimeFirstPacket = racStatusInfo_[nodeId]->getCreationTimeOfQueueFront();
			ASSERT(creationTimeFirstPacket != 0);
			unsigned int sizeOfOnePacketInBytes = racStatusInfo_[nodeId]->getBytesizeOfOnePacket();

			const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info

			//qos relevant values
			unsigned short qfi = qosinfoit->qfi;
			unsigned short _5qi = mac_->getQosHandler()->get5Qi(qfi);
			double prio = mac_->getQosHandler()->getPriority(_5qi);
			double pdb = mac_->getQosHandler()->getPdb(_5qi);

			//delay of this packet
			simtime_t delay = NOW - creationTimeFirstPacket;

			simtime_t remainDelayBudget = SimTime(pdb) - delay;

			//create a scheduleInfo
			ScheduleInfo tmp;
			tmp.remainDelayBudget = remainDelayBudget;
			tmp.category = "newTx";
			tmp.cid = cid;
			tmp.pdb = pdb;
			tmp.priority = prio;
			tmp.numberRtx = 0;
			tmp.nodeId = nodeId;
			tmp.bytesizeUL = bytesize;
			tmp.sizeOnePacketUL = sizeOfOnePacketInBytes;

			if (!getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
				tmp.sizeOnePacketUL = tmp.bytesizeUL;
			}

			double calcPrio = lambdaPriority * (prio / 90.0) + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget) + lambdaCqi * (1.0 / (txParams.readCqiVector().at(0) + 1.0))
					+ (lambdaRtx * (1.0 / (1.0 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketUL) * lambdaByteSize);

			combinedMap[calcPrio].push_back(tmp);
		}
	}

	//this is needed to consider the bsr connections

	//here we consider the active connections which want to send new data
	//use code from QosModel to find out most prio cid
	//get the activeSet of connections
	ActiveSet activeConnectionTempSet_ = scheduler_->getActiveConnectionSet();

	//create a map with cids sorted by priority
	std::map<double, std::vector<QosInfo>> activeCids;
	for (auto &var : sortedCids) {
		for (auto &qosinfo : var.second) {
			for (auto &cid : activeConnectionTempSet_) {

				MacNodeId nodeId = MacCidToNodeId(cid);
				OmnetId id = binder_->getOmnetId(nodeId);

				//
//				bool conFlag = false;
//				for (auto &ueId : neglectUe) {
//					if (ueId == nodeId) {
//						conFlag = true;
//						break;
//					}
//				}
//				if (conFlag) {
//					continue;
//				}
				//

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

				tmp.bytesizeUL = macBuffer->getQueueOccupancy();
				tmp.sizeOnePacketUL = macBuffer->front().first;

				if (!getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
					tmp.sizeOnePacketUL = tmp.bytesizeUL;
				}

				double calcPrio = lambdaPriority * (prio / 90.0) + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget) + lambdaCqi * (1 / (txParams.readCqiVector().at(0) + 1))
						+ (lambdaRtx * (1 / (1 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketUL) * lambdaByteSize);

				combinedMap[calcPrio].push_back(tmp);
			}
		}
	}

	//bsr connections


	std::map<MacNodeId, std::vector<ScheduledInfo>> scheduledInfoMap;

	if (getSimulation()->getSystemModule()->par("combineQosWithRac").boolValue()) {

		//how many resources available for all?
		//how many resources needed per priority?
		//ensure that highest priority is fully satisfied
		//go to next prio
		//key --> calculated weight

		for (auto &prio : combinedMap) {

			int countedRtx = 0;

			//schedule only the rtxs first (all have same prio)
			for (auto &schedInfo : prio.second) {

				//only the connection with the highest Prio of the same UE gets resources
				if (scheduledInfoMap[schedInfo.nodeId].size() == 1) {
					continue;
				}

				if (schedInfo.category == "rtx") {
					unsigned int rtxBytes = schedulePerAcidRtxWithNRHarq(schedInfo.nodeId, schedInfo.codeword, schedInfo.process);

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


			//share the available blocks in a fair manner
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
					unsigned int reqBlocks = 0;
					unsigned int bytes = 0;
					unsigned int bytesize = schedInfo.bytesizeUL;    //bytes UE want to send
					unsigned int sizePerPacket = schedInfo.sizeOnePacketUL; // bytes for one Packet
					int schedBlocks = 0;
					int schedBytesPerSchedInfo = 0;

					for (Band b = 0; b < mac_->getCellInfo()->getNumBands(); ++b) {

						if (blocks > 0) {

							//real required blocks
							if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
								reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, sizePerPacket, UL, blocks); //required Blocks one Packet
							} else {
								reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, UL, blocks); //required Blocks whole Qeue
							}

							//real required bytes
							bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, reqBlocks, UL); //required Bytes
							//shared available blocks
							schedBytesPerSchedInfo = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, blocksPerSchedInfo, UL); //required Bytes
							if(newTxWithEqualPrio == 1){
								schedBytesPerSchedInfo = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, allocator_->availableBlocks(nodeId, MACRO, b), UL);
							}

							//required blocks fit the size of the shared blocks
							if (reqBlocks <= blocksPerSchedInfo) {
								allocator_->addBlocks(MACRO, b, nodeId, reqBlocks, bytes);
								allocation = true;
								break;
							} else {
								//required blocks larger, schedule only the shared blocks
								reqBlocks = blocksPerSchedInfo;
								bytes = schedBytesPerSchedInfo;
								allocator_->addBlocks(MACRO, b, nodeId, blocksPerSchedInfo, schedBytesPerSchedInfo);
								allocation = true;
								break;
							}
						}
					}

					if (allocation) {
						// create scList id for current cid/codeword
						MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
						// we use the LCID corresponding to the SHORT_BSR
						std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
						scheduleList_[scListId].first = reqBlocks;
						scheduleList_[scListId].second = bytes;

						LteMacBufferMap::iterator bsr = bsrbuf_->find(cid);

						if (!(bsr->second->getQueueLength() <= 0)) {
							if (bsr->second->front().first - bytes <= 0) {
								scheduler_->removeActiveConnection(cid);
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
		//key --> calculated weight
		for (auto &var : combinedMap) {

			//values --> vector with ScheduleInfo with the same weight
			for (auto &vec : var.second) {

				//only the connection with the highest Prio of the same UE gets resources
				if (scheduledInfoMap[vec.nodeId].size() == 1) {
					continue;
				}

				if (vec.category == "rtx") {

					unsigned int rtxBytes = schedulePerAcidRtxWithNRHarq(vec.nodeId, vec.codeword, vec.process);

				} else if (vec.category == "newTx") {

					MacCid cid = vec.cid;
					MacNodeId nodeId = vec.nodeId;
					unsigned int numBands = mac_->getCellInfo()->getNumBands();
					const unsigned int cw = 0;
					bool allocation = false;
					unsigned int blocks = 0;
					unsigned int reqBlocks = 0;
					unsigned int bytes = 0;
					unsigned int bytesize = vec.bytesizeUL;    //bytes UE want to send
					unsigned int sizePerPacket = vec.sizeOnePacketUL;

					for (Band b = 0; b < mac_->getCellInfo()->getNumBands(); ++b) {
						blocks = allocator_->availableBlocks(nodeId, MACRO, b); //in this band available blocks
						if (blocks > 0) {

							if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
								reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, sizePerPacket, UL, blocks); //required Blocks for packet
							} else {
								reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, UL, blocks); //required Blocks for Queue
							}

							bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, reqBlocks, UL); //required Bytes

							if (bytes > 0) {
								allocator_->addBlocks(MACRO, b, nodeId, reqBlocks, bytes);
								allocation = true;
								break;
							}
						}
					}

					if (allocation) {
						// create scList id for current cid/codeword
						MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
						// we use the LCID corresponding to the SHORT_BSR
						std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
						scheduleList_[scListId].first = reqBlocks;
						scheduleList_[scListId].second = bytes;

						LteMacBufferMap::iterator bsr = bsrbuf_->find(cid);

						if (!(bsr->second->getQueueLength() <= 0)) {
							if (bsr->second->front().first - bytes <= 0) {
								scheduler_->removeActiveConnection(cid);
							}
						}
					}

					//EV << NOW << " NRSchedulerGnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

					int availableBlocks = allocator_->computeTotalRbs();

				} else {
					throw cRuntimeError("Error");
				}
				ScheduledInfo tmp;
				tmp.nodeId = vec.nodeId;
				tmp.info = vec;
				scheduledInfoMap[vec.nodeId].push_back(tmp);
			}
		}
	}
	//cleanUp
	// clean up all requests
	for (auto &var : racStatusInfo_) {
		delete var.second;
	}
	racStatus_.clear();
	racStatusInfo_.clear();

	//std::cout << "NRSchedulerGnbUl::qosModelSchedule end at " << simTime().dbl() << std::endl;
}

/**
 * changed the behavior
 * racschedule is called after checking whether a rtx is needed
 */
bool NRSchedulerGnbUl::rtxschedule() {

	//std::cout << "NRSchedulerGnbUl::rtxschedule start at " << simTime().dbl() << std::endl;

	//
	if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
		if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
			qosModelSchedule();
			//return always true to avoid calling other scheduling functions
			return true;
		}
	}
	//

	//std::cout << "NRSchedulerGnbUl::rtxschedule start at " << simTime().dbl() << std::endl;

	if (getSimulation()->getSystemModule()->par("nrHarq").boolValue()) {
		return rtxscheduleWithNRHarq();
	}

	// try to handle RAC requests first and abort rtx scheduling if no OFDMA space is left after
	if (getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue()) {
		if (racschedule())
			return true;
	}

	bool rtxNeeded = false;
	try {
		//EV << NOW << " NRSchedulerGnbUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
		//EV << NOW << " NRSchedulerGnbUl::rtxschedule eNodeB: " << mac_->getMacCellId() << endl;
		//EV << NOW << " NRSchedulerGnbUl::rtxschedule Direction: " << (direction_ == UL ? "UL" : "DL") << endl;
		harqRxBuffers_ = mac_->getHarqRxBuffers();
		HarqRxBuffers::iterator it = harqRxBuffers_->begin(), et = harqRxBuffers_->end();

		for (; it != et; ++it) {
			// get current nodeId
			MacNodeId nodeId = it->first;

			if (nodeId == 0) {
				// UE has left the simulation - erase queue and continue
				harqRxBuffers_->erase(nodeId);
				continue;
			}
			OmnetId id = binder_->getOmnetId(nodeId);
			if (id == 0) {
				harqRxBuffers_->erase(nodeId);
				continue;
			}

			// get current Harq Process for nodeId
			unsigned char currentAcid = harqStatus_.at(nodeId);

			// check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
			bool skip = true;
			unsigned char acid = (currentAcid + 2) % (it->second->getProcesses());
			LteHarqProcessRx *currentProcess = it->second->getProcess(acid);
			std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
			std::vector<RxUnitStatus>::iterator pit = procStatus.begin();
			for (; pit != procStatus.end(); ++pit) {
				if (pit->second == RXHARQ_PDU_CORRUPTED) {
					skip = false;
					break;
				}
			}
			if (skip) {
				continue;
			}

			//EV << NOW << "NRSchedulerGnbUl::rtxschedule UE: " << nodeId << "Acid: " << (unsigned int)currentAcid << endl;

			// Get user transmission parameters
			const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info

			unsigned int codewords = txParams.getLayers().size();        // get the number of available codewords
			unsigned int allocatedBytes = 0;

			// TODO handle the codewords join case (sizeof(cw0+cw1) < currentTbs && currentLayers ==1)

			for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
				unsigned int rtxBytes = 0;
				// FIXME PERFORMANCE: check for rtx status before calling rtxAcid

				// perform a retransmission on available codewords for the selected acid
				rtxBytes = schedulePerAcidRtx(nodeId, cw, currentAcid);
				if (rtxBytes > 0) {
					--codewords;
					allocatedBytes += rtxBytes;
					rtxNeeded = true;
				}
			}
			//EV << NOW << "NRSchedulerGnbUl::rtxschedule user " << nodeId << " allocated bytes : " << allocatedBytes << endl;
		}

		int availableBlocks = allocator_->computeTotalRbs();

		//EV << NOW << " NRSchedulerGnbUl::rtxschedule residual OFDM Space: " << availableBlocks << endl;

		//EV << NOW << " NRSchedulerGnbUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

		//std::cout << "NRSchedulerGnbUl::rtxschedule end at " << simTime().dbl() << std::endl;

		if (!getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue()) {
			bool retValue = racschedule();
		}

		return (availableBlocks == 0);

		//return (availableBlocks == 0);
	} catch (std::exception &e) {
		throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxschedule(): %s", e.what());
	}
	//std::cout << "NRSchedulerGnbUl::rtxschedule end at " << simTime().dbl() << std::endl;

	return 0;
}

bool NRSchedulerGnbUl::rtxscheduleWithNRHarq() {

	//std::cout << "NRSchedulerGnbUl::rtxscheduleWithNRHarq start at " << simTime().dbl() << std::endl;

	if (getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue()) {
		if (racschedule())        //racschedule() returns true if no resource blocks are available
			return true;
	}

	try {
		//EV << NOW << " NRSchedulerGnbUl::rtxscheduleWithNRHarq --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
		//EV << NOW << " NRSchedulerGnbUl::rtxscheduleWithNRHarq eNodeB: " << mac_->getMacCellId() << endl;
		//EV << NOW << " NRSchedulerGnbUl::rtxscheduleWithNRHarq Direction: " << (direction_ == UL ? "UL" : "DL") << endl;
		harqRxBuffers_ = mac_->getHarqRxBuffers();
		HarqRxBuffers::iterator it = harqRxBuffers_->begin(), et = harqRxBuffers_->end();

		//
		std::map<std::pair<MacNodeId, Codeword>, std::pair<unsigned short, simtime_t>> macNodeProcessRxTime;
		for (; it != et; ++it) {

			// get current nodeId
			MacNodeId nodeId = it->first;

			if (nodeId == 0) {
				// UE has left the simulation - erase queue and continue
				harqRxBuffers_->erase(nodeId);
				continue;
			}
			OmnetId id = binder_->getOmnetId(nodeId);
			if (id == 0) {
				harqRxBuffers_->erase(nodeId);
				continue;
			}

			if (schedulingNodeSet.find(nodeId) != schedulingNodeSet.end()) {
				//already one cid scheduled for this node
				continue;
			}

			LteHarqBufferRx *currHarq = it->second;
			std::vector<LteHarqProcessRx*> processes = currHarq->getProcessesRx();

			unsigned short process = 0;
			unsigned int maxProcesses = currHarq->getNumHarqProcesses();

			//find oldest process
			for (process = 0; process < maxProcesses; ++process) {
				LteHarqProcessRx *currentProcess = it->second->getProcess(process);
				const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info
				unsigned int codewords = txParams.getLayers().size();

				for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
					if (currentProcess->getByteLength(cw) <= 0) {
						continue;
					}
					std::pair<MacNodeId, Codeword> nodeIdCw = make_pair(nodeId, cw);
					simtime_t timestamp = currentProcess->getRxTimeForCodeWord(cw);
					std::pair<unsigned short, simtime_t> processRxTime = make_pair(process, timestamp);
					macNodeProcessRxTime[nodeIdCw] = processRxTime;
				}
			}
			//
		}

		while (macNodeProcessRxTime.size() > 0) {
			std::map<std::pair<MacNodeId, Codeword>, std::pair<unsigned short, simtime_t>>::iterator it;
			it = macNodeProcessRxTime.begin();
			std::pair<MacNodeId, Codeword> oldestNodeIdCw = it->first;
			std::pair<unsigned short, simtime_t> oldestProcessRxTime = it->second;

			for (auto &var : macNodeProcessRxTime) {
				std::pair<MacNodeId, Codeword> nodeIdCw = var.first;
				std::pair<unsigned short, simtime_t> processRxTime = var.second;

				if (processRxTime < oldestProcessRxTime) {
					oldestNodeIdCw = nodeIdCw;
					oldestProcessRxTime = processRxTime;
				}
			}

			unsigned int rtxBytes = 0;
			// perform a retransmission on available codewords for the selected acid
			rtxBytes = schedulePerAcidRtxWithNRHarq(oldestNodeIdCw.first, oldestNodeIdCw.second, oldestProcessRxTime.first);
			if (rtxBytes > 0) {
				macNodeProcessRxTime.erase(oldestNodeIdCw);
				schedulingNodeSet.insert(oldestNodeIdCw.first);
			} else {
				break;
			}
		}
		//

		int availableBlocks = allocator_->computeTotalRbs();

		if (!getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue()) {
			racschedule();
		}

		return (availableBlocks == 0);

		//return (availableBlocks == 0);
	} catch (std::exception &e) {
		throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxschedule(): %s", e.what());
	}
	//std::cout << "NRSchedulerGnbUl::rtxscheduleWithNRHarq end at " << simTime().dbl() << std::endl;

	return 0;
}

unsigned int NRSchedulerGnbUl::schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	try {
		std::string bands_msg = "BAND_LIMIT_SPECIFIED";
		if (bandLim == NULL) {
			bands_msg = "NO_BAND_SPECIFIED";
			// Create a vector of band limit using all bands
			// FIXME: bandlim is never deleted
			bandLim = new std::vector<BandLimit>();

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

		//EV << NOW << "NRSchedulerGnbUl::rtxAcid - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

		// Get the current active HARQ process
		//        unsigned char currentAcid = harqStatus_.at(nodeId) ;

		unsigned char currentAcid = (harqStatus_.at(nodeId) + 2) % (harqRxBuffers_->at(nodeId)->getProcesses());
		//EV << "\t the acid that should be considered is " << currentAcid << endl;

		LteHarqProcessRx *currentProcess = harqRxBuffers_->at(nodeId)->getProcess(currentAcid);

		if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
			// exit if the current active HARQ process is not ready for retransmission
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid User is on ACID " << (unsigned int)currentAcid << " HARQ process is IDLE. No RTX scheduled ." << endl;
			delete (bandLim);
			return 0;
		}

		Codeword allocatedCw = 0;
		//        Codeword allocatedCw = MAX_CODEWORDS;
		// search for already allocated codeword
		// create "mirror" scList ID for other codeword than current
		std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), MAX_CODEWORDS - cw - 1);
		if (scheduleList_.find(scListMirrorId) != scheduleList_.end()) {
			allocatedCw = MAX_CODEWORDS - cw - 1;
		}
		// get current process buffered PDU byte length
		unsigned int bytes = currentProcess->getByteLength(cw);
		// bytes to serve
		int toServe = bytes;
		// blocks to allocate for each band
		std::vector<unsigned int> assignedBlocks;
		// bytes which blocks from the preceding vector are supposed to satisfy
		std::vector<unsigned int> assignedBytes;

		// end loop signal [same as bytes>0, but more secure]
		bool finish = false;
		// for each band
		unsigned int size = bandLim->size();
		for (unsigned int i = 0; (i < size) && (!finish); ++i) {
			// save the band and the relative limit
			Band b = bandLim->at(i).band_;
			int limit = bandLim->at(i).limit_.at(cw);

			// TODO add support to multi CW
			//            unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
			//                    ((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
			unsigned int availableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
			unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_);

			// use the provided limit as cap for available bytes, if it is not set to unlimited
			if (limit >= 0)
				bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

			//EV << NOW << " NRSchedulerGnbUl::rtxAcid BAND " << b << endl;
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid Available: " << bandAvailableBytes << " bytes" << endl;

			unsigned int servedBytes = 0;
			// there's no room on current band for serving the entire request
			if (bandAvailableBytes < toServe) {
				// record the amount of served bytes
				servedBytes = bandAvailableBytes;
				// the request can be fully satisfied
			} else {
				// record the amount of served bytes
				servedBytes = toServe;
				// signal end loop - all data have been serviced
				finish = true;
			}

			unsigned int servedBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, servedBytes, direction_, availableBlocks);
			//			if (servedBlocks + 2 <= availableBlocks)
			//				servedBlocks += 2;
			servedBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, servedBlocks, UL);
			//

			// update the bytes counter
			toServe -= servedBytes;
			// update the structures
			assignedBlocks.push_back(servedBlocks);
			assignedBytes.push_back(servedBytes);
		}

		if (toServe > 0) {
			// process couldn't be served - no sufficient space on available bands
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)currentAcid << " on codeword " << cw << endl;
			delete (bandLim);
			return 0;
		} else {
			// record the allocation
			unsigned int size = assignedBlocks.size();
			unsigned int cwAllocatedBlocks = 0;
			unsigned int cwAllocatedBytes = 0;

			// create scList id for current node/codeword
			std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), cw);

			for (unsigned int i = 0; i < size; ++i) {
				// For each LB for which blocks have been allocated
				Band b = bandLim->at(i).band_;

				cwAllocatedBlocks += assignedBlocks.at(i);
				cwAllocatedBytes += assignedBytes.at(i);
				//EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
				//! handle multi-codeword allocation
				if (allocatedCw != MAX_CODEWORDS) {
					//EV << NOW << " NRSchedulerGnbUl::rtxAcid - adding " << assignedBlocks.at(i) << " to band " << i << endl;
					allocator_->addBlocks(antenna, b, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
				}
				//! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
			}

			// signal a retransmission
			// schedule list contains number of granted blocks

			scheduleList_[scListId].first = cwAllocatedBlocks;
			scheduleList_[scListId].second = cwAllocatedBytes;
			bytes = cwAllocatedBytes;

			mac_->insertRtxMap(nodeId, currentAcid, cw);

			// mark codeword as used
			if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
				allocatedCws_.at(nodeId)++;}
			else {
				allocatedCws_[nodeId] = 1;
			}

			//EV << NOW << " NRSchedulerGnbUl::rtxAcid HARQ Process " << (unsigned int)currentAcid << " : " << bytes << " bytes served! " << endl;

			delete (bandLim);

			//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

			return bytes;
		}
	} catch (std::exception &e) {
		throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxAcid(): %s", e.what());
	}
	delete (bandLim);

	//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

	return 0;
}

unsigned int NRSchedulerGnbUl::schedulePerAcidRtxWithNRHarq(MacNodeId nodeId, Codeword cw, unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	try {
		std::string bands_msg = "BAND_LIMIT_SPECIFIED";
		if (bandLim == NULL) {
			bands_msg = "NO_BAND_SPECIFIED";
			// Create a vector of band limit using all bands
			// FIXME: bandlim is never deleted
			bandLim = new std::vector<BandLimit>();

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

		//EV << NOW << "NRSchedulerGnbUl::rtxAcid - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

		// Get the current active HARQ process
		//        unsigned char currentAcid = harqStatus_.at(nodeId) ;

		unsigned char currentAcid = acid;
		//EV << "\t the acid that should be considered is " << currentAcid << endl;

		LteHarqProcessRx *currentProcess = harqRxBuffers_->at(nodeId)->getProcess(currentAcid);

		if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
			// exit if the current active HARQ process is not ready for retransmission
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid User is on ACID " << (unsigned int)currentAcid << " HARQ process is IDLE. No RTX scheduled ." << endl;
			delete (bandLim);
			return 0;
		}

		Codeword allocatedCw = 0;
		//        Codeword allocatedCw = MAX_CODEWORDS;
		// search for already allocated codeword
		// create "mirror" scList ID for other codeword than current
		std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), MAX_CODEWORDS - cw - 1);
		if (scheduleList_.find(scListMirrorId) != scheduleList_.end()) {
			allocatedCw = MAX_CODEWORDS - cw - 1;
		}
		// get current process buffered PDU byte length
		unsigned int bytes = currentProcess->getByteLength(cw);

		// bytes to serve
		int toServe = bytes;
		// blocks to allocate for each band
		std::vector<unsigned int> assignedBlocks;
		// bytes which blocks from the preceding vector are supposed to satisfy
		std::vector<unsigned int> assignedBytes;

		// end loop signal [same as bytes>0, but more secure]
		bool finish = false;
		// for each band
		unsigned int size = bandLim->size();
		for (unsigned int i = 0; (i < size) && (!finish); ++i) {
			// save the band and the relative limit
			Band b = bandLim->at(i).band_;
			int limit = bandLim->at(i).limit_.at(cw);

			// TODO add support to multi CW
			//            unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
			//                    ((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
			unsigned int availableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
			unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_);

			// use the provided limit as cap for available bytes, if it is not set to unlimited
			if (limit >= 0)
				bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

			//EV << NOW << " NRSchedulerGnbUl::rtxAcid BAND " << b << endl;
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid Available: " << bandAvailableBytes << " bytes" << endl;

			unsigned int servedBytes = 0;
			// there's no room on current band for serving the entire request
			if (bandAvailableBytes < toServe) {
				// record the amount of served bytes
				servedBytes = bandAvailableBytes;
				// the request can be fully satisfied
			} else {
				// record the amount of served bytes
				servedBytes = toServe;
				// signal end loop - all data have been serviced
				finish = true;
			}

			unsigned int servedBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, servedBytes, direction_, availableBlocks);
			//			if (servedBlocks + 2 <= availableBlocks)
			//				servedBlocks += 2;
			servedBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, servedBlocks, UL);
			//

			// update the bytes counter
			toServe -= servedBytes;
			// update the structures
			assignedBlocks.push_back(servedBlocks);
			assignedBytes.push_back(servedBytes);
		}

		if (toServe > 0) {
			// process couldn't be served - no sufficient space on available bands
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)currentAcid << " on codeword " << cw << endl;
			delete (bandLim);
			return 0;
		} else {
			// record the allocation
			unsigned int size = assignedBlocks.size();
			unsigned int cwAllocatedBlocks = 0;
			unsigned int cwAllocatedBytes = 0;

			// create scList id for current node/codeword
			std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), cw);

			for (unsigned int i = 0; i < size; ++i) {
				// For each LB for which blocks have been allocated
				Band b = bandLim->at(i).band_;

				cwAllocatedBlocks += assignedBlocks.at(i);
				cwAllocatedBytes += assignedBytes.at(i);
				//EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
				//! handle multi-codeword allocation
				if (allocatedCw != MAX_CODEWORDS) {
					//EV << NOW << " NRSchedulerGnbUl::rtxAcid - adding " << assignedBlocks.at(i) << " to band " << i << endl;
					allocator_->addBlocks(antenna, b, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
				}
				//! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
			}

			// signal a retransmission
			// schedule list contains number of granted blocks

			scheduleList_[scListId].first = cwAllocatedBlocks;
			scheduleList_[scListId].second = cwAllocatedBytes;
			bytes = cwAllocatedBytes;

			mac_->insertRtxMap(nodeId, currentAcid, cw);

			// mark codeword as used
			if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
				allocatedCws_.at(nodeId)++;}
			else {
				allocatedCws_[nodeId] = 1;
			}

			//EV << NOW << " NRSchedulerGnbUl::rtxAcid HARQ Process " << (unsigned int)currentAcid << " : " << bytes << " bytes served! " << endl;

			delete (bandLim);

			//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

			return bytes;
		}
	} catch (std::exception &e) {
		throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxAcid(): %s", e.what());
	}
	delete (bandLim);

	//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

	return 0;
}

unsigned int NRSchedulerGnbUl::scheduleGrant(MacCid cid, unsigned int bytes, bool &terminate, bool &active, bool &eligible, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {

	//std::cout << "NRSchedulerGnbUl::scheduleGrant start at " << simTime().dbl() << std::endl;

	// Get the node ID and logical connection ID
	MacNodeId nodeId = MacCidToNodeId(cid);

	if (schedulingNodeSet.find(nodeId) != schedulingNodeSet.end()) {
		//already one cid scheduled for this node
		active = false;
		return 0;
	}

	LogicalCid flowId = MacCidToLcid(cid);

	Direction dir = direction_;

	// Get user transmission parameters
	const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, dir);
	//get the number of codewords
	unsigned int numCodewords = txParams.getLayers().size();

	// TEST: check the number of codewords
	numCodewords = 1;

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
				//                EV << "Putting band " << i << endl;
				// mark as unlimited
				for (unsigned int j = 0; j < numCodewords; j++) {
					//                    EV << "- Codeword " << j << endl;
					elem.limit_.push_back(-1);
				}
				emptyBandLim_.push_back(elem);
			}
		}
		tempBandLim = emptyBandLim_;
		bandLim = &tempBandLim;
	}
	//    EV << "LteSchedulerEnb::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

	unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
	unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)

	// === Perform normal operation for grant === //

	//    EV << "NRSchedulerGnbUl::grant --------------------::[ START GRANT ]::--------------------" << endl;
	//    EV << "NRSchedulerGnbUl::grant Cell: " << mac_->getMacCellId() << endl;
	//    EV << "NRSchedulerGnbUl::grant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

	//! Multiuser MIMO support
	if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER)) {
		// request AMC for MU_MIMO pairing
		MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, dir);
		if (peer != nodeId) {
			// this user has a valid pairing
			//1) register pairing  - if pairing is already registered false is returned
			//            if (allocator_->configureMuMimoPeering(nodeId, peer))
			//                EV << "NRSchedulerGnbUl::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
			//            else
			//                EV << "NRSchedulerGnbUl::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
		} else {
			//            EV << "NRSchedulerGnbUl::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
		}
	}

	// registering DAS spaces to the allocator
	Plane plane = allocator_->getOFDMPlane(nodeId);
	allocator_->setRemoteAntenna(plane, antenna);

	// search for already allocated codeword
	unsigned int cwAlredyAllocated = 0;
	if (allocatedCws_.find(nodeId) != allocatedCws_.end())
		cwAlredyAllocated = allocatedCws_.at(nodeId);

	// Check OFDM space
	// OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
	// if we are tryang to allocate a peer user in mu_mimo plane
	if (allocator_->computeTotalRbs() == 0
			&& (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING && txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlredyAllocated == 0)
					&& (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE))) {
		terminate = true; // ODFM space ended, issuing terminate flag
		//        EV << "NRSchedulerGnbUl::grant Space ended, no schedulation." << endl;
		return 0;
	}

	// TODO This is just a BAD patch
	// check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
	// otherwise check why an UE is stopped being scheduled while its buffer is not empty
	if (cwAlredyAllocated > 0) {
		terminate = true;
		return 0;
	}

	// ===== DEBUG OUTPUT ===== //
	bool debug = false; // TODO: make this configurable
	if (debug) {
		//        if (limitBl)
		//            EV << "NRSchedulerGnbUl::grant blocks: " << bytes << endl;
		//        else
		//            EV << "NRSchedulerGnbUl::grant Bytes: " << bytes << endl;
		//        EV << "NRSchedulerGnbUl::grant Bands: {";
		unsigned int size = (*bandLim).size();
		//        if (size > 0)
		//        {
		//            EV << (*bandLim).at(0).band_;
		//            for(unsigned int i = 1; i < size; i++)
		//                EV << ", " << (*bandLim).at(i).band_;
		//        }
		//        EV << "}\n";
	}
	// ===== END DEBUG OUTPUT ===== //

	//    EV << "NRSchedulerGnbUl::grant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
	//    EV << "NRSchedulerGnbUl::grant Available codewords: " << numCodewords << endl;

	// Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
	Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function
	if (!checkEligibility(nodeId, cw) || cw >= numCodewords) {
		eligible = false;

		//        EV << "NRSchedulerGnbUl::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
		//        EV << "NRSchedulerGnbUl::grant Total allocation: " << totalAllocatedBytes << "bytes" << endl;
		//        EV << "NRSchedulerGnbUl::grant NOT ELIGIBLE!!!" << endl;
		//        EV << "NRSchedulerGnbUl::grant --------------------::[  END GRANT  ]::--------------------" << endl;
		return totalAllocatedBytes; // return the total number of served bytes
	}

	// Get virtual buffer reference
	LteMacBuffer *conn = ((dir == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

	// get the buffer size

	unsigned int queueLength = conn->getQueueOccupancy(); // in bytes
	if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
		queueLength = (conn->getQueueOccupancy() == 0) ? 0 : conn->getQueueOccupancy() / conn->getQueueLength(); // in bytes single packet
	}

	if (queueLength == 0) {
		active = false;
		//        EV << "NRSchedulerGnbUl::scheduleGrant - scheduled connection is no more active . Exiting grant " << endl;
		//        EV << "NRSchedulerGnbUl::grant --------------------::[  END GRANT  ]::--------------------" << endl;
		return totalAllocatedBytes;
	}

	bool stop = false;
	unsigned int toServe = 0;
	for (; cw < numCodewords; ++cw) {
		//        EV << "NRSchedulerGnbUl::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

		queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
		toServe = queueLength;
		//        EV << "NRSchedulerGnbUl::scheduleGrant bytes to be allocated: " << toServe << endl;

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
			//            EV << "NRSchedulerGnbUl::grant --- BAND " << b << " LIMIT " << limit << "---" << endl;

			// if the limit flag is set to skip, jump off
			if (limit == -2) {
				//                EV << "NRSchedulerGnbUl::grant skipping logical band according to limit value" << endl;
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
				bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, dir);
			} else // if limit is expressed in blocks, limit value must be passed to availableBytes function
			{
				bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, dir, (limitBl) ? limit : -1); // available space (in bytes)
				bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
			}

			// if no allocation can be performed, notify to skip the band on next processing (if any)
			if (bandAvailableBytes == 0) {
				//                EV << "NRSchedulerGnbUl::grant Band " << b << "will be skipped since it has no space left." << endl;
				(*bandLim).at(i).limit_.at(cw) = -2;
				continue;
			}

			//if bandLimit is expressed in bytes
			if (!limitBl) {
				// use the provided limit as cap for available bytes, if it is not set to unlimited
				if (limit >= 0 && limit < (int) bandAvailableBytes) {
					bandAvailableBytes = limit;
					//                    EV << "NRSchedulerGnbUl::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
				}
			} else {
				// if bandLimit is expressed in blocks
				if (limit >= 0 && limit < (int) bandAvailableBlocks) {
					bandAvailableBlocks = limit;
					//                    EV << "NRSchedulerGnbUl::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
				}
			}

			//            EV << "NRSchedulerGnbUl::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

			unsigned int uBytes = (bandAvailableBytes > queueLength) ? queueLength : bandAvailableBytes;
			unsigned int uBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, uBytes, dir, bandAvailableBlocks);

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
				//                EV << "NRSchedulerGnbUl::grant - the first SDU/BSR is served entirely, remove it from the virtual buffer, remaining bytes to serve[" << consumedBytes << "]" << endl;
			} else {
				// serve partial vPkt, update pkt info
				PacketInfo newPktInfo = conn->popFront();
				newPktInfo.first = newPktInfo.first - consumedBytes;
				conn->pushFront(newPktInfo);
				consumedBytes = 0;
				//                EV << "NRSchedulerGnbUl::grant - the first SDU/BSR is partially served, update its size [" << newPktInfo.first << "]" << endl;
			}
		}

		//        EV << "NRSchedulerGnbUl::grant Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;
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
			scheduleList_[scListId].first += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);
			scheduleList_[scListId].second = totalAllocatedBytes;

			//to ensure that just one cid is scheduled for the same node
			schedulingNodeSet.insert(nodeId);
			//

			//            EV << "NRSchedulerGnbUl::grant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
			if (allocatedCws_.at(nodeId) == MAX_CODEWORDS) {
				eligible = false;
				stop = true;
			}
		} else {
			//            EV << "NRSchedulerGnbUl::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
			eligible = false;
			stop = true;
		}
		if (stop)
			break;
	} // Closes loop on Codewords

	//    EV << "NRSchedulerGnbUl::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
	//    EV << "NRSchedulerGnbUl::grant --------------------::[  END GRANT  ]::--------------------" << endl;

	//std::cout << "NRSchedulerGnbUl::scheduleGrant end at " << simTime().dbl() << std::endl;

	return totalAllocatedBytes;
}

void NRSchedulerGnbUl::initialize(Direction dir, LteMacEnb *mac) {
	LteSchedulerEnb::initialize(dir, mac);

	if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
		if (scheduler_)
			delete scheduler_;
		scheduler_ = new NRQoSModel(UL);
		scheduler_->setEnbScheduler(this);
		mac_ = check_and_cast<NRMacGnb*>(mac);
	}
}

void NRSchedulerGnbUl::removePendingRac(MacNodeId nodeId)
{
    //std::cout << "NRSchedulerGnbUl::removePendingRac start at " << simTime().dbl() << std::endl;

    racStatus_.erase(nodeId);
    delete racStatusInfo_[nodeId];
    racStatusInfo_.erase(nodeId);

    //std::cout << "NRSchedulerGnbUl::removePendingRac end at " << simTime().dbl() << std::endl;
}

