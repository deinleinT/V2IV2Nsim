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

#include "nr/stack/mac/layer/NRMacUe.h"

Define_Module(NRMacUe);

void NRMacUe::initialize(int stage) {
	LteMacUe::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		std::string rlcUmType = getParentModule()->getSubmodule("rlc")->par("LteRlcUmType").stdstringValue();
		std::string macType = getParentModule()->par("LteMacType").stdstringValue();
		if (macType.compare("NRMacUe") != 0 && rlcUmType.compare("NRRlcUm") != 0)
			throw cRuntimeError("LteMacUe::initialize - %s module found, must be LteRlcUmRealistic. Aborting", rlcUmType.c_str());
		qosHandler = check_and_cast<QosHandlerUE*>(getParentModule()->getSubmodule("qosHandler"));

		lcgScheduler_ = new NRSchedulerUeUl(this);
		harqProcesses_ = getSystemModule()->par("numberHarqProcesses").intValue();
		harqProcessesNR_ = getSystemModule()->par("numberHarqProcessesNR").intValue();
		if (getSystemModule()->par("nrHarq").boolValue()) {
			raRespWinStart_ = getSystemModule()->par("raRespWinStartNR").intValue();
			harqProcesses_ = harqProcessesNR_;
		}
	}
}

void NRMacUe::macHandleGrant(cPacket *pkt) {
	//std::cout << "NRMacUe macHandleGrant start at " << simTime().dbl() << std::endl;

	//EV << NOW << " NRMacUe::macHandleGrant - UE [" << nodeId_ << "] - Grant received" << endl;

	LteSchedulingGrant *grant = check_and_cast<LteSchedulingGrant*>(pkt);

	if (!isRtxSignalisedEnabled()) {
		if (grant->getNewTx()) {
			//check whether there is already a newTX grant
			if (!getSystemModule()->par("nrHarq").boolValue()) {
				for (auto &var : schedulingGrantMap) {
					if (var.second->getNewTx()){
						delete grant;
						return;
					}

					if (var.second->getProcessId() == grant->getProcessId()){
						delete grant;
						return;
					}
				}
			}
			if (!firstTx) {
				grant->setProcessId(harqProcesses_ - 2);
				schedulingGrantMap[harqProcesses_ - 2] = grant;
			} else {
				grant->setProcessId(*racRequests_.begin());
				schedulingGrantMap[*racRequests_.begin()] = grant;
			}
			racRequests_.clear();

		} else {
			//check whether there is a grant for a RTX for the same processNumber
			for (auto &var : schedulingGrantMap) {
				if (var.second->getProcessId() == grant->getProcessId()) {
					delete schedulingGrantMap[char(grant->getProcessId())];
					//std::cout << "RTX Grant available for process " << grant->getProcessId() << " although there is a grant in map" << std::endl;
					break;
				}
			}
			schedulingGrantMap[char(grant->getProcessId())] = grant;

		}
		if (schedulingGrantMap.empty()) {
			delete schedulingGrant_;
			schedulingGrant_ = NULL;
			return;
		}
		if (schedulingGrantMap.find(grant->getProcessId()) != schedulingGrantMap.end()) {
			schedulingGrant_ = schedulingGrantMap[grant->getProcessId()];
		} else {
			for (auto &var : schedulingGrantMap) {
				if (var.second->getNewTx()) {
					schedulingGrant_ = var.second;
					break;
				}
			}
		}
	} else {

		if (schedulingGrant_ != NULL) {
			delete schedulingGrant_;
			schedulingGrant_ = NULL;
		}

		schedulingGrant_ = grant;

		if (grant->getPeriodic()) {
			periodCounter_ = grant->getPeriod();
			expirationCounter_ = grant->getExpiration();
		}
	}

// clearing pending RAC requests
	racRequested_ = false;

//std::cout << "NRMacUe macHandleGrant start at " << simTime().dbl() << std::endl;
}

void NRMacUe::handleMessage(cMessage *msg) {

//std::cout << "NRMacUe::handleMessage start at " << simTime().dbl() << std::endl;

	if (strcmp(msg->getName(), "RRC") == 0) {
		cGate *incoming = msg->getArrivalGate();
		if (incoming == up_[IN]) {
			//from rlc to phy
			send(msg, gate("lowerLayer$o"));
		} else if (incoming == down_[IN]) {
			//from phy to rlc
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

//std::cout << "NRMacUe::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacUe::fromPhy(cPacket *pkt) {

//std::cout << "NRMacUe::fromPhy start at " << simTime().dbl() << std::endl;

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

//std::cout << "NRMacUe::fromPhy end at " << simTime().dbl() << std::endl;
}

/**
 * checks the scheduleListDl_ and the scheduled cids
 * if there are several cids in the scheduleList one requestMessage
 * is generated and send to upper layer
 */
int NRMacUe::macSduRequest() {

//std::cout << "NRMacUe macSduRequest start at " << simTime().dbl() << std::endl;

//EV << "----- START NRMacUe::macSduRequest -----\n";
	int sent = 0;

// Ask for a MAC sdu for each scheduled user on each codeword
	LteMacScheduleListWithSizes::const_iterator it;

//more than one Cid in schedule list
//create one MacSduRequest with all requests
	if (scheduleListWithSizes_.size() > 1) {
		// get the number of granted bytes
		it = scheduleListWithSizes_.begin();
		MacCid newCid = idToMacCid(MacCidToNodeId(it->first.first), 0);
		std::pair<MacCid, Codeword> schedulePair(newCid, it->first.second);
		unsigned int allocatedByte = 0;
		unsigned int allocatedBytesTmp = 0;
		for (auto &var : scheduleListWithSizes_) {
			allocatedByte += var.second.second - MAC_HEADER;
		}

		if (allocatedByte <= RLC_HEADER_UM)
			return sent;

		allocatedBytesTmp = allocatedByte;

		LteMacSduRequest *macSduRequest = new LteMacSduRequest("LteMacSduRequest");
		macSduRequest->setContainsFlag(true);
		macSduRequest->setCdi(newCid);
		macSduRequest->setSduSize(allocatedByte);
		FlowControlInfo *lteInfo = new FlowControlInfo();
		lteInfo->setRlcType(UM);
		lteInfo->setContainsSeveralCids(true);
		lteInfo->setCid(newCid);
		macSduRequest->setControlInfo(lteInfo);
		macSduRequest->setByteLength(allocatedByte);

		//iterate over all scheduledSids and build a request, insert it into macSduRequest
		for (it = scheduleListWithSizes_.begin(); it != scheduleListWithSizes_.end(); it++) {
			if (allocatedBytesTmp <= RLC_HEADER_UM)
				break;
			MacCid destCid = it->first.first;
			MacNodeId destId = MacCidToNodeId(destCid);

			LteMacSduRequest *tmp = new LteMacSduRequest("LteMacSduRequest");
			// send the request message to the upper layer
			tmp->setUeId(destId);
			tmp->setLcid(MacCidToLcid(destCid));
			tmp->setSduSize(it->second.second - MAC_HEADER);
			allocatedBytesTmp = allocatedBytesTmp - tmp->getSduSize();
			tmp->setControlInfo((&connDesc_[destCid])->dup());
			tmp->setCdi(destCid);
			macSduRequest->getRequest().push_back(tmp);
		}
		if (macSduRequest->getRequest().size() == 0) {
			if (macSduRequest->removeControlInfo())
				delete macSduRequest->removeControlInfo();
			delete macSduRequest;
			return sent;
		} else {
			scheduleListWithSizes_.clear();
			scheduleListWithSizes_[schedulePair] = std::pair<unsigned int, unsigned int>(1, allocatedByte);
			sendUpperPackets(macSduRequest);
			return 1;
		}
	}

//send the request to rlc
	unsigned int allocatedBytes = 0;
	for (auto &var : scheduleListWithSizes_) {
		allocatedBytes += var.second.second - MAC_HEADER;
	}
	for (it = scheduleListWithSizes_.begin(); it != scheduleListWithSizes_.end(); it++) {

		if (allocatedBytes <= RLC_HEADER_UM)
			break;
		MacCid destCid = it->first.first;
		MacNodeId destId = MacCidToNodeId(destCid);
		LteMacSduRequest *macSduRequest = new LteMacSduRequest("LteMacSduRequest");

		if (it->second.second <= MAC_HEADER + RLC_HEADER_UM)
			continue;

		// send the request message to the upper layer
		macSduRequest->setUeId(destId);
		macSduRequest->setLcid(MacCidToLcid(destCid));
		macSduRequest->setSduSize(it->second.second - MAC_HEADER);
		macSduRequest->setControlInfo((&connDesc_[destCid])->dup());
		macSduRequest->setCdi(destCid);

		sendUpperPackets(macSduRequest);

		sent = 1;
	}

//EV << "------ END NRMacUe::macSduRequest ------\n";

//std::cout << "NRMacUe macSduRequest end at " << simTime().dbl() << std::endl;

	return sent;
}

/**
 * changed the default behaviour from LteMacEnb
 * checks after successive reception whether there are more than one SDU in the pkt
 * extract each of them and send it to rlc layer
 * @param pkt
 */
void NRMacUe::macPduUnmake(cPacket *pkt) {
	LteMacPdu *macPkt = check_and_cast<LteMacPdu*>(pkt);
	while (macPkt->hasSdu()) {
		cPacket *upPkt = macPkt->popSdu(); // LteRlcUMDataPdu --> flag
		take(upPkt);
		LteRlcUmDataPdu *rlcPdu = check_and_cast<LteRlcUmDataPdu*>(upPkt);

		if (rlcPdu->getRequest().size() > 0) {
			while (rlcPdu->getRequest().size() > 0) {
				std::vector<LteRlcUmDataPdu*> tmp = rlcPdu->getRequest();
				LteRlcUmDataPdu *t = tmp.back()->dup(); // contains SDU in SDU List!
				FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(t->getControlInfo());
				MacCid cid = lteInfo->getCid();
				take(t);
				if (connDescIn_.find(cid) == connDescIn_.end()) {
					FlowControlInfo toStore(*lteInfo);
					connDescIn_[cid] = toStore; // QosHandler
				}
				rlcPdu->getRequest().pop_back();
				sendUpperPackets(t);
				//update qoshander
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
			delete rlcPdu;
		} else {
			//EV << "NRMacUe: pduUnmaker extracted SDU" << endl;
			FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(upPkt->getControlInfo());
			MacCid cid = lteInfo->getCid();
			if (connDescIn_.find(cid) == connDescIn_.end()) {
				FlowControlInfo toStore(*lteInfo);
				connDescIn_[cid] = toStore;
			}
			//update qoshandler
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

	ASSERT(macPkt->getOwner() == this);
	delete macPkt;
}

void NRMacUe::handleSelfMessage() {
//std::cout << "NRMacUe handleSelfMessage start at " << simTime().dbl() << std::endl;

//EV << "----- UE MAIN LOOP -----" << endl;

	if (getSystemModule()->par("nrHarq").boolValue()) {
		handleSelfMessageWithNRHarq();
		return;
	}

// extract pdus from all harqrxbuffers and pass them to unmaker
	HarqRxBuffers::iterator hit = harqRxBuffers_.begin();
	HarqRxBuffers::iterator het = harqRxBuffers_.end();
	LteMacPdu *pdu = NULL;
	std::list<LteMacPdu*> pduList;

	for (; hit != het; ++hit) {
		pduList = hit->second->extractCorrectPdus();
		while (!pduList.empty()) {
			pdu = pduList.front();
			pduList.pop_front();
			macPduUnmake(pdu);
		}
	}

//EV << NOW << "NRMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int) currentHarq_ << endl;

	if (!rtxSignalisedFlagEnabled)
		checkConfiguredGrant();

	if (schedulingGrant_ == NULL) {
		//EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;
		checkRAC();

	} else if (schedulingGrant_->getPeriodic()) {
		// Periodic checks
		if (--expirationCounter_ < 0) {
			// Periodic grant is expired
			delete schedulingGrant_;
			schedulingGrant_ = NULL;
			// if necessary, a RAC request will be sent to obtain a grant
			checkRAC();
			//return;
		} else if (--periodCounter_ > 0) {
			return;
		} else {
			// resetting grant period
			periodCounter_ = schedulingGrant_->getPeriod();
			// this is periodic grant TTI - continue with frame sending
		}
	}

	requestedSdus_ = 0;
	if (schedulingGrant_ != NULL) // if a grant is configured
	{
		if (!firstTx) {
			//EV << "\t currentHarq_ counter initialized " << endl;
			firstTx = true;
			currentHarq_ = harqProcesses_ - 2;
		}

		bool retx = false;

		HarqTxBuffers::iterator it2;
		LteHarqBufferTx *currHarq;
		//map with gNodeB id as key, and LteHarqBufferTx is value with processes
		ASSERT(harqTxBuffers_.size() == 0 || harqTxBuffers_.size() == 1);
		for (it2 = harqTxBuffers_.begin(); it2 != harqTxBuffers_.end(); it2++) {
			//EV << "\t Looking for retx in acid " << (unsigned int) currentHarq_ << endl;
			currHarq = it2->second;

			retx = currHarq->getProcess(currentHarq_)->hasReadyUnits();
			CwList cwListRetx = currHarq->getProcess(currentHarq_)->readyUnitsIds();

			//EV << "\t [process=" << (unsigned int) currentHarq_ << "] , [retx=" << ((retx) ? "true" : "false") << "] , [n=" << cwListRetx.size() << "]" << endl;

			// if a retransmission is needed
			if (retx) {
				UnitList signal;
				signal.first = currentHarq_;
				signal.second = cwListRetx;
				currHarq->markSelected(signal, schedulingGrant_->getUserTxParams()->getLayers().size());
				rtxSignalised = false;
				//resetSchedulingGrant();
			}
		}
		// if no retx is needed, proceed with normal scheduling
		if (!retx) {
			scheduleListWithSizes_ = *lcgScheduler_->schedule();
			requestedSdus_ = macSduRequest();

			if (requestedSdus_ == 0) {
				// no data to send, but if bsrTriggered is set, send a BSR
				macPduMake();
			}

		}

		// Message that triggers flushing of Tx H-ARQ buffers for all users
		// This way, flushing is performed after the (possible) reception of new MAC PDUs
		cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
		flushHarqMsg->setSchedulingPriority(1);        // after other messages
		scheduleAt(NOW, flushHarqMsg);

	}

	unsigned int purged = 0;
// purge from corrupted PDUs all Rx H-HARQ buffers
	for (hit = harqRxBuffers_.begin(); hit != het; ++hit) {
		purged += hit->second->purgeCorruptedPdus();
	}
//EV << NOW << " NRMacUe::handleSelfMessage Purged " << purged << " PDUS" << endl;

	if (requestedSdus_ == 0) {
		// update current harq process id
		currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
	}

//EV << "--- END UE MAIN LOOP ---" << endl;

//std::cout << "NRMacUe handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void NRMacUe::handleSelfMessageWithNRHarq() {
//std::cout << "NRMacUe handleSelfMessageWithNRHarq start at " << simTime().dbl() << std::endl;

//EV << "----- UE MAIN LOOP -----" << endl;

// extract pdus from all harqrxbuffers and pass them to unmaker
	HarqRxBuffers::iterator hit = harqRxBuffers_.begin();
	HarqRxBuffers::iterator het = harqRxBuffers_.end();
	LteMacPdu *pdu = NULL;
	std::list<LteMacPdu*> pduList;

	for (; hit != het; ++hit) {
		pduList = hit->second->extractCorrectPdus();
		while (!pduList.empty()) {
			pdu = pduList.front();
			pduList.pop_front();
			macPduUnmake(pdu);
		}
	}

//EV << NOW << "NRMacUe::handleSelfMessageWithNRHarq " << nodeId_ << " - HARQ process " << (unsigned int) currentHarq_ << endl;

	if (schedulingGrant_ == NULL) {
		//EV << NOW << " NRMacUe::handleSelfMessageWithNRHarq " << nodeId_ << " NO configured grant" << endl;
		checkRAC();

	}

	requestedSdus_ = 0;
	if (schedulingGrant_ != NULL) // if a grant is configured
	{
		if (!firstTx) {
			//EV << "\t currentHarq_ counter initialized " << endl;
			firstTx = true;
			currentHarq_ = harqProcesses_ - 2;
		}

		bool retx = false;

		HarqTxBuffers::iterator it2;
		LteHarqBufferTx *currHarq;

		for (it2 = harqTxBuffers_.begin(); it2 != harqTxBuffers_.end(); it2++) {
			//EV << "\t Looking for retx in acid " << (unsigned int) currentHarq_ << endl;
			currHarq = it2->second;

			//check whether a process needs a rtx
			unsigned short rtxProcessNumber = 0;
			for (unsigned short i = currentHarq_; i < harqProcessesNR_; i++) {
				retx = currHarq->getProcess(i)->hasReadyUnits();
				if (retx) {
					rtxProcessNumber = i;
					break;
				}
			}
			if (!retx) {
				for (unsigned short i = 0; i < currentHarq_; i++) {
					retx = currHarq->getProcess(i)->hasReadyUnits();
					if (retx) {
						rtxProcessNumber = i;
						break;
					}
				}
			}
			CwList cwListRetx = currHarq->getProcess(rtxProcessNumber)->readyUnitsIds();

			//EV << "\t [process=" << (unsigned int) currentHarq_ << "] , [retx=" << ((retx) ? "true" : "false") << "] , [n=" << cwListRetx.size() << "]" << endl;

			// if a retransmission is needed
			if (retx) {
				UnitList signal;
				signal.first = rtxProcessNumber;
				signal.second = cwListRetx;
				currHarq->markSelected(signal, schedulingGrant_->getUserTxParams()->getLayers().size());
				rtxSignalised = false;
			}
		}
		// if no retx is needed, proceed with normal scheduling
		if (!retx) {
			scheduleListWithSizes_ = *lcgScheduler_->schedule();
			requestedSdus_ = macSduRequest();

			if (requestedSdus_ == 0) {
				// no data to send, but if bsrTriggered is set, send a BSR
				macPduMake();
			}

		}

		// Message that triggers flushing of Tx H-ARQ buffers for all users
		// This way, flushing is performed after the (possible) reception of new MAC PDUs
		cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
		flushHarqMsg->setSchedulingPriority(1);        // after other messages
		scheduleAt(NOW, flushHarqMsg);

	}

	unsigned int purged = 0;
// purge from corrupted PDUs all Rx H-HARQ buffers
	for (hit = harqRxBuffers_.begin(); hit != het; ++hit) {
		purged += hit->second->purgeCorruptedPdus();
	}
//EV << NOW << " NRMacUe::handleSelfMessage Purged " << purged << " PDUS" << endl;

	if (requestedSdus_ == 0) {
		// update current harq process id
		currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
	}

//EV << "--- END UE MAIN LOOP ---" << endl;

//std::cout << "NRMacUe handleSelfMessageWithNRHarq end at " << simTime().dbl() << std::endl;
}

void NRMacUe::macPduMake(MacCid cid) {

//std::cout << "NRMacUe macPduMake start at " << simTime().dbl() << std::endl;

	int64 size = 0;

	macPduList_.clear();
	MacCid destCid;
	Codeword cw;

//  Build a MAC pdu for each scheduled user on each codeword
	LteMacScheduleListWithSizes::const_iterator it;
	for (it = scheduleListWithSizes_.begin(); it != scheduleListWithSizes_.end(); it++) {
		LteMacPdu *macPkt;
		cPacket *pkt;

		//maybe the packet from rlc has not arrived at mac layer yet
		if (mbuf_.find(it->first.first) == mbuf_.end()) {
			continue;
		}
		//

		destCid = it->first.first;
		cw = it->first.second;

		// from an UE perspective, the destId is always the one of the eNb
		MacNodeId destId = getMacCellId();

		std::pair<MacCid, Codeword> pktId = std::pair<MacCid, Codeword>(destCid, cw);
		unsigned int sduPerCid = it->second.first;

		MacPduList::iterator pit = macPduList_.find(pktId);

		if (sduPerCid == 0 && !bsrTriggered_) {
			continue;
		}

		// No packets for this user on this codeword
		if (pit == macPduList_.end()) {
			//std::pair<unsigned int, unsigned short> key(destCid, mbuf_[destCid]->front()->getKind());
			UserControlInfo *uinfo = new UserControlInfo();
			uinfo->setSourceId(getMacNodeId());
			uinfo->setDestId(destId);
			uinfo->setDirection(UL);
			uinfo->setUserTxParams(schedulingGrant_->getUserTxParams()->dup());
			uinfo->setCid(destCid);
			uinfo->setQfi(qosHandler->getQosInfo()[destCid].qfi);
			uinfo->setRadioBearerId(qosHandler->getQosInfo()[destCid].radioBearerId);
			uinfo->setApplication(qosHandler->getQosInfo()[destCid].appType);
			uinfo->setLcid(qosHandler->getQosInfo()[destCid].lcid);
			uinfo->setRlcType(qosHandler->getQosInfo()[destCid].rlcType);
			uinfo->setTraffic(qosHandler->getQosInfo()[destCid].trafficClass);
			macPkt = new LteMacPdu("LteMacPdu");
			macPkt->setHeaderLength(MAC_HEADER);
			macPkt->setControlInfo(uinfo);
			macPkt->setTimestamp(NOW);
			macPduList_[pktId] = macPkt;

		} else {
			macPkt = pit->second;
		}

		while (sduPerCid > 0) {
			// Add SDU to PDU
			// Find Mac Pkt

			if (mbuf_.find(destCid) == mbuf_.end()) {
				throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);
			}

			if (mbuf_[destCid]->isEmpty()) {
				throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);
			}

			pkt = mbuf_[destCid]->popFront(); // LteRlcUmDataPdu
			drop(pkt);
			macPkt->pushSdu(pkt);
			sduPerCid--;
		}

		// consider virtual buffers to compute BSR size
		if (macBuffers_.find(destCid) == macBuffers_.end()) {
			for (auto &var : macBuffers_) {
				if (var.first == destCid) {
					size = size + var.second->getQueueOccupancy();
				}
			}
		} else {
			size = size + macBuffers_[destCid]->getQueueOccupancy();
		}
	}

//  Put MAC pdus in H-ARQ buffers

	MacPduList::iterator pit;
	for (pit = macPduList_.begin(); pit != macPduList_.end(); pit++) {
		MacCid destCid = pit->first.first;
		MacNodeId enbId = cellId_;
		Codeword cw = pit->first.second;

		LteHarqBufferTx *txBuf;
		HarqTxBuffers::iterator hit = harqTxBuffers_.find(enbId);
		if (hit != harqTxBuffers_.end()) {
			// the tx buffer already exists
			txBuf = hit->second;
		} else {
			// the tx buffer does not exist yet for this mac node id, create one
			// FIXME: hb is never deleted
			LteHarqBufferTx *hb = new LteHarqBufferTx((unsigned int) harqProcesses_, this, (LteMacBase*) getMacByMacNodeId(cellId_));
			harqTxBuffers_[enbId] = hb;
			txBuf = hb;
		}

		// search for an empty unit within current harq process
		UnitList txList = txBuf->getEmptyUnits(currentHarq_);
		//EV << "NRMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "] , [curr=" << (unsigned int)currentHarq_ << "]" << endl;

		LteMacPdu *macPkt = pit->second;

		if (bsrTriggered_) {
			MacBsr *bsr = new MacBsr();
			bsr->setTimestamp(simTime().dbl());
			bsr->setSize(size);
			macPkt->pushCe(bsr);
			bsrTriggered_ = false;
			//EV << "NRMacUe::macPduMake - BSR with size " << size << "created" << endl;
		}

		//EV << "NRMacUe: pduMaker created PDU: " << macPkt->info() << endl;

		// txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
		if (txList.second.empty()) {
			//EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
			delete macPkt;
		} else {
			if (getSystemModule()->par("nrHarq").boolValue()) {
				UnitList temp = txBuf->firstAvailable();
				txBuf->insertPdu(temp.first, cw, macPkt);
			} else {
				txBuf->insertPdu(txList.first, cw, macPkt);
			}
		}
	}
//std::cout << "NRMacUe macPduMake end at " << simTime().dbl() << std::endl;
}

void NRMacUe::handleUpperMessage(cPacket *pkt) {
//std::cout << "NRMacUe handleUpperMessage start at " << simTime().dbl() << std::endl;

    //bufferize packet
	bufferizePacket(pkt);

	//true, if in bufferizePacket a macBufferOverflow was detected
	if(pkt->hasBitError()){
		delete pkt;
		return;
	}

	if (strcmp(pkt->getName(), "lteRlcFragment") == 0) {
		// new MAC SDU has been received
		if (pkt->getByteLength() == 0) {
			delete pkt;
		}

		// build a MAC PDU only after all MAC SDUs have been received from RLC
		requestedSdus_--;
		if (requestedSdus_ == 0) {
			macPduMake();
			// update current harq process id
			//EV << NOW << " NRMacUe::handleMessage - incrementing counter for HARQ processes " << (unsigned int) currentHarq_ << " --> " << (currentHarq_ + 1) % harqProcesses_ << endl;
			currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
		}
	} else {
		delete pkt;
	}

//std::cout << "NRMacUe handleUpperMessage end at " << simTime().dbl() << std::endl;
}

bool NRMacUe::bufferizePacket(cPacket *pkt) {

//std::cout << "NRMacUe bufferizePacket start at " << simTime().dbl() << std::endl;

	LteRlcUmDataPdu *rlcPdu;
	pkt->setTimestamp();        // Add timestamp with current time to packet
	FlowControlInfo *lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

	MacCid cid = lteInfo->getCid();
	if (pkt->getByteLength() == 0) {
		try {
			rlcPdu = check_and_cast<LteRlcUmDataPdu*>(pkt);
			if (rlcPdu->getRequest().size() > 1)
				cid = idToMacCid(lteInfo->getSourceId(), 0);
			else
				cid = lteInfo->getCid();
		} catch (...) {
			pkt->setBitError(true);
			return false;
		}
	}

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

			//EV << "NRMacUe : Using new buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Bytes in the Queue: " << vqueue->getQueueOccupancy() << "\n";
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
			//EV << "NRMacUe : Using old buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << vqueue->getQueueOccupancy() << "\n";
		}

		return true;    // notify the activation of the connection
	}

// this is a MAC SDU, bufferize it in the MAC buffer
	LteMacBuffers::iterator it;
	it = mbuf_.find(cid);
	if (it == mbuf_.end()) {
		// Queue not found for this cid: create
		LteMacQueue *queue = new LteMacQueue(queueSize_);

		queue->pushBack(pkt);

		mbuf_[cid] = queue;

		//EV << "NRMacUe : Using new buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << queue->getQueueSize() - queue->getByteLength() << "\n";
	} else {
		LteMacQueue *queue = it->second;
		if (!queue->pushBack(pkt)) {
			// packet queue full or we have discarded fragments for this main packet
			totalOverflowedBytes_ += pkt->getByteLength();
			double sample = (double) totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
			if (lteInfo->getDirection() == DL) {
				emit(macBufferOverflowDl_, sample);
			} else {
				emit(macBufferOverflowUl_, sample);
			}
			pkt->setBitError(true);
		}

		//EV << "NRMacUe : Using old buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << "(cid: " << cid << "), Space left in the Queue: " << queue->getQueueSize() - queue->getByteLength() << "\n";
	}

//std::cout << "NRMacUe bufferizePacket end at " << simTime().dbl() << std::endl;

	return false; // do not need to notify the activation of the connection (already done when received newDataPkt)
}

void NRMacUe::flushHarqBuffers() {

//std::cout << "NRMacUe flushHarqBuffers start at " << simTime().dbl() << std::endl;

// send the selected units to lower layers
	HarqTxBuffers::iterator it2;
	for (it2 = harqTxBuffers_.begin(); it2 != harqTxBuffers_.end(); it2++)
		it2->second->sendSelectedDown();

// deleting non-periodic grant
	if (schedulingGrant_ != NULL && !schedulingGrant_->getPeriodic() && !getRtxSignalised()) {
		resetSchedulingGrant();
	}

//std::cout << "NRMacUe flushHarqBuffers end at " << simTime().dbl() << std::endl;
}

void NRMacUe::checkRAC() {
//std::cout << "NRMacUe checkRAC start at " << simTime().dbl() << std::endl;

//EV << NOW << " NRMacUe::checkRAC , Ue  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_ << ", raRespTimer:" << raRespTimer_ << endl;

// to be set in omnetpp.ini --> if true, the ue does not send a rac request when the last transmission failed (a HARQNACK arrived)
// a new rac request will be sent after a successfull rtx
	if (rtxSignalisedFlagEnabled) {
		if (rtxSignalised) {
			return;
		}
	}

	if (!getSystemModule()->par("newTxbeforeRtx").boolValue()) {
		if (getSystemModule()->par("nrHarq").boolValue()) {
			bool retx = false;

			HarqTxBuffers::iterator it2;
			LteHarqBufferTx *currHarq;
			for (it2 = harqTxBuffers_.begin(); it2 != harqTxBuffers_.end(); it2++) {
				//EV << "\t Looking for retx in acid " << (unsigned int) currentHarq_ << endl;
				currHarq = it2->second;

				retx = currHarq->getProcess(currentHarq_)->hasReadyUnits();

				// if a retransmission is needed
				if (retx) {
					return;
				}
			}
		}
	}

	if (racBackoffTimer_ > 0) {
		racBackoffTimer_--;
		return;
	}

	if (raRespTimer_ > 0) {
		// decrease RAC response timer
		raRespTimer_--;
		//EV << NOW << " NRMacUe::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
		return;
	}

//     Avoids double requests whithin same TTI window
	if (racRequested_) {
		//EV << NOW << " NRMacUe::checkRAC - double RAC request" << endl;
		racRequested_ = false;
		return;
	}

	bool trigger = false;

	LteMacBufferMap::const_iterator it;
	unsigned int bytesize = 0;
	for (it = macBuffers_.begin(); it != macBuffers_.end(); ++it) {
		if (!(it->second->isEmpty())) {
			MacCid cid = it->first;
			bytesize += it->second->getQueueOccupancy();
			trigger = true;
		}
	}

	if ((racRequested_ = trigger)) {

		LteRac *racReq = new LteRac("RacRequest");
		UserControlInfo *uinfo = new UserControlInfo();
		uinfo->setSourceId(nodeId_);
		uinfo->setDestId(getMacCellId());
		uinfo->setDirection(UL);
		uinfo->setFrameType(RACPKT);
		uinfo->setBytesize(bytesize);

		QosInfo tmp = qosHandler->getQosInfo()[macBuffers_.begin()->first];
		uinfo->setLcid(tmp.lcid);
		uinfo->setCid(idToMacCid(nodeId_, 0));
		uinfo->setQfi(tmp.qfi);
		uinfo->setRadioBearerId(tmp.radioBearerId);
		uinfo->setApplication(tmp.appType);
		uinfo->setBytesize(bytesize);
		racReq->setControlInfo(uinfo);
		racReq->setKind(tmp.appType);

		if (!isRtxSignalisedEnabled()) {
			if (!firstTx) {
				racRequests_.insert(harqProcesses_ - 2);
			} else {
				if (getSystemModule()->par("nrHarq").boolValue()) {
					racRequests_.insert(currentHarq_);
				} else {
					racRequests_.insert((currentHarq_ + 4) % harqProcesses_);
				}
			}
		}

		sendLowerPackets(racReq);

		//EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY " << endl;

		// wait at least  "raRespWinStart_" TTIs before another RAC request
		raRespTimer_ = raRespWinStart_;
	}
//std::cout << "NRMacUe checkRAC end at " << simTime().dbl() << std::endl;
}

