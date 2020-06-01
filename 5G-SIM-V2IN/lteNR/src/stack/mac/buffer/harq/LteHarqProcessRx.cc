//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified for 5G-Sim-V2I/N
//

#include "stack/mac/buffer/harq/LteHarqProcessRx.h"
#include "stack/mac/layer/LteMacBase.h"
#include "common/LteControlInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/packet/LteMacPdu.h"

LteHarqProcessRx::LteHarqProcessRx(unsigned char acid, LteMacBase *owner)
{
    pdu_.resize(MAX_CODEWORDS, NULL);
    status_.resize(MAX_CODEWORDS, RXHARQ_PDU_EMPTY);
    rxTime_.resize(MAX_CODEWORDS, 0);
    result_.resize(MAX_CODEWORDS, false);
    acid_ = acid;
    macOwner_ = owner;
    transmissions_ = 0;
    maxHarqRtx_ = owner->par("maxHarqRtx");
}

void LteHarqProcessRx::insertPdu(Codeword cw, LteMacPdu *pdu)
{

    //std::cout << "LteHarqProcessRx::insertPdu start at " << simTime().dbl() << std::endl;

    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(pdu->getControlInfo());
    bool ndi = lteInfo->getNdi();

    //EV << "LteHarqProcessRx::insertPdu - ndi is " << ndi << endl;
    if (ndi && !(status_.at(cw) == RXHARQ_PDU_EMPTY))
        throw cRuntimeError("New data arriving in busy harq process -- this should not happen");

    if (!ndi && !(status_.at(cw) == RXHARQ_PDU_EMPTY) && !(status_.at(cw) == RXHARQ_PDU_CORRUPTED))
        throw cRuntimeError(
            "Trying to insert macPdu in non-empty rx harq process: Node %d acid %d, codeword %d, ndi %d, status %d",
            macOwner_->getMacNodeId(), acid_, cw, ndi, status_.at(cw));

    // deallocate corrupted pdu received in previous transmissions
    if (pdu_.at(cw) != NULL){
            macOwner_->dropObj(pdu_.at(cw));
            delete pdu_.at(cw);
    }

    // store new received pdu
    pdu_.at(cw) = pdu;
    result_.at(cw) = lteInfo->getDeciderResult();
    status_.at(cw) = RXHARQ_PDU_EVALUATING;
    rxTime_.at(cw) = NOW;

    transmissions_++;

    //std::cout << "LteHarqProcessRx::insertPdu start at " << simTime().dbl() << std::endl;
}

bool LteHarqProcessRx::isEvaluated(Codeword cw)
{
    //std::cout << "LteHarqProcessRx::isEvaluated start at " << simTime().dbl() << std::endl;

    if (status_.at(cw) == RXHARQ_PDU_EVALUATING && (NOW - rxTime_.at(cw)) >= HARQ_FB_EVALUATION_INTERVAL)
        return true;
    else
        return false;
}

LteHarqFeedback *LteHarqProcessRx::createFeedback(Codeword cw)
{

    //std::cout << "LteHarqProcessRx::createFeedback start at " << simTime().dbl() << std::endl;

    if (!isEvaluated(cw))
        throw cRuntimeError("Cannot send feedback for a pdu not in EVALUATING state");

    UserControlInfo *pduInfo = check_and_cast<UserControlInfo *>(pdu_.at(cw)->getControlInfo());
    LteHarqFeedback *fb = new LteHarqFeedback();
    fb->setAcid(acid_);
    fb->setCw(cw);
    fb->setResult(result_.at(cw));
    fb->setFbMacPduId(pdu_.at(cw)->getMacPduId());
    fb->setByteLength(0);
    UserControlInfo *fbInfo = new UserControlInfo();
    fbInfo->setSourceId(pduInfo->getDestId());
    fbInfo->setDestId(pduInfo->getSourceId());
    fbInfo->setFrameType(HARQPKT);
    fbInfo->setApplication(pduInfo->getApplication());
    fbInfo->setQfi(pduInfo->getQfi());
    fbInfo->setRadioBearerId(pduInfo->getRadioBearerId());
    fbInfo->setTraffic(pduInfo->getTraffic());
    fbInfo->setRlcType(pduInfo->getRlcType());
    fbInfo->setLcid(pduInfo->getLcid());
    fb->setControlInfo(fbInfo);

    if (!result_.at(cw))
    {
        // NACK will be sent
        status_.at(cw) = RXHARQ_PDU_CORRUPTED;

        //EV << "LteHarqProcessRx::createFeedback - tx number " << (unsigned int)transmissions_ << endl;
        if (transmissions_ == (maxHarqRtx_ + 1))
        {
            //EV << NOW << " LteHarqProcessRx::createFeedback - max number of tx reached for cw " << cw << ". Resetting cw" << endl;

            // purge PDU
            purgeCorruptedPdu(cw);
            resetCodeword(cw);
        }
    }
    else
    {
        status_.at(cw) = RXHARQ_PDU_CORRECT;
    }

    //std::cout << "LteHarqProcessRx::createFeedback end at " << simTime().dbl() << std::endl;

    return fb;
}

bool LteHarqProcessRx::isCorrect(Codeword cw)
{
    //std::cout << "LteHarqProcessRx::isCorrect start at " << simTime().dbl() << std::endl;

    return (status_.at(cw) == RXHARQ_PDU_CORRECT);
}

LteMacPdu* LteHarqProcessRx::extractPdu(Codeword cw)
{
    //std::cout << "LteHarqProcessRx::extractPdu start at " << simTime().dbl() << std::endl;

    if (!isCorrect(cw))
        throw cRuntimeError("Cannot extract pdu if the state is not CORRECT");

    // temporary copy of pdu pointer because reset NULLs it, and I need to return it
    LteMacPdu *pdu = pdu_.at(cw);
    pdu_.at(cw) = NULL;
    resetCodeword(cw);
    return pdu;
}

int64_t LteHarqProcessRx::getByteLength(Codeword cw)
{
    //std::cout << "LteHarqProcessRx::getByteLength start at " << simTime().dbl() << std::endl;

    if (pdu_.at(cw) != NULL)
    {
        return pdu_.at(cw)->getByteLength();
    }
    else
        return 0;
}

void LteHarqProcessRx::purgeCorruptedPdu(Codeword cw)
{
    //std::cout << "LteHarqProcessRx::purgeCorruptedPdu start at " << simTime().dbl() << std::endl;

    // drop ownership
    if (pdu_.at(cw) != NULL)
        macOwner_->dropObj(pdu_.at(cw));

    delete pdu_.at(cw);
    pdu_.at(cw) = NULL;

    //std::cout << "LteHarqProcessRx::purgeCorruptedPdu end at " << simTime().dbl() << std::endl;
}

void LteHarqProcessRx::resetCodeword(Codeword cw)
{
    //std::cout << "LteHarqProcessRx::resetCodeword start at " << simTime().dbl() << std::endl;

    // drop ownership
    if (pdu_.at(cw) != NULL){
        macOwner_->dropObj(pdu_.at(cw));
        delete pdu_.at(cw);
    }

    pdu_.at(cw) = NULL;
    status_.at(cw) = RXHARQ_PDU_EMPTY;
    rxTime_.at(cw) = 0;
    result_.at(cw) = false;

    transmissions_ = 0;

    //std::cout << "LteHarqProcessRx::resetCodeword end at " << simTime().dbl() << std::endl;
}

LteHarqProcessRx::~LteHarqProcessRx()
{
    for (unsigned char i = 0; i < MAX_CODEWORDS; ++i)
    {
        if (pdu_.at(i) != NULL)
        {
            cObject *mac = macOwner_;
            if (pdu_.at(i)->getOwner() == mac)
            {
                delete pdu_.at(i);
            }
            pdu_.at(i) = NULL;
        }
    }
}

std::vector<RxUnitStatus>
LteHarqProcessRx::getProcessStatus()
{
    //std::cout << "LteHarqProcessRx::getProcessStatus start at " << simTime().dbl() << std::endl;

    std::vector<RxUnitStatus> ret(MAX_CODEWORDS);

    for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
    {
        ret[j].first = j;
        ret[j].second = getUnitStatus(j);
    }
    return ret;
}
