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

#include "nr/stack/mac/layer/NRMacGnb.h"
#include "stack/mac/packet/LteSchedulingGrant.h"

Define_Module(NRMacGnb);

NRMacGnb::NRMacGnb() :
		LteMacEnb() {
	scheduleListDl_ = NULL;
}

NRMacGnb::~NRMacGnb() {
}

void NRMacGnb::initialize(int stage) {
	LteMacEnb::initialize(stage);
	if (stage == 0) {

		/* Create and initialize MAC Downlink scheduler */
		delete enbSchedulerDl_;
		enbSchedulerDl_ = check_and_cast<LteSchedulerEnbDl*>(new NRSchedulerGnbDl());
		enbSchedulerDl_->initialize(DL, this);

		/* Create and initialize MAC Uplink scheduler */
		delete enbSchedulerUl_;
		enbSchedulerUl_ = check_and_cast<LteSchedulerEnbUl*>(new NRSchedulerGnbUl());
		enbSchedulerUl_->initialize(UL, this);
		harqProcesses_ = getSystemModule()->par("numberHarqProcesses").intValue();
		harqProcessesNR_ = getSystemModule()->par("numberHarqProcessesNR").intValue();
		if (getSystemModule()->par("nrHarq").boolValue()) {
			harqProcesses_ = harqProcessesNR_;
		}
	}else

	if(stage == INITSTAGE_LAST){
		amc_->printTBS();
	}

}

void NRMacGnb::handleMessage(cMessage *msg) {

	//std::cout << "NRMacGnbRealistic::handleMessage start at " << simTime().dbl() << std::endl;

	if (strcmp(msg->getName(), "RRC") == 0) {
		cGate *incoming = msg->getArrivalGate();
		if (incoming == up_[IN]) {
			//from pdcp to mac
			send(msg, gate("lowerLayer$o"));
		} else if (incoming == down_[IN]) {
			//from mac to pdcp
			send(msg, gate("upperLayer$o"));
		}
	}

	if (msg->isSelfMessage()) {
		if (strcmp(msg->getName(), "flushHarqMsg") == 0) {
			flushHarqBuffers();
			delete msg;
			return;
		}
	}

	LteMacBase::handleMessage(msg);

	//std::cout << "NRMacGnbRealistic::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGnb::handleSelfMessage() {
	//std::cout << "NRMacGnb::handleSelfMessage start at " << simTime().dbl() << std::endl;

	/***************
	 *  MAIN LOOP  *
	 ***************/
	EnbType nodeType = cellInfo_->getEnbType();

	//EV << "-----" << ((nodeType==MACRO_ENB)?"MACRO":"MICRO") << " ENB MAIN LOOP -----" << endl;

	/*************
	 * END DEBUG
	 *************/

	/* Reception */

	// extract pdus from all harqrxbuffers and pass them to unmaker
	HarqRxBuffers::iterator hit = harqRxBuffers_.begin();
	HarqRxBuffers::iterator het = harqRxBuffers_.end();
	LteMacPdu *pdu = NULL;
	std::list<LteMacPdu*> pduList;

	for (; hit != het; hit++) {
		pduList = hit->second->extractCorrectPdus();
		while (!pduList.empty()) {
			pdu = pduList.front();
			pduList.pop_front();
			macPduUnmake(pdu);
		}
	}

	/*UPLINK*/
	//EV << "============================================== UPLINK ==============================================" << endl;
	if (binder_->getLastUpdateUlTransmissionInfo() < NOW)  // once per TTI, even in case of multicell scenarios
		binder_->initAndResetUlTransmissionInfo();

	//TODO enable sleep mode also for UPLINK???
	(enbSchedulerUl_->resourceBlocks()) = getNumRbUl();

	enbSchedulerUl_->updateHarqDescs();

	LteMacScheduleListWithSizes *scheduleListUl = enbSchedulerUl_->schedule();
	// send uplink grants to PHY layer
	if (!scheduleListUl->empty())
		sendGrants(scheduleListUl);
	//EV << "============================================ END UPLINK ============================================" << endl;

	//EV << "============================================ DOWNLINK ==============================================" << endl;
	/*DOWNLINK*/
	// Set current available OFDM space
	(enbSchedulerDl_->resourceBlocks()) = getNumRbDl();

	// use this flag to enable/disable scheduling...don't look at me, this is very useful!!!
//    bool activation = true;
//
//    if (activation) {
	// clear previous schedule list
	if (scheduleListDl_ != NULL)
		scheduleListDl_->clear();

	// perform Downlink scheduling
	scheduleListDl_ = enbSchedulerDl_->schedule();        //--> ok

	// requests SDUs to the RLC layer
	if (!scheduleListDl_->empty())
		macSduRequest();
//    }
	//EV << "========================================== END DOWNLINK ============================================" << endl;

	// purge from corrupted PDUs all Rx H-HARQ buffers for all users
	for (hit = harqRxBuffers_.begin(); hit != het; ++hit) {
		hit->second->purgeCorruptedPdus();
	}

	// Message that triggers flushing of Tx H-ARQ buffers for all users
	// This way, flushing is performed after the (possible) reception of new MAC PDUs
	cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
	flushHarqMsg->setSchedulingPriority(1);        // after other messages
	scheduleAt(NOW, flushHarqMsg);

	//EV << "--- END " << ((nodeType == MACRO_ENB) ? "MACRO" : "MICRO") << " ENB MAIN LOOP ---" << endl;

	//std::cout << "NRMacGnb::handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGnb::sendGrants(LteMacScheduleListWithSizes *scheduleList) {
	//std::cout << "LteMacEnb::sendGrants start at " << simTime().dbl() << std::endl;

	//EV << NOW << "LteMacEnb::sendGrants " << endl;

	while (!scheduleList->empty()) {
		LteMacScheduleListWithSizes::iterator it, ot;
		it = scheduleList->begin();

		Codeword cw = it->first.second;
		Codeword otherCw = MAX_CODEWORDS - cw;

		MacCid cid = it->first.first;
		LogicalCid lcid = MacCidToLcid(cid);
		MacNodeId nodeId = MacCidToNodeId(cid);

		unsigned int granted = it->second.first;        //blocks
		unsigned int codewords = 0;

		// removing visited element from scheduleList.
		scheduleList->erase(it);

		if (granted > 0) {
			// increment number of allocated Cw
			++codewords;
		} else {
			// active cw becomes the "other one"
			cw = otherCw;
		}

		std::pair<unsigned int, Codeword> otherPair(nodeId, otherCw);

		if ((ot = (scheduleList->find(otherPair))) != (scheduleList->end())) {
			// increment number of allocated Cw
			++codewords;

			// removing visited element from scheduleList.
			scheduleList->erase(ot);
		}

		if (granted == 0)
			continue; // avoiding transmission of 0 grant (0 grant should not be created)

		//EV << NOW << " LteMacEnb::sendGrants Node[" << getMacNodeId() << "] - " << granted << " blocks to grant for user " << nodeId << " on " << codewords << " codewords. CW[" << cw << "\\" << otherCw << "]" << endl;

		// TODO Grant is set aperiodic as default
		LteSchedulingGrant *grant = new LteSchedulingGrant("LteGrant");

		grant->setDirection(UL);

		grant->setCodewords(codewords);

		// set total granted blocks
		grant->setTotalGrantedBlocks(granted);

		UserControlInfo *uinfo = new UserControlInfo();
		uinfo->setSourceId(getMacNodeId());
		uinfo->setDestId(nodeId);
		uinfo->setFrameType(GRANTPKT);
		uinfo->setCid(cid);
		uinfo->setLcid(lcid);

		grant->setControlInfo(uinfo);
		if (rtxMap[nodeId].size() == 1) {
			//one rtx scheduled
			auto temp = rtxMap[nodeId];
			unsigned short lastProcess = temp.begin()->second.processId;
			grant->setProcessId(lastProcess);
			grant->setNewTx(false);
			rtxMap[nodeId].erase(temp.begin()->second.processId);
			rtxMap.erase(nodeId);
		} else if (rtxMap[nodeId].size() == 0) {
			//no rtx
			grant->setNewTx(true);
			grant->setProcessId(-1);

		} else {
			auto & temp = rtxMap[nodeId];
			unsigned short order = 17;
			for(auto & var : temp){
				if(var.second.order < order){
					order = var.second.order;
				}
			}

			unsigned short processId = temp.at(order).processId;
			grant->setProcessId(processId);
			grant->setNewTx(false);
			rtxMap[nodeId].erase(temp.erase(order));
			//rtxMap.erase(nodeId);
		}

		// get and set the user's UserTxParams
		const UserTxParams &ui = getAmc()->computeTxParams(nodeId, UL);
		UserTxParams *txPara = new UserTxParams(ui);
		grant->setUserTxParams(txPara);

		// acquiring remote antennas set from user info
		const std::set<Remote> &antennas = ui.readAntennaSet();
		std::set<Remote>::const_iterator antenna_it, antenna_et = antennas.end();
		const unsigned int logicalBands = cellInfo_->getNumBands();

		//  HANDLE MULTICW
		for (; cw < codewords; ++cw) {
			unsigned int grantedBytes = 0;

			for (Band b = 0; b < logicalBands; ++b) {
				unsigned int bandAllocatedBlocks = 0;

				for (antenna_it = antennas.begin(); antenna_it != antenna_et; ++antenna_it) {
					bandAllocatedBlocks += enbSchedulerUl_->readPerUeAllocatedBlocks(nodeId, *antenna_it, b);
				}

				grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw, bandAllocatedBlocks, UL);
			}

			grant->setGrantedCwBytes(cw, grantedBytes);
			//EV << NOW << " LteMacEnb::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
		}

		RbMap map;

		enbSchedulerUl_->readRbOccupation(nodeId, map);

		grant->setGrantedBlocks(map);

		// send grant to PHY layer
		sendLowerPackets(grant);
	}

	//std::cout << "LteMacEnb::sendGrants end at " << simTime().dbl() << std::endl;
}


void NRMacGnb::fromPhy(cPacket *pkt) {
	//std::cout << "NRMacGnbRealistic::fromPhy start at " << simTime().dbl() << std::endl;

	UserControlInfo *userInfo = check_and_cast<UserControlInfo*>(pkt->getControlInfo());

	if (userInfo->getFrameType() == DATAPKT) {

		if (qosHandler->getQosInfo().find(userInfo->getCid()) == qosHandler->getQosInfo().end()) {
			QosInfo tmp;
			tmp.appType = (ApplicationType) userInfo->getApplication();
			tmp.cid = userInfo->getCid();
			tmp.lcid = userInfo->getLcid();
			tmp.qfi = userInfo->getQfi();
			tmp.radioBearerId = userInfo->getRadioBearerId();
			tmp.destNodeId = userInfo->getDestId();
			tmp.senderNodeId = userInfo->getSourceId();
			tmp.containsSeveralCids = userInfo->getContainsSeveralCids();
			tmp.rlcType = userInfo->getRlcType();
			tmp.trafficClass = (LteTrafficClass) userInfo->getTraffic();
			qosHandler->getQosInfo()[userInfo->getCid()] = tmp;
		}

	}

	LteMacBase::fromPhy(pkt);

	//std::cout << "NRMacGnbRealistic::fromPhy end at " << simTime().dbl() << std::endl;
}

/**
 * changed the default behaviour from LteMacEnb
 * checks whether there are more than one SDU in the pkt
 * @param pkt
 */
void NRMacGnb::macPduUnmake(cPacket *pkt) {
	//std::cout << "NRMacEnb::macPduUnmake start at " << simTime().dbl() << std::endl;

	LteMacPdu *macPkt = check_and_cast<LteMacPdu*>(pkt);
	while (macPkt->hasSdu()) {
		// Extract and send SDU
		cPacket *upPkt = macPkt->popSdu();
		take(upPkt);
		LteRlcUmDataPdu *rlc = check_and_cast<LteRlcUmDataPdu*>(upPkt);

		if (rlc->getRequest().size() > 0) {
			while (rlc->getRequest().size() > 0) {
				std::vector<LteRlcUmDataPdu*> tmp = rlc->getRequest();
				LteRlcUmDataPdu *t = tmp.back()->dup();
				FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(t->getControlInfo());
				MacCid cid = lteInfo->getCid();
				take(t);
				if (connDescIn_.find(cid) == connDescIn_.end()) {
					FlowControlInfo toStore(*lteInfo);
					connDescIn_[cid] = toStore;
				}
				rlc->getRequest().pop_back();
				sendUpperPackets(t);
				if (qosHandler->getQosInfo().find(cid) == qosHandler->getQosInfo().end()) {
					qosHandler->getQosInfo()[cid].appType = static_cast<ApplicationType>(lteInfo->getApplication());
					qosHandler->getQosInfo()[cid].destNodeId = lteInfo->getDestId();
					qosHandler->getQosInfo()[cid].lcid = lteInfo->getLcid();
					qosHandler->getQosInfo()[cid].qfi = lteInfo->getQfi();
					qosHandler->getQosInfo()[cid].cid = lteInfo->getCid();
					qosHandler->getQosInfo()[cid].radioBearerId = lteInfo->getRadioBearerId();
					qosHandler->getQosInfo()[cid].senderNodeId = lteInfo->getSourceId();
					qosHandler->getQosInfo()[cid].trafficClass = (LteTrafficClass) lteInfo->getTraffic();
					qosHandler->getQosInfo()[cid].rlcType = lteInfo->getRlcType();
					qosHandler->getQosInfo()[cid].containsSeveralCids = lteInfo->getContainsSeveralCids();
				}
			}
		} else {
			//EV << "NRMacGnb: pduUnmaker extracted SDU" << endl;
			FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(upPkt->getControlInfo());
			MacCid cid = lteInfo->getCid();
			if (connDescIn_.find(cid) == connDescIn_.end()) {
				FlowControlInfo toStore(*lteInfo);
				connDescIn_[cid] = toStore;
			}
			if (qosHandler->getQosInfo().find(cid) == qosHandler->getQosInfo().end()) {
				qosHandler->getQosInfo()[cid].appType = static_cast<ApplicationType>(lteInfo->getApplication());
				qosHandler->getQosInfo()[cid].destNodeId = lteInfo->getDestId();
				qosHandler->getQosInfo()[cid].lcid = lteInfo->getLcid();
				qosHandler->getQosInfo()[cid].qfi = lteInfo->getQfi();
				qosHandler->getQosInfo()[cid].cid = lteInfo->getCid();
				qosHandler->getQosInfo()[cid].radioBearerId = lteInfo->getRadioBearerId();
				qosHandler->getQosInfo()[cid].senderNodeId = lteInfo->getSourceId();
				qosHandler->getQosInfo()[cid].trafficClass = (LteTrafficClass) lteInfo->getTraffic();
				qosHandler->getQosInfo()[cid].rlcType = lteInfo->getRlcType();
				qosHandler->getQosInfo()[cid].containsSeveralCids = lteInfo->getContainsSeveralCids();
			}
			sendUpperPackets(upPkt);
		}
	}

	while (macPkt->hasCe()) {

		MacBsr *bsr = check_and_cast<MacBsr*>(macPkt->popCe());
		UserControlInfo *lteInfo = check_and_cast<UserControlInfo*>(macPkt->getControlInfo());
		MacCid cid = lteInfo->getCid();
		bufferizeBsr(bsr, cid);
		delete bsr;
	}

	ASSERT(macPkt->getOwner() == this);
	delete macPkt;

	//std::cout << "NRMacEnb::macPduUnmake end at " << simTime().dbl() << std::endl;

}

/**
 * checks the scheduleListDl_ and the scheduled cids
 * if there are several cids in the scheduleList one requestMessage
 * is generated and send to upper layer
 */
void NRMacGnb::macSduRequest() {
	//std::cout << "NRMacGnb::macSduRequest start at " << simTime().dbl() << std::endl;

	//EV << "----- START NRMacGnb::macSduRequest -----\n";

	// Ask for a MAC sdu for each scheduled user on each codeword
	LteMacScheduleListWithSizes::const_iterator it;
	if (scheduleListDl_->size() == 1) {
		for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++) {
			MacCid destCid = it->first.first;
			Codeword cw = it->first.second;
			MacNodeId destId = MacCidToNodeId(destCid);

			// for each band, count the number of bytes allocated for this ue
			unsigned int allocatedBytes = 0;
			unsigned int allocatedBytesTmp = 0;
			int numBands = cellInfo_->getNumBands();
			for (Band b = 0; b < numBands; b++) {
				// get the number of bytes allocated to this connection
				// (this represents the MAC PDU size)
				allocatedBytes += enbSchedulerDl_->allocator_->getBytes(MACRO, b, destId);
			}
			allocatedBytesTmp = allocatedBytes;
			ASSERT(it->second.second == allocatedBytes);

			if (allocatedBytes <= MAC_HEADER + RLC_HEADER_UM || it->second.second <= MAC_HEADER + RLC_HEADER_UM)
				continue;

			// send the request message to the upper layer
			LteMacSduRequest *macSduRequest = new LteMacSduRequest("LteMacSduRequest");
			macSduRequest->setUeId(destId);
			macSduRequest->setContainsFlag(false);
			macSduRequest->setLcid(MacCidToLcid(destCid));
			macSduRequest->setSduSize(it->second.second - MAC_HEADER);
			macSduRequest->setControlInfo((&connDesc_[destCid])->dup());
			macSduRequest->setCdi(destCid);
			sendUpperPackets(macSduRequest);
		}
	} else {
		int nodeCounter = 0;
		std::map<MacNodeId, unsigned short> nodeCidCounts;
		std::set<MacNodeId> nodeIds;
		std::map<MacNodeId, std::set<MacCid>> nodeCids;
		for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++) {
			MacCid destCid = it->first.first;
			MacNodeId destId = MacCidToNodeId(destCid);
			if (nodeCidCounts.find(destId) == nodeCidCounts.end())
				nodeCidCounts[destId] = 0;
			nodeIds.insert(destId);
			nodeCidCounts[destId]++;
			nodeCids[destId].insert(destCid);
		}
		nodeCounter = nodeIds.size();

		if (nodeCounter > 1) {                //several nodes

			for (auto &node : nodeCids) {

				if (nodeCidCounts[node.first] == 1) { // several Nodes, but one cid for this node

					MacCid destCid = (*node.second.begin());
					MacNodeId destId = node.first;
					Codeword cw;
					unsigned int test;
					for (auto &var : *scheduleListDl_) {
						if (var.first.first == destCid) {
							cw = var.first.second;
							test = var.second.second - MAC_HEADER;
						}
					}

					//            // for each band, count the number of bytes allocated for this ue
					unsigned int allocatedBytes = 0;
					unsigned int allocatedBytesTmp = 0;
					int numBands = cellInfo_->getNumBands();
					for (Band b = 0; b < numBands; b++) {
						// get the number of bytes allocated to this connection
						// (this represents the MAC PDU size)
						allocatedBytes += enbSchedulerDl_->allocator_->getBytes(MACRO, b, destId);
					}
					allocatedBytesTmp = allocatedBytes;
					ASSERT(test == allocatedBytes - MAC_HEADER);
					if (allocatedBytes - MAC_HEADER <= RLC_HEADER_UM || test <= RLC_HEADER_UM)
						continue;

					// send the request message to the upper layer
					LteMacSduRequest *macSduRequest = new LteMacSduRequest("LteMacSduRequest");
					macSduRequest->setUeId(destId);
					macSduRequest->setContainsFlag(false);
					macSduRequest->setLcid(MacCidToLcid(destCid));
					macSduRequest->setSduSize(allocatedBytes - MAC_HEADER);
					macSduRequest->setControlInfo((&connDesc_[destCid])->dup());
					macSduRequest->setCdi(destCid);
					sendUpperPackets(macSduRequest);

					continue;
				}

				MacCid newCid = idToMacCid(node.first, 0); // several cids
				std::pair<MacCid, Codeword> schedulePair;
				for (auto &var : *scheduleListDl_) {
					for (auto &cid : node.second) {
						if (var.first.first == cid)
							schedulePair = std::make_pair(newCid, var.first.second);
					}
				}
				unsigned int allocatedBytes = 0;
				unsigned int allocatedBytesTmp = 0;
				for (auto &var : *scheduleListDl_) {
					if (MacCidToNodeId(var.first.first) == node.first)
						allocatedBytes += var.second.second - MAC_HEADER;
				}

				if (allocatedBytes <= RLC_HEADER_UM)
					continue;

				allocatedBytesTmp = allocatedBytes;
				LteMacSduRequest *macSduRequest = new LteMacSduRequest("LteMacSduRequest");
				macSduRequest->setContainsFlag(true);
				macSduRequest->setCdi(newCid);
				macSduRequest->setSduSize(allocatedBytes);
				FlowControlInfo *lteInfo = new FlowControlInfo();
				lteInfo->setRlcType(UM);
				lteInfo->setContainsSeveralCids(true);
				macSduRequest->setControlInfo(lteInfo);
				macSduRequest->setByteLength(allocatedBytes);

				for (auto &var : *scheduleListDl_) {
					MacCid destCid = var.first.first;
					for (auto &cids : node.second) {
						if (cids == destCid) {
							if (allocatedBytesTmp <= RLC_HEADER_UM)
								continue;
							MacNodeId destId = MacCidToNodeId(destCid);
							LteMacSduRequest *tmp = new LteMacSduRequest("LteMacSduRequest");
							// send the request message to the upper layer
							tmp->setUeId(destId);
							tmp->setLcid(MacCidToLcid(destCid));
							tmp->setSduSize(var.second.second - MAC_HEADER);
							tmp->setControlInfo((&connDesc_[destCid])->dup());
							tmp->setCdi(destCid);
							allocatedBytesTmp = allocatedBytesTmp - tmp->getSduSize();
							macSduRequest->getRequest().push_back(tmp);
							scheduleListDl_->erase(var.first);
						}
					}

				}

				if (macSduRequest->getRequest().size() == 0) {
					delete macSduRequest;
				} else {
					(*scheduleListDl_)[schedulePair] = std::pair<unsigned int, unsigned int>(1, allocatedBytes);
					macSduRequest->setName("LteMacSduRequest");
					sendUpperPackets(macSduRequest);
				}
			}
		} else { //one node and several cids

			it = scheduleListDl_->begin();
			MacCid newCid = idToMacCid(MacCidToNodeId(it->first.first), 0);
			std::pair<MacCid, Codeword> schedulePair(newCid, it->first.second);
			unsigned int allocatedBytes = 0;
			unsigned int allocatedBytesTmp = 0;
			for (auto &var : *scheduleListDl_) {
				allocatedBytes += var.second.second - MAC_HEADER;
			}

			if (allocatedBytes <= RLC_HEADER_UM || it->second.second <= RLC_HEADER_UM)
				return;
			allocatedBytesTmp = allocatedBytes;
			LteMacSduRequest *macSduRequest = new LteMacSduRequest("LteMacSduRequest");
			macSduRequest->setContainsFlag(true);
			macSduRequest->setCdi(newCid);
			macSduRequest->setSduSize(allocatedBytes);
			FlowControlInfo *lteInfo = new FlowControlInfo();
			lteInfo->setRlcType(UM);
			lteInfo->setContainsSeveralCids(true);
			macSduRequest->setControlInfo(lteInfo);
			macSduRequest->setByteLength(allocatedBytes);

			//iterate over all scheduledSids and build a request, insert it into macSduRequest
			for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++) {
				if (allocatedBytesTmp <= RLC_HEADER_UM)
					continue;
				MacCid destCid = it->first.first;
				MacNodeId destId = MacCidToNodeId(destCid);
				LteMacSduRequest *tmp = new LteMacSduRequest("LteMacSduRequest");
				// send the request message to the upper layer
				tmp->setUeId(destId);
				tmp->setLcid(MacCidToLcid(destCid));
				tmp->setSduSize(it->second.second - MAC_HEADER);
				tmp->setControlInfo((&connDesc_[destCid])->dup());
				tmp->setCdi(destCid);
				allocatedBytesTmp = allocatedBytesTmp - tmp->getSduSize();
				macSduRequest->getRequest().push_back(tmp);
			}

			if (macSduRequest->getRequest().size() == 0) {
				delete macSduRequest;
			} else {
				scheduleListDl_->clear();
				(*scheduleListDl_)[schedulePair] = std::pair<unsigned int, unsigned int>(1, allocatedBytes);
				macSduRequest->setName("LteMacSduRequest");
				sendUpperPackets(macSduRequest);
			}
		}

	}

	//EV << "------ END NRMacGnb::macSduRequest ------\n";

	//std::cout << "NRMacGnb::macSduRequest end at " << simTime().dbl() << std::endl;
}

void NRMacGnb::macPduMake(MacCid cid) {
	//std::cout << "NRMacGnb::macPduMake start at " << simTime().dbl() << std::endl;

	//EV << "----- START NRMacGnb::macPduMake -----\n";
	// Finalizes the scheduling decisions according to the schedule list,
	// detaching sdus from real buffers.

	macPduList_.clear();

	//  Build a MAC pdu for each scheduled user on each codeword
	LteMacScheduleListWithSizes::const_iterator it;
	for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++) {
		LteMacPdu *macPkt;
		cPacket *pkt;
		MacCid destCid = it->first.first;

		if (destCid != cid)
			continue;

		// check whether the RLC has sent some data. If not, skip
		// (e.g. because the size of the MAC PDU would contain only MAC header - MAC SDU requested size = 0B)
		if (mbuf_[destCid]->getQueueLength() == 0)
			break;

		Codeword cw = it->first.second;
		MacNodeId destId = MacCidToNodeId(destCid);
		std::pair<MacCid, Codeword> pktId = std::pair<MacCid, Codeword>(destCid, cw);
		unsigned int sduPerCid = it->second.first;
		unsigned int grantedBlocks = 0;
		TxMode txmode;
		MacPduList::iterator pit = macPduList_.find(pktId);

		while (sduPerCid > 0) {
			if ((mbuf_[destCid]->getQueueLength()) < (int) sduPerCid) {
				throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
						"Queue real SDU length is %d  while scheduled SDUs are %d", destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
			}

			// Add SDU to PDU

			// No packets for this user on this codeword
			if (pit == macPduList_.end()) {
				UserControlInfo *uinfo = new UserControlInfo();
				uinfo->setSourceId(getMacNodeId());
				uinfo->setDestId(destId);
				uinfo->setDirection(DL);

				const UserTxParams &txInfo = amc_->computeTxParams(destId, DL);

				UserTxParams *txPara = new UserTxParams(txInfo);

				uinfo->setUserTxParams(txPara);
				txmode = txInfo.readTxMode();
				RbMap rbMap;
				uinfo->setTxMode(txmode);
				uinfo->setCw(cw);
				grantedBlocks = enbSchedulerDl_->readRbOccupation(destId, rbMap);

				uinfo->setGrantedBlocks(rbMap);
				uinfo->setTotalGrantedBlocks(grantedBlocks);

				//
				uinfo->setLcid(qosHandler->getQosInfo()[destCid].lcid);
				uinfo->setQfi(qosHandler->getQosInfo()[destCid].qfi);
				uinfo->setRadioBearerId(qosHandler->getQosInfo()[destCid].radioBearerId);
				uinfo->setApplication(qosHandler->getQosInfo()[destCid].appType);
				uinfo->setTraffic(qosHandler->getQosInfo()[destCid].trafficClass);
				uinfo->setRlcType(qosHandler->getQosInfo()[destCid].rlcType);
				uinfo->setCid(destCid);
				//

				macPkt = new LteMacPdu("LteMacPdu");
				macPkt->setHeaderLength(MAC_HEADER);
				macPkt->setControlInfo(uinfo);
				macPkt->setTimestamp(NOW);
				macPduList_[pktId] = macPkt;
			} else {
				macPkt = pit->second;
			}
			if (mbuf_[destCid]->getQueueLength() == 0) {
				throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
						"Queue real SDU length is %d  while scheduled SDUs are %d", destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
			}
			pkt = mbuf_[destCid]->popFront(); // is LteRlcUmDataPdu

			ASSERT(pkt != NULL);

			drop(pkt);
			macPkt->pushSdu(pkt);
			sduPerCid--;
		}
	}

	MacPduList::iterator pit;
	for (pit = macPduList_.begin(); pit != macPduList_.end(); pit++) {
		MacCid cid = pit->first.first;
		MacNodeId destId = MacCidToNodeId(cid);
		Codeword cw = pit->first.second;

		LteHarqBufferTx *txBuf;
		HarqTxBuffers::iterator hit = harqTxBuffers_.find(destId);
		if (hit != harqTxBuffers_.end()) {
			txBuf = hit->second;
		} else {
			// FIXME: possible memory leak

			LteHarqBufferTx *hb = new LteHarqBufferTx(harqProcesses_, this, (LteMacBase*) getMacUe(destId));
			harqTxBuffers_[destId] = hb;
			txBuf = hb;
		}
		UnitList txList = (txBuf->firstAvailable());

		LteMacPdu *macPkt = pit->second;
		//EV << "LteMacBase: pduMaker created PDU: " << macPkt->info() << endl;

		// pdu transmission here (if any)
		if (txList.second.empty()) {
			//EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
			delete macPkt;
		} else {
			if (txList.first == HARQ_NONE)
				throw cRuntimeError("LteMacBase: pduMaker sending to uncorrect void H-ARQ process. Aborting");
			txBuf->insertPdu(txList.first, cw, macPkt);
		}
	}
	//EV << "------ END NRMacGnb::macPduMake ------\n";

	//std::cout << "NRMacGnb::macPduMake end at " << simTime().dbl() << std::endl;
}

bool NRMacGnb::bufferizePacket(cPacket *pkt) {
	//std::cout << "NRMacGnb::bufferizePacket start at " << simTime().dbl() << std::endl;

	if (pkt->getByteLength() == 0)
		return false;

	pkt->setTimestamp();        // Add timestamp with current time to packet

	FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

	// obtain the cid from the packet informations
	MacCid cid = ctrlInfoToMacCid(lteInfo);

	// this packet is used to signal the arrival of new data in the RLC buffers
	if (strcmp(pkt->getName(), "newDataPkt") == 0) {
		// update the virtual buffer for this connection

		// build the virtual packet corresponding to this incoming packet
		PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());

		LteMacBufferMap::iterator it = macBuffers_.find(cid);
		if (it == macBuffers_.end()) {
			LteMacBuffer *vqueue = new LteMacBuffer();
			vqueue->pushBack(vpkt);
			macBuffers_[cid] = vqueue;

			// make a copy of lte control info and store it to traffic descriptors map
			FlowControlInfo toStore(*lteInfo);
			connDesc_[cid] = toStore;
			// register connection to lcg map.
			LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

			lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

			//EV << "LteMacBuffers : Using new buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Bytes in the Queue: " << vqueue->getQueueOccupancy() << "\n";
		} else {
			LteMacBuffer *vqueue = macBuffers_.find(cid)->second;
			if (vqueue == NULL || vqueue == nullptr) {
				LteMacBuffer *vqueue = new LteMacBuffer();
				vqueue->pushBack(vpkt);
				macBuffers_[cid] = vqueue;

				// make a copy of lte control info and store it to traffic descriptors map
				FlowControlInfo toStore(*lteInfo);
				connDesc_[cid] = toStore;
				// register connection to lcg map.
				LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

				lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));
			} else {
				vqueue->pushBack(vpkt);
			}

			//EV << "LteMacBuffers : Using old buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << vqueue->getQueueOccupancy() << "\n";
		}

		return true;    // notify the activation of the connection
	}

	// this is a MAC SDU, bufferize it in the MAC buffer

	LteMacBuffers::iterator it = mbuf_.find(cid);
	if (it == mbuf_.end()) {
		// Queue not found for this cid: create
		LteMacQueue *queue = new LteMacQueue(queueSize_);

		queue->pushBack(pkt);

		mbuf_[cid] = queue;

		//EV << "LteMacBuffers : Using new buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << queue->getQueueSize() - queue->getByteLength() << "\n";
	} else {
		// Found
		LteMacQueue *queue = it->second;
		if (!queue->pushBack(pkt)) {
			totalOverflowedBytes_ += pkt->getByteLength();
			double sample = (double) totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
			if (lteInfo->getDirection() == DL) {
				emit(macBufferOverflowDl_, sample);
			} else {
				emit(macBufferOverflowUl_, sample);
			}

			//EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
			pkt->setBitError(true);
			return false;
		}

		//EV << "LteMacBuffers : Using old buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << queue->getQueueSize() - queue->getByteLength() << "\n";
	}

	//std::cout << "NRMacGnb::bufferizePacket end at " << simTime().dbl() << std::endl;

	return false; // do not need to notify the activation of the connection (already done when received newDataPkt)
}

void NRMacGnb::handleUpperMessage(cPacket *pkt) {
	//std::cout << "NRMacGnb::handleUpperMessage start at " << simTime().dbl() << std::endl;

	FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());
	MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());

	//true, if in bufferizePacket a macBufferOverflow was detected
	if(pkt->hasBitError()){
		delete pkt;
		return;
	}

	if (bufferizePacket(pkt)) {
		enbSchedulerDl_->backlog(cid);
	}

	if (strcmp(pkt->getName(), "lteRlcFragment") == 0) {
		// new MAC SDU has been received
		if (pkt->getByteLength() == 0)
			delete pkt;
		else {  // creates pdus from schedule list and puts them in harq buffers

			macPduMake(cid);
		}
	} else {
		delete pkt;
	}

	//std::cout << "NRMacGnb::handleUpperMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGnb::flushHarqBuffers() {
	//std::cout << "NRMacGnb::flushHarqBuffers start at " << simTime().dbl() << std::endl;

	HarqTxBuffers::iterator it;
	for (it = harqTxBuffers_.begin(); it != harqTxBuffers_.end(); it++)
		it->second->sendSelectedDown();

	//std::cout << "NRMacGnb::flushHarqBuffers end at " << simTime().dbl() << std::endl;
}
