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

#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"

/**
 * TODO:
 * - rimuovere i commenti dalle funzioni quando saranno implementate nel enb scheduler
 */

void LteScheduler::setEnbScheduler(LteSchedulerEnb* eNbScheduler)
{
    eNbScheduler_ = eNbScheduler;
    direction_ = eNbScheduler_->direction_;
    mac_ = eNbScheduler_->mac_;
    initializeGrants();
}


unsigned int LteScheduler::requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible , std::vector<BandLimit>* bandLim)
{
    //std::cout << "LteScheduler::requestGrant  at " << simTime().dbl() << std::endl;

    return eNbScheduler_->scheduleGrant(cid, bytes, terminate, active, eligible ,bandLim);
}


bool LteScheduler::scheduleRetransmissions()
{
    //std::cout << "LteScheduler::scheduleRetransmissions  at " << simTime().dbl() << std::endl;

    return eNbScheduler_->rtxschedule();
}

void LteScheduler::scheduleRacRequests()
{
    //std::cout << "LteScheduler::scheduleRacRequests  at " << simTime().dbl() << std::endl;

    //return (dynamic_cast<LteSchedulerEnbUl*>(eNbScheduler_))->serveRacs();
}

void LteScheduler::requestRacGrant(MacNodeId nodeId)
{
    //std::cout << "LteScheduler::requestRacGrant  at " << simTime().dbl() << std::endl;

    //return (dynamic_cast<LteSchedulerEnbUl*>(eNbScheduler_))->racGrantEnb(nodeId);
}

void LteScheduler::schedule()
{
    //std::cout << "LteScheduler::schedule start at " << simTime().dbl() << std::endl;

    prepareSchedule();
    commitSchedule();

    //std::cout << "LteScheduler::schedule end at " << simTime().dbl() << std::endl;
}

void LteScheduler::initializeGrants()
{
    //std::cout << "LteScheduler::initializeGrants start at " << simTime().dbl() << std::endl;

    if (direction_ == DL)
    {
        grantTypeMap_[CONVERSATIONAL] = aToGrantType(mac_->par("grantTypeConversationalDl"));
        grantTypeMap_[STREAMING] = aToGrantType(mac_->par("grantTypeStreamingDl"));
        grantTypeMap_[INTERACTIVE] = aToGrantType(mac_->par("grantTypeInteractiveDl"));
        grantTypeMap_[BACKGROUND] = aToGrantType(mac_->par("grantTypeBackgroundDl"));

        grantSizeMap_[CONVERSATIONAL] = mac_->par("grantSizeConversationalDl");
        grantSizeMap_[STREAMING] = mac_->par("grantSizeStreamingDl");
        grantSizeMap_[INTERACTIVE] = mac_->par("grantSizeInteractiveDl");
        grantSizeMap_[BACKGROUND] = mac_->par("grantSizeBackgroundDl");
    }
    else if (direction_ == UL)
    {
        grantTypeMap_[CONVERSATIONAL] = aToGrantType(mac_->par("grantTypeConversationalUl"));
        grantTypeMap_[STREAMING] = aToGrantType(mac_->par("grantTypeStreamingUl"));
        grantTypeMap_[INTERACTIVE] = aToGrantType(mac_->par("grantTypeInteractiveUl"));
        grantTypeMap_[BACKGROUND] = aToGrantType(mac_->par("grantTypeBackgroundUl"));

        grantSizeMap_[CONVERSATIONAL] = mac_->par("grantSizeConversationalUl");
        grantSizeMap_[STREAMING] = mac_->par("grantSizeStreamingUl");
        grantSizeMap_[INTERACTIVE] = mac_->par("grantSizeInteractiveUl");
        grantSizeMap_[BACKGROUND] = mac_->par("grantSizeBackgroundUl");
    }
    else
    {
        throw cRuntimeError("Unknown direction %d", direction_);
    }

    //std::cout << "LteScheduler::initializeGrants end at " << simTime().dbl() << std::endl;
}
