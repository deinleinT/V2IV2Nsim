//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2020
// Author: Thomas Deinlein
//

#include "stack/mac/buffer/harq/LteHarqBufferTx.h"

LteHarqBufferTx::LteHarqBufferTx(unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac) {
	numProc_ = numProc;
	macOwner_ = owner;
	nodeId_ = dstMac->getMacNodeId();
	selectedAcid_ = HARQ_NONE;
	processes_ = new std::vector<LteHarqProcessTx*>(numProc);
	numEmptyProc_ = numProc;
	for (unsigned int i = 0; i < numProc_; i++) {
		(*processes_)[i] = new LteHarqProcessTx(i, MAX_CODEWORDS, numProc_, macOwner_, dstMac);
	}
}

UnitList LteHarqBufferTx::firstReadyForRtx() {

	//std::cout << "LteHarqBufferRx::firstReadyForRtx start at " << simTime().dbl() << std::endl;

	unsigned char oldestProcessAcid = HARQ_NONE;
	simtime_t oldestTxTime = NOW + 1;
	simtime_t currentTxTime = 0;

	for (unsigned int i = 0; i < numProc_; i++) {
		if ((*processes_)[i]->hasReadyUnits()) {
			currentTxTime = (*processes_)[i]->getOldestUnitTxTime();
			if (currentTxTime < oldestTxTime) {
				oldestTxTime = currentTxTime;
				oldestProcessAcid = i;
			}
		}
	}
	UnitList ret;
	ret.first = oldestProcessAcid;
	if (oldestProcessAcid != HARQ_NONE) {
		ret.second = (*processes_)[oldestProcessAcid]->readyUnitsIds();
	}

	//std::cout << "LteHarqBufferRx::firstReadyForRtx end at " << simTime().dbl() << std::endl;

	return ret;
}

inet::int64 LteHarqBufferTx::pduLength(unsigned char acid, Codeword cw) {
	//std::cout << "LteHarqBufferRx::pduLength  at " << simTime().dbl() << std::endl;

	return (*processes_)[acid]->getPduLength(cw);
}

void LteHarqBufferTx::markSelected(UnitList unitIds, unsigned char availableTbs) {
	//std::cout << "LteHarqBufferRx::markSelected start at " << simTime().dbl() << std::endl;

	if (unitIds.second.size() == 0) {
		//EV << "H-ARQ TX buffer: markSelected(): empty unitIds list" << endl;
		return;
	}

	if (availableTbs == 0) {
		//EV << "H-ARQ TX buffer: markSelected(): no available TBs" << endl;
		return;
	}

	unsigned char acid = unitIds.first;
	CwList cwList = unitIds.second;

	if (cwList.size() > availableTbs) {
		//eg: tx mode siso trying to rtx 2 TBs
		// this is the codeword which will contain the jumbo TB
		Codeword cw = cwList.front();
		cwList.pop_front();
		LteMacPdu *basePdu = (*processes_)[acid]->getPdu(cw);
		while (cwList.size() > 0) {
			Codeword cw = cwList.front();
			cwList.pop_front();
			LteMacPdu *guestPdu = (*processes_)[acid]->getPdu(cw);
			while (guestPdu->hasSdu())
				basePdu->pushSdu(guestPdu->popSdu());
			while (guestPdu->hasCe())
				basePdu->pushCe(guestPdu->popCe());
			(*processes_)[acid]->dropPdu(cw);
		}
		(*processes_)[acid]->markSelected(cw);
	} else {
		CwList::iterator it;
		// all units are marked
		for (it = cwList.begin(); it != cwList.end(); it++) {
			(*processes_)[acid]->markSelected(*it);
		}
	}

	selectedAcid_ = acid;

	// user tx params could have changed, modify them
	//    UserControlInfo *uInfo = check_and_cast<UserControlInfo *>(basePdu->getControlInfo());
	// TODO: get amc and modify user tx params
	//uInfo->setTxMode(???)

	// debug output
	//EV << "H-ARQ TX: process " << (int)selectedAcid_ << " has been selected for retransmission" << endl;

	//std::cout << "LteHarqBufferRx::markSelected end at " << simTime().dbl() << std::endl;
}

bool LteHarqBufferTx::isCompletelyEmpty(unsigned char acid, Codeword cw, LteMacPdu *pdu) {

	if (!(*processes_)[acid]->isEmpty()){
		return false;
		//throw cRuntimeError("H-ARQ TX buffer: new process selected for tx is not completely empty");
	}else {
		return true;
	}

}

void LteHarqBufferTx::insertPdu(unsigned char acid, Codeword cw, LteMacPdu *pdu) {
	//std::cout << "LteHarqBufferRx::insertPdu start at " << simTime().dbl() << std::endl;

	if (selectedAcid_ == HARQ_NONE) {
		// the process has not been used for rtx, or it is the first TB inserted, it must be completely empty
		if (!(*processes_)[acid]->isEmpty())
			throw cRuntimeError("H-ARQ TX buffer: new process selected for tx is not completely empty");
	}

	if (!(*processes_)[acid]->isUnitEmpty(cw))
		throw cRuntimeError("LteHarqBufferTx::insertPdu(): unit is not empty");

	selectedAcid_ = acid;
	numEmptyProc_--;
	(*processes_)[acid]->insertPdu(pdu, cw);

	// debug output
	//EV << "H-ARQ TX: new pdu (id " << pdu->getId() << " ) inserted into process " << (int)acid << " ""codeword id: " << (int)cw << " ""for node with id " << check_and_cast<UserControlInfo *>(pdu->getControlInfo())->getDestId() << endl;

	//std::cout << "LteHarqBufferRx::insertPdu end at " << simTime().dbl() << std::endl;

}

UnitList LteHarqBufferTx::firstAvailable() {

	//std::cout << "LteHarqBufferRx::firstAvailable start at " << simTime().dbl() << std::endl;

	UnitList ret;

	unsigned char acid = HARQ_NONE;

	if (selectedAcid_ == HARQ_NONE) {
		for (unsigned int i = 0; i < numProc_; i++) {
			if ((*processes_)[i]->isEmpty()) {
				acid = i;
				break;
			}
		}
	} else {
		acid = selectedAcid_;
	}
	ret.first = acid;

	if (acid != HARQ_NONE) {
		// if there is any free process, return empty list
		ret.second = (*processes_)[acid]->emptyUnitsIds();
	}

	//std::cout << "LteHarqBufferRx::firstAvailable end at " << simTime().dbl() << std::endl;

	return ret;
}

UnitList LteHarqBufferTx::getEmptyUnits(unsigned char acid) {
	//std::cout << "LteHarqBufferRx::getEmptyUnits start at " << simTime().dbl() << std::endl;

	// TODO add multi CW check and retx checks
	UnitList ret;
	ret.first = acid;
	ret.second = (*processes_)[acid]->emptyUnitsIds();

	//std::cout << "LteHarqBufferRx::getEmptyUnits end at " << simTime().dbl() << std::endl;

	return ret;
}

void LteHarqBufferTx::receiveHarqFeedback(LteHarqFeedback *fbpkt) {
	//std::cout << "LteHarqBufferRx::receiveHarqFeedback start at " << simTime().dbl() << std::endl;

	//EV << "LteHarqBufferTx::receiveHarqFeedback - start" << endl;

	bool result = fbpkt->getResult();
	HarqAcknowledgment harqResult = result ? HARQACK : HARQNACK;
	Codeword cw = fbpkt->getCw();
	unsigned char acid = fbpkt->getAcid();
	long fbPduId = fbpkt->getFbMacPduId(); // id of the pdu that should receive this fb
	long unitPduId = (*processes_)[acid]->getPduId(cw);

	//for codeblockgroups -- for testing
	LteControlInfo *info = check_and_cast<LteControlInfo*>(fbpkt->getControlInfo());
	if(info->getCodeBlockGroupsActivated()){
		LteHarqProcessTx *process = (*processes_)[acid];
		LteMacPdu *  pdu = process->getPdu(cw);
		LteControlInfo *infoPdu = check_and_cast<LteControlInfo*>(pdu->getControlInfo());
		infoPdu->setRestByteSize(info->getRestByteSize());
		infoPdu->setCodeBlockGroupsActivated(true);
		infoPdu->setNumberOfCodeBlockGroups(info->getNumberOfCodeBlockGroups());
		infoPdu->setBlocksForCodeBlockGroups(info->getBlocksForCodeBlockGroups());
		infoPdu->setInitialByteSize(info->getInitialByteSize());
	}

	// After handover or a D2D mode switch, the process nay have been dropped. The received feedback must be ignored.
	if ((*processes_)[acid]->isDropped()) {
		//EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << ". The corresponding unit has been reset after handover or a D2D mode switch (the contained pdu was dropped). Ignore feedback." << endl;
		delete fbpkt;
		//std::cout << "LteHarqBufferRx::receiveHarqFeedback end at " << simTime().dbl() << std::endl;
		return;
	}

	if (fbPduId != unitPduId) {
		// fb is not for the pdu in this unit, maybe the addressed one was dropped
		//EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << "Codeword " << cw << " not addressed to the actually contained pdu (maybe it was dropped)" << endl;
		//EV << "Received id: " << fbPduId << endl;
		//EV << "PDU id: " << unitPduId << endl;
		// todo: comment endsim after tests
		throw cRuntimeError("H-ARQ TX: fb is not for the pdu in this unit, maybe the addressed one was dropped");
	}
	bool reset = (*processes_)[acid]->pduFeedback(harqResult, cw);
	if (reset)
		numEmptyProc_++;

	// debug output
	const char *ack = result ? "ACK" : "NACK";
	//EV << "H-ARQ TX: feedback received for process " << (int)acid << " codeword " << (int)cw << "" result is " << ack << endl;

	ASSERT(fbpkt->getOwner() == this->macOwner_);
	UserControlInfo *userInfo = check_and_cast<UserControlInfo*>(fbpkt->getControlInfo());

	//added for 5G-SIM-V2I/N
	if (this->macOwner_->isRtxSignalisedEnabled()) {
		if (this->macOwner_->getNodeType() == UE) {
			this->macOwner_->getRtxSignalised() = !result;
		} else {
			this->macOwner_->setRtxSignalised(userInfo->getSourceId(), !result);
		}
	}

	//std::cout << "LteHarqBufferTx::receiveHarqFeedback end at " << simTime().dbl() << std::endl;

	delete fbpkt;
}

void LteHarqBufferTx::sendSelectedDown() {
	//std::cout << "LteHarqBufferRx::sendSelectedDown start at " << simTime().dbl() << std::endl;

	if (selectedAcid_ == HARQ_NONE) {
		//EV << "LteHarqBufferTx::sendSelectedDown - no process selected in H-ARQ buffer, nothing to do" << endl;
		return;
	}

	CwList ul = (*processes_)[selectedAcid_]->selectedUnitsIds();
	CwList::iterator it;
	for (it = ul.begin(); it != ul.end(); it++) {
		LteMacPdu *pduToSend = (*processes_)[selectedAcid_]->extractPdu(*it);
		macOwner_->sendLowerPackets(pduToSend);

		// debug output
		//EV << "\t H-ARQ TX: pdu (id " << pduToSend->getId() << " ) extracted from process " << (int)selectedAcid_ << " ""codeword " << (int)*it << " for node with id " << check_and_cast<UserControlInfo *>(pduToSend->getControlInfo())->getDestId() << endl;
	}
	selectedAcid_ = HARQ_NONE;

	//std::cout << "LteHarqBufferRx::sendSelectedDown end at " << simTime().dbl() << std::endl;
}

void LteHarqBufferTx::dropProcess(unsigned char acid) {
	//std::cout << "LteHarqBufferRx::dropProcess start at " << simTime().dbl() << std::endl;

	// pdus can be dropped only if the unit is in BUFFERED state.
	CwList ul = (*processes_)[acid]->readyUnitsIds();
	CwList::iterator it;

	for (it = ul.begin(); it != ul.end(); it++) {
		(*processes_)[acid]->dropPdu(*it);
	}
	// if a process contains units in BUFFERED state, then all units of this
	// process are either empty or in BUFFERED state (ready).
	numEmptyProc_++;

	//std::cout << "LteHarqBufferRx::dropProcess end at " << simTime().dbl() << std::endl;
}

void LteHarqBufferTx::selfNack(unsigned char acid, Codeword cw) {
	//std::cout << "LteHarqBufferRx::selfNack start at " << simTime().dbl() << std::endl;

	bool reset = false;
	CwList ul = (*processes_)[acid]->readyUnitsIds();
	CwList::iterator it;

	for (it = ul.begin(); it != ul.end(); it++) {
		reset = (*processes_)[acid]->selfNack(*it);
	}
	if (reset)
		numEmptyProc_++;

	//std::cout << "LteHarqBufferRx::selfNack end at " << simTime().dbl() << std::endl;
}

void LteHarqBufferTx::forceDropProcess(unsigned char acid) {
	//std::cout << "LteHarqBufferRx::forceDropProcess start at " << simTime().dbl() << std::endl;

	(*processes_)[acid]->forceDropProcess();
	if (acid == selectedAcid_)
		selectedAcid_ = HARQ_NONE;
	numEmptyProc_++;

	//std::cout << "LteHarqBufferRx::forceDropProcess end at " << simTime().dbl() << std::endl;
}

void LteHarqBufferTx::forceDropUnit(unsigned char acid, Codeword cw) {
	//std::cout << "LteHarqBufferRx::forceDropUnit start at " << simTime().dbl() << std::endl;

	bool reset = (*processes_)[acid]->forceDropUnit(cw);
	if (reset) {
		if (acid == selectedAcid_)
			selectedAcid_ = HARQ_NONE;
		numEmptyProc_++;
	}

	//std::cout << "LteHarqBufferRx::forceDropUnit end at " << simTime().dbl() << std::endl;
}

BufferStatus LteHarqBufferTx::getBufferStatus() {
	//std::cout << "LteHarqBufferRx::getBufferStatus start at " << simTime().dbl() << std::endl;

	BufferStatus bs(numProc_);
	unsigned int numHarqUnits = 0;
	for (unsigned int i = 0; i < numProc_; i++) {
		numHarqUnits = (*processes_)[i]->getNumHarqUnits();
		std::vector<UnitStatus> vus(numHarqUnits);
		vus = (*processes_)[i]->getProcessStatus();
		bs[i] = vus;
	}

	//std::cout << "LteHarqBufferRx::getBufferStatus end at " << simTime().dbl() << std::endl;

	return bs;
}

LteHarqProcessTx*
LteHarqBufferTx::getProcess(unsigned char acid) {
	//std::cout << "LteHarqBufferRx::getProcess start at " << simTime().dbl() << std::endl;

	try {
		//std::cout << "LteHarqBufferRx::getProcess end at " << simTime().dbl() << std::endl;
		return (*processes_).at(acid);
	} catch (std::out_of_range &x) {
		throw cRuntimeError("LteHarqBufferTx::getProcess(): acid %i out of bounds", int(acid));
	}

}

LteHarqProcessTx*
LteHarqBufferTx::getSelectedProcess() {
	//std::cout << "LteHarqBufferRx::getSelectedProcess  at " << simTime().dbl() << std::endl;
	return getProcess(selectedAcid_);
}

LteHarqBufferTx::~LteHarqBufferTx() {

	std::vector<LteHarqProcessTx*>::iterator it = processes_->begin();
	for (; it != processes_->end(); ++it)
		delete *it;

	processes_->clear();
	delete processes_;
	processes_ = NULL;
	macOwner_ = NULL;
}

bool LteHarqBufferTx::isInUnitList(unsigned char acid, Codeword cw, UnitList unitIds) {
	//std::cout << "LteHarqBufferRx::isInUnitList start at " << simTime().dbl() << std::endl;

	if (acid != unitIds.first)
		return false;

	CwList::iterator it;
	for (it = unitIds.second.begin(); it != unitIds.second.end(); it++) {
		if (cw == *it) {
			//std::cout << "LteHarqBufferRx::isInUnitList end at " << simTime().dbl() << std::endl;
			return true;
		}
	}
	//std::cout << "LteHarqBufferRx::isInUnitList end at " << simTime().dbl() << std::endl;
	return false;
}
