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

#include "nr/stack/mac/layer/NRMacUE.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/common/TimeTag_m.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/rlc/packet/LteRlcSdu_m.h"

Define_Module(NRMacUE);

void NRMacUE::initialize(int stage)
{

    //LteMacBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        /* Gates initialization */
        up_[IN_GATE] = gate("RLC_to_MAC");
        up_[OUT_GATE] = gate("MAC_to_RLC");
        down_[IN_GATE] = gate("PHY_to_MAC");
        down_[OUT_GATE] = gate("MAC_to_PHY");

        /* Create buffers */
        queueSize_ = par("queueSize");

        /* Get reference to binder */
        binder_ = getNRBinder();

        /* Set The MAC MIB */

        muMimo_ = par("muMimo");

        harqProcesses_ = par("harqProcesses");

        /* statistics */
        statDisplay_ = par("statDisplay");

        totalOverflowedBytes_ = 0;
        nrFromUpper_ = 0;
        nrFromLower_ = 0;
        nrToUpper_ = 0;
        nrToLower_ = 0;

        /* register signals */
        macBufferOverflowDl_ = registerSignal("macBufferOverFlowDl");
        macBufferOverflowUl_ = registerSignal("macBufferOverFlowUl");
        if (isD2DCapable())
            macBufferOverflowD2D_ = registerSignal("macBufferOverFlowD2D");

        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

        measuredItbs_ = registerSignal("measuredItbs");
        WATCH(queueSize_);
        WATCH(nodeId_);
        WATCH_MAP(mbuf_);
        WATCH_MAP(macBuffers_);
    }

    //LteMacUe::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        cqiDlMuMimo0_ = registerSignal("cqiDlMuMimo0");
        cqiDlMuMimo1_ = registerSignal("cqiDlMuMimo1");
        cqiDlMuMimo2_ = registerSignal("cqiDlMuMimo2");
        cqiDlMuMimo3_ = registerSignal("cqiDlMuMimo3");
        cqiDlMuMimo4_ = registerSignal("cqiDlMuMimo4");

        cqiDlTxDiv0_ = registerSignal("cqiDlTxDiv0");
        cqiDlTxDiv1_ = registerSignal("cqiDlTxDiv1");
        cqiDlTxDiv2_ = registerSignal("cqiDlTxDiv2");
        cqiDlTxDiv3_ = registerSignal("cqiDlTxDiv3");
        cqiDlTxDiv4_ = registerSignal("cqiDlTxDiv4");

        cqiDlSpmux0_ = registerSignal("cqiDlSpmux0");
        cqiDlSpmux1_ = registerSignal("cqiDlSpmux1");
        cqiDlSpmux2_ = registerSignal("cqiDlSpmux2");
        cqiDlSpmux3_ = registerSignal("cqiDlSpmux3");
        cqiDlSpmux4_ = registerSignal("cqiDlSpmux4");

        cqiDlSiso0_ = registerSignal("cqiDlSiso0");
        cqiDlSiso1_ = registerSignal("cqiDlSiso1");
        cqiDlSiso2_ = registerSignal("cqiDlSiso2");
        cqiDlSiso3_ = registerSignal("cqiDlSiso3");
        cqiDlSiso4_ = registerSignal("cqiDlSiso4");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cellId_ = getAncestorPar("masterId");
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        nodeId_ = getAncestorPar("macNodeId");

        /* Insert UeInfo in the Binder */
        UeInfo *info = new UeInfo();
        info->id = nodeId_;            // local mac ID
        info->cellId = cellId_;        // cell ID
        info->init = false;            // flag for phy initialization
        info->ue = this->getParentModule()->getParentModule(); // reference to the UE module
        info->phy = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

        phy_ = info->phy;

        binder_->addUeInfo(info);

        if (cellId_ > 0) {
            LteAmc *amc = check_and_cast<LteMacEnb*>(getMacByMacNodeId(cellId_))->getAmc();
            amc->attachUser(nodeId_, UL);
            amc->attachUser(nodeId_, DL);
        }

        // find interface entry and use its address
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        // TODO: how do we find the LTE interface?
        InterfaceEntry *interfaceEntry = interfaceTable->findInterfaceByName("wlan");
        if (interfaceEntry == nullptr)
            throw new cRuntimeError("no interface entry for lte interface - cannot bind node %i", nodeId_);

        inet::Ipv4InterfaceData *ipv4if = interfaceEntry->getProtocolData<inet::Ipv4InterfaceData>();
        if (ipv4if == nullptr)
            throw new cRuntimeError("no Ipv4 interface data - cannot bind node %i", nodeId_);
        binder_->setMacNodeId(ipv4if->getIPAddress(), nodeId_);

        // Register the "ext" interface, if present
        if (hasPar("enableExtInterface") && getAncestorPar("enableExtInterface").boolValue()) {
            // get address of the localhost to enable forwarding
            Ipv4Address extHostAddress = Ipv4Address(getAncestorPar("extHostAddress").stringValue());
            binder_->setMacNodeId(extHostAddress, nodeId_);
        }
    }
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER) {
        const std::map<double, LteChannelModel*> *channelModels = phy_->getChannelModels();
        std::map<double, LteChannelModel*>::const_iterator it = channelModels->begin();
        for (; it != channelModels->end(); ++it) {
            lcgScheduler_[it->first] = new NRSchedulerUeUl(this, it->first);
        }
    }
    else if (stage == inet::INITSTAGE_LAST) {
        /* Start TTI tick */
        // the period is equal to the minimum period according to the numerologies used by the carriers in this node
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);     // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(binder_->getUeMaxNumerologyIndex(nodeId_));
        scheduleAt(NOW + ttiPeriod_, ttiTick_);

        const std::set<NumerologyIndex> *numerologyIndexSet = binder_->getUeNumerologyIndex(nodeId_);
        if (numerologyIndexSet != NULL) {
            std::set<NumerologyIndex>::const_iterator it = numerologyIndexSet->begin();
            for (; it != numerologyIndexSet->end(); ++it) {
                // set periodicity for this carrier according to its numerology
                NumerologyPeriodCounter info;
                info.max = 1 << (binder_->getUeMaxNumerologyIndex(nodeId_) - *it); // 2^(maxNumerologyIndex - numerologyIndex)
                info.current = info.max - 1;
                numerologyPeriodCounter_[*it] = info;
            }
        }
        useQosModel = getSimulation()->getSystemModule()->par("useQosModel").boolValue();
    }

    //NRMacUe::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        rcvdD2DModeSwitchNotification_ = registerSignal("rcvdD2DModeSwitchNotification");
    }
    if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        // get parameters
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");

        if (cellId_ > 0) {
            preconfiguredTxParams_ = getPreconfiguredTxParams();

            // get the reference to the eNB
            enb_ = check_and_cast<LteMacEnbD2D*>(getMacByMacNodeId(cellId_));

            LteAmc *amc = check_and_cast<LteMacEnb*>(
                    getSimulation()->getModule(binder_->getOmnetId(cellId_))->getSubmodule("cellularNic")->getSubmodule(
                            "mac"))->getAmc();
            amc->attachUser(nodeId_, D2D);
        }
        else
            enb_ = NULL;
    }

    //local
    if (stage == inet::INITSTAGE_LOCAL) {

        qosHandler = check_and_cast<QosHandlerUE*>(getParentModule()->getSubmodule("qosHandler"));

        harqProcesses_ = getSystemModule()->par("numberHarqProcesses").intValue();
        harqProcessesNR_ = getSystemModule()->par("numberHarqProcessesNR").intValue();
        if (getSystemModule()->par("nrHarq").boolValue()) {
            harqProcesses_ = harqProcessesNR_;
            raRespWinStart_ = getSystemModule()->par("raRespWinStartNR").intValue();
        }
    }
}

void NRMacUE::macHandleGrant(cPacket *pktAux)
{
    //std::cout << "NRMacUe macHandleGrant start at " << simTime().dbl() << std::endl;

//    NRMacUe::macHandleGrant(pkt);

    // extract grant
    auto pkt = check_and_cast<inet::Packet*>(pktAux);
    auto grant = pkt->popAtFront<LteSchedulingGrant>();

    auto userInfo = pkt->getTag<UserControlInfo>();
    double carrierFrequency = userInfo->getCarrierFrequency();

    // delete old grant
    if (schedulingGrant_.find(carrierFrequency) != schedulingGrant_.end()
            && schedulingGrant_[carrierFrequency] != nullptr) {
        schedulingGrant_[carrierFrequency] = nullptr;
    }

    // store received grant
    schedulingGrant_[carrierFrequency] = grant;

    if (grant->getPeriodic()) {
        periodCounter_[carrierFrequency] = grant->getPeriod();
        expirationCounter_[carrierFrequency] = grant->getExpiration();
    }

    // clearing pending RAC requests
    racRequested_ = false;
    racD2DMulticastRequested_ = false;

    delete pkt;

//std::cout << "NRMacUe macHandleGrant start at " << simTime().dbl() << std::endl;
}

void NRMacUE::handleMessage(omnetpp::cMessage *msg)
{

//std::cout << "NRMacUe::handleMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacUe::handleMessage(msg);

//std::cout << "NRMacUe::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacUE::fromPhy(omnetpp::cPacket *pktAux)
{

//std::cout << "NRMacUe::fromPhy start at " << simTime().dbl() << std::endl;

    auto pkt = check_and_cast<inet::Packet*>(pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();

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
            tmp.lastUpdate = NOW;
            qosHandler->getQosInfo()[userInfo->getCid()] = tmp;
        }
    }

    //TODO check if code has to be overwritten
    NRMacUe::fromPhy(pkt);

//std::cout << "NRMacUe::fromPhy end at " << simTime().dbl() << std::endl;
}

int NRMacUE::macSduRequest()
{

//std::cout << "NRMacUe macSduRequest start at " << simTime().dbl() << std::endl;

//std::cout << "NRMacUe macSduRequest end at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    return NRMacUe::macSduRequest();

}

void NRMacUE::macPduUnmake(cPacket *pkt)
{

    //TODO check if code has to be overwritten
    NRMacUe::macPduUnmake(pkt);

}

void NRMacUE::handleSelfMessage()
{
//std::cout << "NRMacUe handleSelfMessage start at " << simTime().dbl() << std::endl;

    if (useQosModel) {
        // extract pdus from all harqrxbuffers and pass them to unmaker
        std::map<double, HarqRxBuffers>::iterator mit = harqRxBuffers_.begin();
        std::map<double, HarqRxBuffers>::iterator met = harqRxBuffers_.end();
        for (; mit != met; mit++) {
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit->first)) > 0)
                continue;

            HarqRxBuffers::iterator hit = mit->second.begin();
            HarqRxBuffers::iterator het = mit->second.end();

            std::list<Packet*> pduList;
            for (; hit != het; ++hit) {
                pduList = hit->second->extractCorrectPdus();
                while (!pduList.empty()) {
                    auto pdu = pduList.front();
                    pduList.pop_front();
                    macPduUnmake(pdu);
                }
            }
        }

        // no grant available - if user has backlogged data, it will trigger scheduling request
        // no harq counter is updated since no transmission is sent.

        bool noSchedulingGrants = true;
        auto git = schedulingGrant_.begin();
        auto get = schedulingGrant_.end();
        for (; git != get; ++git) {
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
                continue;

            if (git->second != NULL)
                noSchedulingGrants = false;
        }

        if (noSchedulingGrants) {
            checkRAC();
            // TODO ensure all operations done  before return ( i.e. move H-ARQ rx purge before this point)
        }
//        else {
//            bool periodicGrant = false;
//            bool checkRac = false;
//            bool skip = false;
//            for (git = schedulingGrant_.begin(); git != get; ++git) {
//                if (git->second != NULL && git->second->getPeriodic()) {
//                    periodicGrant = true;
//                    double carrierFreq = git->first;
//
//                    // Periodic checks
//                    if (--expirationCounter_[carrierFreq] < 0) {
//                        // Periodic grant is expired
//                        git->second = nullptr;
//                        checkRac = true;
//                    }
//                    else if (--periodCounter_[carrierFreq] > 0) {
//                        skip = true;
//                    }
//                    else {
//                        // resetting grant period
//                        periodCounter_[carrierFreq] = git->second->getPeriod();
//                        // this is periodic grant TTI - continue with frame sending
//                        checkRac = false;
//                        skip = false;
//                        break;
//                    }
//                }
//            }
//            if (periodicGrant) {
//                if (checkRac)
//                    checkRAC();
//                else {
//                    if (skip)
//                        return;
//                }
//            }
//        }

        scheduleList_.clear();
        requestedSdus_ = 0;
        if (!noSchedulingGrants) // if a grant is configured
        {
            bool retx = false;

            HarqTxBuffers::iterator it2;
            LteHarqBufferTx *currHarq;
            std::map<double, HarqTxBuffers>::iterator mtit;
            bool flag = false;

            for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit) {
                double carrierFrequency = mtit->first;

                if (!schedulingGrant_[carrierFrequency]->getNewTx()) {

                    // skip if no grant is configured for this carrier
                    if (schedulingGrant_.find(carrierFrequency)
                            == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == NULL)
                        continue;

                    // skip if this is not the turn of this carrier
                    if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                        continue;

                    for (it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++) {
                        currHarq = it2->second;
                        unsigned int numProcesses = currHarq->getNumProcesses();

                        for (unsigned int proc = 0; proc < numProcesses; proc++) {
                            LteHarqProcessTx *currProc = currHarq->getProcess(proc);

                            // check if the current process has unit ready for retx
                            bool ready = currProc->hasReadyUnits();
                            CwList cwListRetx = currProc->readyUnitsIds();

                            if (!ready)
                                continue;

                            // check if one 'ready' unit has the same direction of the grant
                            bool checkDir = false;
                            CwList::iterator cit = cwListRetx.begin();
//                            auto lteInfo = schedulingGrant_[carrierFrequency]->getTag<UserControlInfo>();
//                            MacCid cid = idToMacCid(nodeId_, lteInfo->getLcid());
                            auto packet = currHarq->getProcess(proc)->getPdu(0);
                            auto lteInfo = packet->getTag<UserControlInfo>();
                            auto pdu = packet->peekAtFront<LteMacPdu>();
                            MacCid cid = pdu->getMacCid();

                            for (; cit != cwListRetx.end(); ++cit) {
                                Codeword cw = *cit;
                                auto info = currProc->getPdu(cw)->getTag<UserControlInfo>();
                                if (info->getDirection() == schedulingGrant_[carrierFrequency]->getDirection()) {
                                    checkDir = true;
                                    break;
                                }
                            }

                            // if a retransmission is needed
                            if (ready && checkDir) {

                                if (lteInfo->getCid() == cid) {
                                    //found the correct one

                                    UnitList signal;
                                    signal.first = proc;
                                    signal.second = cwListRetx;
                                    currHarq->markSelected(signal,
                                            schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                                    retx = true;
                                    flag = true;

                                }
                            }
                            if (flag)
                                break;
                        }
                        if (flag)
                            break;
                    }
                    if (flag)
                        break;
                }
            }

            // if no retx is needed, proceed with normal scheduling
            if (!retx) {
                emptyScheduleList_ = true;
                std::map<double, LteSchedulerUeUl*>::iterator sit;
                for (sit = lcgScheduler_.begin(); sit != lcgScheduler_.end(); ++sit) {
                    double carrierFrequency = sit->first;

                    // skip if this is not the turn of this carrier
                    if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                        continue;

                    LteSchedulerUeUl *carrierLcgScheduler = sit->second;

                    LteMacScheduleList *carrierScheduleList = carrierLcgScheduler->schedule();

                    scheduleList_[carrierFrequency] = carrierScheduleList;
                    if (!carrierScheduleList->empty())
                        emptyScheduleList_ = false;
                }

                if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && emptyScheduleList_) {
                    // no connection scheduled, but we can use this grant to send a BSR to the eNB
                    macPduMake();
                }
                else {
                    requestedSdus_ = macSduRequest(); // returns an integer
                }

            }

            // Message that triggers flushing of Tx H-ARQ buffers for all users
            // This way, flushing is performed after the (possible) reception of new MAC PDUs
            cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
            flushHarqMsg->setSchedulingPriority(1);        // after other messages
            scheduleAt(NOW, flushHarqMsg);
        }

        decreaseNumerologyPeriodCounter();

    }
    else {
        NRMacUe::handleSelfMessage();
    }

//std::cout << "NRMacUe handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void NRMacUE::macPduMake(MacCid cid)
{

//std::cout << "NRMacUe macPduMake start at " << simTime().dbl() << std::endl;

//    NRMacUe::macPduMake(cid);

    int64 size = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = false;
    // UE is in D2D-mode but it received an UL grant (for BSR)
    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git) {
        double carrierFreq = git->first;

        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (git->second != nullptr && git->second->getDirection() == UL && emptyScheduleList_) {
            if (bsrTriggered_ || bsrD2DMulticastTriggered_) {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                LteMacBufferMap::const_iterator itbsr;
                for (itbsr = macBuffers_.begin(); itbsr != macBuffers_.end(); itbsr++) {
                    MacCid cid = itbsr->first;
                    Direction connDir = (Direction) connDesc_[cid].getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (bsrTriggered_ && connDir != D2D)
                        continue;
                    if (bsrD2DMulticastTriggered_ && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += itbsr->second->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0) {
                        if (connDesc_[cid].getRlcType() == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (connDesc_[cid].getRlcType() == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0) {
                    // Call the appropriate function for make a BSR for a D2D communication
                    Packet *macPktBsr = makeBsr(sizeBsr);
                    auto info = macPktBsr->getTag<UserControlInfo>();

                    if (info != NULL) {
                        info->setCarrierFrequency(carrierFreq);
                        info->setUserTxParams(git->second->getUserTxParams()->dup());
                        if (bsrD2DMulticastTriggered_) {
                            info->setLcid(D2D_MULTI_SHORT_BSR);
                            bsrD2DMulticastTriggered_ = false;
                        }
                        else
                            info->setLcid(D2D_SHORT_BSR);
                    }

                    // Add the created BSR to the PDU List
                    if (macPktBsr != NULL) {
                        LteChannelModel *channelModel = phy_->getChannelModel();
                        if (channelModel == NULL)
                            throw cRuntimeError("NRMacUe::macPduMake - channel model is a null pointer. Abort.");
                        else
                            macPduList_[channelModel->getCarrierFrequency()][std::pair<MacNodeId, Codeword>(
                                    getMacCellId(), 0)] = macPktBsr;
                        bsrAlreadyMade = true;
                        EV << "NRMacUe::macPduMake - BSR D2D created with size " << sizeBsr << "created" << endl;
                    }
                }
                else {
                    bsrD2DMulticastTriggered_ = false;
                    bsrTriggered_ = false;
                }
            }
            break;
        }
    }

    if (!bsrAlreadyMade) {
        // In a D2D communication if BSR was created above this part isn't executed
        // Build a MAC PDU for each scheduled user on each codeword
        std::map<double, LteMacScheduleList*>::iterator cit = scheduleList_.begin();
        for (; cit != scheduleList_.end(); ++cit) {
            double carrierFreq = cit->first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
                continue;

            LteMacScheduleList::const_iterator it;
            for (it = cit->second->begin(); it != cit->second->end(); it++) {
                Packet *macPkt;

                MacCid destCid = it->first.first;
                Codeword cw = it->first.second;

                // get the direction (UL/D2D/D2D_MULTI) and the corresponding destination ID
                FlowControlInfo *lteInfo = &(connDesc_.at(destCid));
                MacNodeId destId = lteInfo->getDestId();
                Direction dir = (Direction) lteInfo->getDirection();

                std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
                unsigned int sduPerCid = it->second;

                if (sduPerCid == 0 && !bsrTriggered_ && !bsrD2DMulticastTriggered_)
                    continue;

                if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                MacPduList::iterator pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                if (pit == macPduList_[carrierFreq].end()) {
                    // Create a PDU
                    macPkt = new Packet("LteMacPdu");
                    auto header = makeShared<LteMacPdu>();
                    header->setHeaderLength(MAC_HEADER);
                    header->setMacCid(destCid);
                    header->setMacLcid(lteInfo->getLcid());
                    macPkt->insertAtFront(header);

                    macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(dir);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCid(destCid);
//                        macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(SHORT_BSR));
                    macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(lteInfo->getLcid());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                    if (usePreconfiguredTxParams_)
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(preconfiguredTxParams_->dup());
                    else
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(
                                schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else {
                    // Never goes here because of the macPduList_.clear() at the beginning
                    macPkt = pit->second;
                }

                while (sduPerCid > 0) {
                    // Add SDU to PDU
                    // Find Mac Pkt
                    if (mbuf_.find(destCid) == mbuf_.end())
                        throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);

                    if (mbuf_[destCid]->isEmpty())
                        throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);

                    auto pkt = check_and_cast<Packet*>(mbuf_[destCid]->popFront());

                    if (pkt != NULL) {
                        // multicast support
                        // this trick gets the group ID from the MAC SDU and sets it in the MAC PDU
                        auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);
                        if (infoVec.empty())
                            throw cRuntimeError("No tag of type LteControlInfo found");

                        int32 groupId = infoVec.front().getMulticastGroupId();
                        if (groupId >= 0) // for unicast, group id is -1
                            macPkt->getTag<UserControlInfo>()->setMulticastGroupId(groupId);

                        drop(pkt);

                        auto header = macPkt->removeAtFront<LteMacPdu>();
                        header->pushSdu(pkt);
                        macPkt->insertAtFront(header);
                        sduPerCid--;
                    }
                    else
                        throw cRuntimeError("NRMacUe::macPduMake - extracted SDU is NULL. Abort.");
                }

                // consider virtual buffers to compute BSR size
                size += macBuffers_[destCid]->getQueueOccupancy();

                if (size > 0) {
                    // take into account the RLC header size
                    if (connDesc_[destCid].getRlcType() == UM)
                        size += RLC_HEADER_UM;
                    else if (connDesc_[destCid].getRlcType() == AM)
                        size += RLC_HEADER_AM;
                }
            }
        }
    }

    // Put MAC PDUs in H-ARQ buffers
    std::map<double, MacPduList>::iterator lit;
    for (lit = macPduList_.begin(); lit != macPduList_.end(); ++lit) {
        double carrierFreq = lit->first;
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers &harqTxBuffers = harqTxBuffers_[carrierFreq];

        MacPduList::iterator pit;
        for (pit = lit->second.begin(); pit != lit->second.end(); pit++) {
            MacNodeId destId = pit->first.first;
            Codeword cw = pit->first.second;
            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx *txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end()) {
                // The tx buffer already exists
                txBuf = hit->second;
            }
            else {
                // The tx buffer does not exist yet for this mac node id, create one
                LteHarqBufferTx *hb;
                // FIXME: hb is never deleted
                auto info = pit->second->getTag<UserControlInfo>();
                if (info->getDirection() == UL) {
                    //                    hb = new LteHarqBufferTx((unsigned int) ENB_TX_HARQ_PROCESSES, this, (LteMacBase*) getMacByMacNodeId(destId));
                    hb = new LteHarqBufferTx(harqProcesses_, this, (LteMacBase*) getMacByMacNodeId(destId));
                }
                else {                // D2D or D2D_MULTI
                    hb = new LteHarqBufferTxD2D((unsigned int) ENB_TX_HARQ_PROCESSES, this,
                            (LteMacBase*) getMacByMacNodeId(destId));
                }
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the first available process
            UnitList txList = txBuf->firstAvailable();
            EV << "NRMacUe::macPduMake - [Used Acid=" << (unsigned int) txList.first << "]" << endl;

            //Get a reference of the LteMacPdu from pit pointer (extract Pdu from the MAP)
            auto macPkt = pit->second;

            auto header = macPkt->removeAtFront<LteMacPdu>();
            // Attach BSR to PDU if RAC is won and wasn't already made
            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && !bsrAlreadyMade) {
                MacBsr *bsr = new MacBsr();
                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);
                bsrTriggered_ = false;
                bsrD2DMulticastTriggered_ = false;
                EV << "NRMacUe::macPduMake - BSR created with size " << size << endl;
            }

            macPkt->insertAtFront(header);

            EV << "NRMacUe: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty()) {
                EV << "NRMacUe() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else {
                //Insert PDU in the Harq Tx Buffer
                //txList.first is the acid
                txBuf->insertPdu(txList.first, cw, macPkt);
            }
        }
    }

//std::cout << "NRMacUe macPduMake end at " << simTime().dbl() << std::endl;
}

void NRMacUE::handleUpperMessage(cPacket *pkt)
{
//std::cout << "NRMacUe handleUpperMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacUe::handleUpperMessage(pkt);

//std::cout << "NRMacUe handleUpperMessage end at " << simTime().dbl() << std::endl;
}

bool NRMacUE::bufferizePacket(omnetpp::cPacket *pktAux)
{

    //std::cout << "NRMacUe bufferizePacket start at " << simTime().dbl() << std::endl;

    //return NRMacUe::bufferizePacket(pkt);
    auto pkt = check_and_cast<Packet*>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet - should not be buffered
        delete pkt;
        return false;
    }

    pkt->setTimestamp();           // add time-stamp with current time to packet

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (checkIfHeaderType<LteRlcPduNewData>(pkt)) {
        // update the virtual buffer for this connection

        // build the virtual packet corresponding to this incoming packet
        pkt->popAtFront<LteRlcPduNewData>();
        auto rlcSdu = pkt->peekAtFront<LteRlcSdu>();
        PacketInfo vpkt(rlcSdu->getLengthMainPacket(), pkt->getTimestamp());

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

        }
        else {
            LteMacBuffer *vqueue = NULL;
            LteMacBufferMap::iterator it = macBuffers_.find(cid);
            if (it != macBuffers_.end())
                vqueue = it->second;

            if (vqueue != NULL) {
                vqueue->pushBack(vpkt);

            }
            else {
                throw cRuntimeError("LteMacUe::bufferizePacket - cannot find mac buffer for cid %d", cid);
            }

            //simplified Flow Control
            if (getSimulation()->getSystemModule()->par("useSimplifiedFlowControl").boolValue()) {
                if (macBuffers_[cid]->getQueueOccupancy() > (queueSize_ / 8)) {
                    getNRBinder()->setQueueStatus(MacCidToNodeId(cid), lteInfo->getDirection(),
                            lteInfo->getApplication(), true);
                }
                else {
                    getNRBinder()->setQueueStatus(MacCidToNodeId(cid), lteInfo->getDirection(),
                            lteInfo->getApplication(), false);
                }
            }
            //

        }

        delete pkt;
        return true;    // notify the activation of the connection
    }

    // this is a MAC SDU, bufferize it in the MAC buffer

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end()) {
        // Queue not found for this cid: create
        LteMacQueue *queue = new LteMacQueue(queueSize_);

        queue->pushBack(pkt);

        mbuf_[cid] = queue;

    }
    else {
        // Found
        LteMacQueue *queue = it->second;
        if (!queue->pushBack(pkt)) {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double) totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection() == DL) {
                emit(macBufferOverflowDl_, sample);
            }
            else {
                emit(macBufferOverflowUl_, sample);
            }

            delete pkt;
            return false;
        }

        //simplified Flow Control --> to ensure the packet flow continues
        if (getSimulation()->getSystemModule()->par("useSimplifiedFlowControl").boolValue()) {
            if (macBuffers_[cid]->getQueueOccupancy() > (queueSize_ / 4 / 8)) {
                getNRBinder()->setQueueStatus(MacCidToNodeId(cid), lteInfo->getDirection(), lteInfo->getApplication(),
                        true);
            }
            else {
                getNRBinder()->setQueueStatus(MacCidToNodeId(cid), lteInfo->getDirection(), lteInfo->getApplication(),
                        false);
            }
        }
        //
    }

    return true;

}

void NRMacUE::flushHarqBuffers()
{

//std::cout << "NRMacUe flushHarqBuffers start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacUe::flushHarqBuffers();

//std::cout << "NRMacUe flushHarqBuffers end at " << simTime().dbl() << std::endl;
}

void NRMacUE::checkRAC()
{
//std::cout << "NRMacUe checkRAC start at " << simTime().dbl() << std::endl;

    //check if QosModel is activated
    if (useQosModel) {
        checkRACQoSModel();
        return;
    }
    else {
        //NRMacUe::checkRAC();
        if (racBackoffTimer_ > 0) {
            racBackoffTimer_--;
            return;
        }

        if (raRespTimer_ > 0) {
            // decrease RAC response timer
            raRespTimer_--;

            return;
        }

        // Avoids double requests whithin same TTI window
        if (racRequested_) {
            racRequested_ = false;
            return;
        }
        if (racD2DMulticastRequested_) {
            racD2DMulticastRequested_ = false;
            return;
        }

        bool trigger = false;
        bool triggerD2DMulticast = false;
        unsigned int bytesize = 0;
        MacCid cid;

        LteMacBufferMap::const_iterator it;

        for (it = macBuffers_.begin(); it != macBuffers_.end(); ++it) {
            if (!(it->second->isEmpty())) {
                cid = it->first;
                bytesize = it->second->getQueueOccupancy();
                if (connDesc_.at(cid).getDirection() == D2D_MULTI)
                    triggerD2DMulticast = true;
                else
                    trigger = true;
                break;
            }
        }

        if ((racRequested_ = trigger) || (racD2DMulticastRequested_ = triggerD2DMulticast)) {
            auto pkt = new Packet("RacRequest");
            pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
            pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
            pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
            pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

            //pkt->addTagIfAbsent<UserControlInfo>()->setBytesize(bytesize);
            QosInfo tmp = qosHandler->getQosInfo()[cid];
            pkt->addTagIfAbsent<UserControlInfo>()->setLcid(tmp.lcid);
            //pkt->addTagIfAbsent<UserControlInfo>()->setCid(idToMacCid(nodeId_, 0));

            pkt->addTagIfAbsent<UserControlInfo>()->setCid(idToMacCid(nodeId_, tmp.lcid));

            pkt->addTagIfAbsent<UserControlInfo>()->setQfi(tmp.qfi);
            pkt->addTagIfAbsent<UserControlInfo>()->setRadioBearerId(tmp.radioBearerId);
            pkt->addTagIfAbsent<UserControlInfo>()->setApplication(tmp.appType);
            pkt->addTagIfAbsent<UserControlInfo>()->setTraffic(tmp.trafficClass);
            pkt->addTagIfAbsent<UserControlInfo>()->setRlcType(tmp.rlcType);
            pkt->addTagIfAbsent<UserControlInfo>()->setBytesize(bytesize);

            auto racReq = makeShared<LteRac>();

            pkt->insertAtFront(racReq);
            sendLowerPackets(pkt);

            // wait at least  "raRespWinStart_" TTIs before another RAC request
            raRespTimer_ = raRespWinStart_;
        }
    }

//std::cout << "NRMacUe checkRAC end at " << simTime().dbl() << std::endl;
}

void NRMacUE::checkRACQoSModel()
{

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
    MacCid cid;
    simtime_t creationTimeOfQueueFront;
    unsigned int sizeOfOnePacketInBytes;

    //retrieve the lambdaValues from the ini file
    double lambdaPriority = getSimulation()->getSystemModule()->par("lambdaPriority").doubleValue();
    double lambdaRemainDelayBudget = getSimulation()->getSystemModule()->par("lambdaRemainDelayBudget").doubleValue();
    //double lambdaCqi = getSimulation()->getSystemModule()->par("lambdaCqi").doubleValue();
    double lambdaByteSize = getSimulation()->getSystemModule()->par("lambdaByteSize").doubleValue();
    double lambdaRtx = getSimulation()->getSystemModule()->par("lambdaRtx").doubleValue();

    std::map<double, std::vector<ScheduleInfo>> combinedMap;

    //const UserTxParams &txParams = (check_and_cast<LteMacEnb*>(getMacByMacNodeId(cellId_)))->getAmc()->computeTxParams(nodeId_, UL);
    simtime_t remainDelayBudget;

    //
    std::map<double, std::vector<QosInfo>> qosinfosMap;
    //
    qosinfosMap = getQosHandler()->getEqualPriorityMap(UL);
    for (auto &var : qosinfosMap) {
        for (auto &qosinfo : var.second) {
            if (!macBuffers_[qosinfo.cid]->isEmpty()) {

                cid = qosinfo.cid;
                simtime_t creationTimeFirstPacket = macBuffers_[cid]->getHolTimestamp();
                ASSERT(creationTimeFirstPacket != 0);

                ScheduleInfo tmp;
                tmp.category = "newTx";
                tmp.cid = cid;
                tmp.pdb = getQosHandler()->getPdb(getQosHandler()->get5Qi(getQosHandler()->getQfi(cid)));
                tmp.priority = getQosHandler()->getPriority(getQosHandler()->get5Qi(getQosHandler()->getQfi(cid)));
                tmp.numberRtx = 0;
                tmp.nodeId = MacCidToNodeId(tmp.cid);
                tmp.bytesizeUL = macBuffers_[cid]->getQueueOccupancy();
                tmp.sizeOnePacketUL = macBuffers_[cid]->front().first;

                simtime_t delay = NOW - creationTimeFirstPacket;
                remainDelayBudget = tmp.pdb - delay;
                tmp.remainDelayBudget = remainDelayBudget;

                //calculate the weight to find the macBufferCid with the highest priority

                double calcPrio = lambdaPriority * (tmp.priority / 90.0)
                        + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget)
                        //+ lambdaCqi * (1 / (txParams.readCqiVector().at(0) + 1))
                        + (lambdaRtx * (1 / (1 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketUL) * lambdaByteSize);

                combinedMap[calcPrio].push_back(tmp);
            }
        }
    }

    //choose the first entry
    for (auto &var : combinedMap) {
        for (auto &vec : var.second) {
            cid = vec.cid;
            bytesize = vec.bytesizeUL;
            creationTimeOfQueueFront = macBuffers_[cid]->getHolTimestamp();
            sizeOfOnePacketInBytes = vec.sizeOnePacketUL;

            trigger = true;
            break;
        }
        if (trigger)
            break;
    }

    if ((racRequested_ = trigger)) {

        auto pkt = new Packet("RacRequest");
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        pkt->addTagIfAbsent<UserControlInfo>()->setBytesize(bytesize);
        QosInfo tmp = qosHandler->getQosInfo()[cid];
        pkt->addTagIfAbsent<UserControlInfo>()->setLcid(tmp.lcid);
        //pkt->addTagIfAbsent<UserControlInfo>()->setCid(idToMacCid(nodeId_, 0));

        pkt->addTagIfAbsent<UserControlInfo>()->setCid(idToMacCid(nodeId_, tmp.lcid));

        pkt->addTagIfAbsent<UserControlInfo>()->setQfi(tmp.qfi);
        pkt->addTagIfAbsent<UserControlInfo>()->setRadioBearerId(tmp.radioBearerId);
        pkt->addTagIfAbsent<UserControlInfo>()->setApplication(tmp.appType);
        pkt->addTagIfAbsent<UserControlInfo>()->setTraffic(tmp.trafficClass);
        pkt->addTagIfAbsent<UserControlInfo>()->setRlcType(tmp.rlcType);
        pkt->addTagIfAbsent<UserControlInfo>()->setBytesize(bytesize);
        //
        pkt->addTagIfAbsent<UserControlInfo>()->setCreationTimeOfQueueFront(creationTimeOfQueueFront);
        pkt->addTagIfAbsent<UserControlInfo>()->setBytesizeOfOnePacket(sizeOfOnePacketInBytes);

        auto racReq = makeShared<LteRac>();
        pkt->insertAtFront(racReq);

        sendLowerPackets(pkt);

        //EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY " << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        raRespTimer_ = raRespWinStart_;
    }
}

