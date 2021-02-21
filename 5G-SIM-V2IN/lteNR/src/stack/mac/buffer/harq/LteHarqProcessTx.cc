//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq/LteHarqProcessTx.h"

LteHarqProcessTx::LteHarqProcessTx(unsigned char acid, unsigned int numUnits, unsigned int numProcesses,
    LteMacBase *macOwner, LteMacBase *dstMac)
{
    macOwner_ = macOwner;
    acid_ = acid;
    numHarqUnits_ = numUnits;
    units_ = new UnitVector(numUnits);
    numProcesses_ = numProcesses;
    numEmptyUnits_ = numUnits; //++ @ insert, -- @ unit reset (ack or fourth nack)
    numSelected_ = 0; //++ @ markSelected and insert, -- @ extract/sendDown
    dropped_ = false;

    // H-ARQ unit instances
    for (unsigned int i = 0; i < numHarqUnits_; i++)
    {
        (*units_)[i] = new LteHarqUnitTx(acid, i, macOwner_, dstMac);
    }
}

std::vector<UnitStatus>
LteHarqProcessTx::getProcessStatus()
{

    //std::cout << "LteHarqProcessTx::getProcessStatus start at " << simTime().dbl() << std::endl;

    std::vector<UnitStatus> ret(numHarqUnits_);

    for (unsigned int j = 0; j < numHarqUnits_; j++)
    {
        ret[j].first = j;
        ret[j].second = getUnitStatus(j);
    }
    return ret;
}

void LteHarqProcessTx::insertPdu(LteMacPdu *pdu, Codeword cw)
{
    //std::cout << "LteHarqProcessTx::insertPdu start at " << simTime().dbl() << std::endl;

    numEmptyUnits_--;
    numSelected_++;
    (*units_)[cw]->insertPdu(pdu);
    dropped_ = false;

    //std::cout << "LteHarqProcessTx::insertPdu end at " << simTime().dbl() << std::endl;
}

void LteHarqProcessTx::markSelected(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::markSelected start at " << simTime().dbl() << std::endl;

    if (numSelected_ == numHarqUnits_)
        throw cRuntimeError("H-ARQ TX process: cannot select another unit because they are all already selected");

    numSelected_++;
    (*units_)[cw]->markSelected();

    //std::cout << "LteHarqProcessTx::markSelected end at " << simTime().dbl() << std::endl;
}

LteMacPdu *LteHarqProcessTx::extractPdu(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::extractPdu start at " << simTime().dbl() << std::endl;

    if (numSelected_ == 0)
        throw cRuntimeError("H-ARQ TX process: cannot extract pdu: numSelected = 0 ");

    numSelected_--;
    LteMacPdu *pdu = (*units_)[cw]->extractPdu();

    //std::cout << "LteHarqProcessTx::extractPdu end at " << simTime().dbl() << std::endl;

    return pdu;
}

bool LteHarqProcessTx::pduFeedback(HarqAcknowledgment fb, Codeword cw)
{

    //std::cout << "LteHarqProcessTx::pduFeedback start at " << simTime().dbl() << std::endl;

    // controllare se numempty == numunits e restituire true/false
    bool reset = (*units_)[cw]->pduFeedback(fb);

    if (reset)
    {
        numEmptyUnits_++;
    }

    // return true if the process has become empty
    if (numEmptyUnits_ == numHarqUnits_)
        reset = true;
    else
        reset = false;

    //std::cout << "LteHarqProcessTx::pduFeedback end at " << simTime().dbl() << std::endl;

    return reset;
}

bool LteHarqProcessTx::selfNack(Codeword cw)
{

    //std::cout << "LteHarqProcessTx::selfNack start at " << simTime().dbl() << std::endl;

    bool reset = (*units_)[cw]->selfNack();

    if (reset)
    {
        numEmptyUnits_++;
    }

    // return true if the process has become empty
    if (numEmptyUnits_ == numHarqUnits_)
        reset = true;
    else
        reset = false;

    //std::cout << "LteHarqProcessTx::selfNack end at " << simTime().dbl() << std::endl;

    return reset;
}

bool LteHarqProcessTx::hasReadyUnits()
{

    //std::cout << "LteHarqProcessTx::hasReadyUnits start at " << simTime().dbl() << std::endl;

    for (unsigned int i = 0; i < numHarqUnits_; i++)
    {
        if ((*units_)[i]->isReady())
            return true;
    }

    //std::cout << "LteHarqProcessTx::hasReadyUnits end at " << simTime().dbl() << std::endl;

    return false;
}

simtime_t LteHarqProcessTx::getOldestUnitTxTime()
{
    //std::cout << "LteHarqProcessTx::getOldestUnitTxTime start at " << simTime().dbl() << std::endl;

    simtime_t oldestTxTime = NOW + 1;
    simtime_t curTxTime = 0;
    for (unsigned int i = 0; i < numHarqUnits_; i++)
    {
        if ((*units_)[i]->isReady())
        {
            curTxTime = (*units_)[i]->getTxTime();
            if (curTxTime < oldestTxTime)
            {
                oldestTxTime = curTxTime;
            }
        }
    }

    //std::cout << "LteHarqProcessTx::getOldestUnitTxTime end at " << simTime().dbl() << std::endl;

    return oldestTxTime;
}

CwList LteHarqProcessTx::readyUnitsIds()
{

    //std::cout << "LteHarqProcessTx::readyUnitsIds start at " << simTime().dbl() << std::endl;

    CwList ul;

    for (Codeword i = 0; i < numHarqUnits_; i++)
    {
        if ((*units_)[i]->isReady())
        {
            ul.push_back(i);
        }
    }

    //std::cout << "LteHarqProcessTx::readyUnitsIds end at " << simTime().dbl() << std::endl;

    return ul;
}

CwList LteHarqProcessTx::emptyUnitsIds()
{

    //std::cout << "LteHarqProcessTx::emptyUnitsIds start at " << simTime().dbl() << std::endl;

    CwList ul;
    for (Codeword i = 0; i < numHarqUnits_; i++)
    {
        if ((*units_)[i]->isEmpty())
        {
            ul.push_back(i);
        }
    }

    //std::cout << "LteHarqProcessTx::emptyUnitsIds end at " << simTime().dbl() << std::endl;

    return ul;
}

CwList LteHarqProcessTx::selectedUnitsIds()
{
    //std::cout << "LteHarqProcessTx::selectedUnitsIds start at " << simTime().dbl() << std::endl;

    CwList ul;
    for (Codeword i = 0; i < numHarqUnits_; i++)
    {
        if ((*units_)[i]->isMarked())
        {
            ul.push_back(i);
        }
    }

    //std::cout << "LteHarqProcessTx::selectedUnitsIds end at " << simTime().dbl() << std::endl;

    return ul;
}

bool LteHarqProcessTx::isEmpty()
{
    //std::cout << "LteHarqProcessTx::isEmpty start at " << simTime().dbl() << std::endl;

    return (numEmptyUnits_ == numHarqUnits_);
}

LteMacPdu *LteHarqProcessTx::getPdu(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::getPdu start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->getPdu();
}

long LteHarqProcessTx::getPduId(Codeword cw)
{

    //std::cout << "LteHarqProcessTx::getPduId start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->getMacPduId();
}

void LteHarqProcessTx::forceDropProcess()
{

    //std::cout << "LteHarqProcessTx::forceDropProcess start at " << simTime().dbl() << std::endl;

    for (unsigned int i = 0; i < numHarqUnits_; i++)
    {
        (*units_)[i]->forceDropUnit();
    }
    numEmptyUnits_ = numHarqUnits_;
    numSelected_ = 0;
    dropped_ = true;

    //std::cout << "LteHarqProcessTx::forceDropProcess end at " << simTime().dbl() << std::endl;
}

bool LteHarqProcessTx::forceDropUnit(Codeword cw)
{

    //std::cout << "LteHarqProcessTx::forceDropUnit start at " << simTime().dbl() << std::endl;

    if ((*units_)[cw]->isMarked())
        numSelected_--;

    (*units_)[cw]->forceDropUnit();
    numEmptyUnits_++;

    //std::cout << "LteHarqProcessTx::forceDropUnit end at " << simTime().dbl() << std::endl;

    // empty process?
    return numEmptyUnits_ == numHarqUnits_;
}

TxHarqPduStatus LteHarqProcessTx::getUnitStatus(Codeword cw)
{

    //std::cout << "LteHarqProcessTx::getUnitStatus start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->getStatus();
}

void LteHarqProcessTx::dropPdu(Codeword cw)
{

    //std::cout << "LteHarqProcessTx::dropPdu start at " << simTime().dbl() << std::endl;

    (*units_)[cw]->dropPdu();
    numEmptyUnits_++;
}

bool LteHarqProcessTx::isUnitEmpty(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::isUnitEmpty start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->isEmpty();
}

bool LteHarqProcessTx::isUnitReady(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::isUnitReady start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->isReady();
}

unsigned char LteHarqProcessTx::getTransmissions(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::getTransmissions start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->getTransmissions();
}

inet::int64 LteHarqProcessTx::getPduLength(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::getPduLength start at " << simTime().dbl() << std::endl;

	//for codeblockgroups
	if (getSimulation()->getSystemModule()->par("useCodeBlockGroups").boolValue()) {
		LteControlInfo *pduInfo = check_and_cast<LteControlInfo*>((*units_)[cw]->getPdu()->getControlInfo());
		if (pduInfo->getRestByteSize() == 0) {
			return (*units_)[cw]->getPduLength();
		} else {
			return pduInfo->getRestByteSize();
		}
	}

    return (*units_)[cw]->getPduLength();
}

simtime_t LteHarqProcessTx::getTxTime(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::getTxTime start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->getTxTime();
}

bool LteHarqProcessTx::isUnitMarked(Codeword cw)
{
    //std::cout << "LteHarqProcessTx::isUnitMarked start at " << simTime().dbl() << std::endl;

    return (*units_)[cw]->isMarked();
}

bool LteHarqProcessTx::isDropped()
{
    //std::cout << "LteHarqProcessTx::isDropped start at " << simTime().dbl() << std::endl;

    return dropped_;
}

LteHarqProcessTx::~LteHarqProcessTx()
{
    UnitVector::iterator it = units_->begin();
    for (; it != units_->end(); ++it)
         delete *it;

    units_->clear();
    delete units_;
    units_ = NULL;
    macOwner_ = NULL;
}
