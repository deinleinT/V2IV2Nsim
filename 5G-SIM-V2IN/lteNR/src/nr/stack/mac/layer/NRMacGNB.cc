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

#include "nr/stack/mac/layer/NRMacGNB.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "inet/common/TimeTag_m.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/rlc/packet/LteRlcSdu_m.h"

Define_Module(NRMacGNB);

NRMacGNB::NRMacGNB() :
        NRMacGnb()
{
    scheduleListDl_ = NULL;
}

NRMacGNB::~NRMacGNB()
{
}

void NRMacGNB::initialize(int stage)
{
    //NRMacGnb::initialize(stage);

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

        qosHandler = check_and_cast<QosHandlerGNB*>(getParentModule()->getSubmodule("qosHandler"));
    }

    //LteMacEnb::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        nodeId_ = getAncestorPar("macNodeId");

        cellId_ = nodeId_;

        // TODO: read NED parameters, when will be present
        cellInfo_ = getCellInfo();

        /* Get number of antennas */
        numAntennas_ = getNumAntennas();

        //Initialize the current sub frame type with the first subframe of the MBSFN pattern
        currentSubFrameType_ = NORMAL_FRAME_TYPE;

        eNodeBCount = par("eNodeBCount");
        WATCH(numAntennas_);
        WATCH_MAP(bsrbuf_);
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT) {
        /* Create and initialize AMC module */

        amc_ = new NRAmc(this, binder_, cellInfo_, numAntennas_,
                getSimulation()->getSystemModule()->par("useExtendedMcsTable").boolValue());

        std::string modeString = par("pilotMode").stdstringValue();

        if (modeString == "AVG_CQI")
            amc_->setPilotMode(AVG_CQI);
        else if (modeString == "MAX_CQI")
            amc_->setPilotMode(MAX_CQI);
        else if (modeString == "MIN_CQI")
            amc_->setPilotMode(MIN_CQI);
        else if (modeString == "MEDIAN_CQI")
            amc_->setPilotMode(MEDIAN_CQI);
        else if (modeString == "ROBUST_CQI")
            amc_->setPilotMode(ROBUST_CQI);
        else
            throw cRuntimeError("LteMacEnb::initialize - Unknown Pilot Mode %s \n", modeString.c_str());

        /* Insert EnbInfo in the Binder */
        EnbInfo *info = new EnbInfo();
        info->id = nodeId_;            // local mac ID
        info->nodeType = nodeType_;    // eNB or gNB
        info->type = MACRO_ENB;        // eNb Type
        info->init = false;            // flag for phy initialization
        info->eNodeB = this->getParentModule()->getParentModule();  // reference to the eNodeB module

        // register the pair <id,name> to the binder
        const char *moduleName = getParentModule()->getParentModule()->getFullName();
        binder_->registerName(nodeId_, moduleName);

        // get the reference to the PHY layer
        phy_ = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

        info->phy = phy_;
        info->mac = this;
        binder_->addEnbInfo(info);

    }
    else if (stage == inet::INITSTAGE_LINK_LAYER) {
        /* Create and initialize MAC Downlink scheduler */
        if (enbSchedulerDl_ == nullptr) {
            enbSchedulerDl_ = new NRSchedulerGnbDl();
            enbSchedulerDl_->initialize(DL, this);
        }

        /* Create and initialize MAC Uplink scheduler */
        if (enbSchedulerUl_ == nullptr) {
            enbSchedulerUl_ = new NRSchedulerGnbUL();
            enbSchedulerUl_->initialize(UL, this);
        }
        harqProcesses_ = getSystemModule()->par("numberHarqProcesses").intValue();
        harqProcessesNR_ = getSystemModule()->par("numberHarqProcessesNR").intValue();
        if (getSystemModule()->par("nrHarq").boolValue()) {
            harqProcesses_ = harqProcessesNR_;
        }
    }
    else if (stage == inet::INITSTAGE_LAST) {
        /* Start TTI tick */
        // the period is equal to the minimum period according to the numerologies used by the carriers in this node
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);                                             // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(cellInfo_->getMaxNumerologyIndex());
        scheduleAt(NOW + ttiPeriod_, ttiTick_);

        const CarrierInfoMap *carriers = cellInfo_->getCarrierInfoMap();
        CarrierInfoMap::const_iterator it = carriers->begin();
        for (; it != carriers->end(); ++it) {
            // set periodicity for this carrier according to its numerology
            NumerologyPeriodCounter info;
            info.max = 1 << (cellInfo_->getMaxNumerologyIndex() - it->second.numerologyIndex); // 2^(maxNumerologyIndex - numerologyIndex)
            info.current = info.max - 1;
            numerologyPeriodCounter_[it->second.numerologyIndex] = info;
        }

        // set the periodicity for each scheduler
        enbSchedulerDl_->initializeSchedulerPeriodCounter(cellInfo_->getMaxNumerologyIndex());
        enbSchedulerUl_->initializeSchedulerPeriodCounter(cellInfo_->getMaxNumerologyIndex());

    }

    //LteMacEnbD2D::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL + 1) {
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");
        Cqi d2dCqi = par("d2dCqi");
        if (usePreconfiguredTxParams_)
            check_and_cast<AmcPilotD2D*>(amc_->getPilot())->setPreconfiguredTxParams(d2dCqi);

        msHarqInterrupt_ = par("msHarqInterrupt").boolValue();
        msClearRlcBuffer_ = par("msClearRlcBuffer").boolValue();
    }
    else if (stage == INITSTAGE_LAST)  // be sure that all UEs have been initialized
            {
        reuseD2D_ = par("reuseD2D");
        reuseD2DMulti_ = par("reuseD2DMulti");

        if (reuseD2D_ || reuseD2DMulti_) {
            conflictGraphUpdatePeriod_ = par("conflictGraphUpdatePeriod");

            CGType cgType = CG_DISTANCE;  // TODO make this parametric
            switch (cgType) {
                case CG_DISTANCE: {
                    conflictGraph_ = new DistanceBasedConflictGraph(this, reuseD2D_, reuseD2DMulti_,
                            par("conflictGraphThreshold"));
                    check_and_cast<DistanceBasedConflictGraph*>(conflictGraph_)->setThresholds(
                            par("conflictGraphD2DInterferenceRadius"), par("conflictGraphD2DMultiTxRadius"),
                            par("conflictGraphD2DMultiInterferenceRadius"));
                    break;
                }
                default: {
                    throw cRuntimeError("LteMacEnbD2D::initialize - CG type unknown. Aborting");
                }
            }

            scheduleAt(NOW + 0.05, new cMessage("updateConflictGraph"));
        }

        amc_->printTBS();
    }

}

void NRMacGNB::fromPhy(omnetpp::cPacket *pktIn)
{
    //std::cout << "NRMacGnbRealistic::fromPhy start at " << simTime().dbl() << std::endl;

    auto pkt = check_and_cast<inet::Packet*>(pktIn);
    auto userInfo = pkt->getTag<UserControlInfo>();

    if (userInfo->getFrameType() == DATAPKT) {

        if (qosHandler->getQosInfo().find(userInfo->getCid()) == qosHandler->getQosInfo().end()) {
            if (userInfo->getCid() != 0) {
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
                qosHandler->insertQosInfo(userInfo->getCid(), tmp);
            }
        }
    }

    NRMacGnb::fromPhy(pkt);

    //std::cout << "NRMacGnbRealistic::fromPhy end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::handleMessage(cMessage *msg)
{

    //std::cout << "NRMacGnbRealistic::handleMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::handleMessage(msg);

    //std::cout << "NRMacGnbRealistic::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::handleSelfMessage()
{
    //std::cout << "NRMacGnb::handleSelfMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::handleSelfMessage();

    //std::cout << "NRMacGnb::handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::sendGrants(std::map<double, LteMacScheduleList> *scheduleList)
{
    //std::cout << "LteMacEnb::sendGrants start at " << simTime().dbl() << std::endl;

    //NRMacGnb::sendGrants(scheduleList);
    std::map<double, LteMacScheduleList>::iterator cit = scheduleList->begin();
    for (; cit != scheduleList->end(); ++cit) {
        LteMacScheduleList &carrierScheduleList = cit->second;
        while (!carrierScheduleList.empty()) {
            LteMacScheduleList::iterator it, ot;
            it = carrierScheduleList.begin();

            Codeword cw = it->first.second;
            Codeword otherCw = MAX_CODEWORDS - cw;
            MacCid cid = it->first.first;
            LogicalCid lcid = MacCidToLcid(cid);
            MacNodeId nodeId = MacCidToNodeId(cid);
            unsigned int granted = it->second;
            unsigned int codewords = 0;

            //new
//            MacCid realCid = 0;
//            MacCid realLcid = 0;
//            for (auto &var : qosHandler->getAllQosInfos(nodeId)) {
//
//                //latest received rac --> contains the correct qfi
//                realCid = idToMacCid(nodeId, var.lcid);
//                realLcid = var.lcid;
//                break;
//
//            }
//            ASSERT(realCid != 0);
//            if (qosHandler->getQosInfo().find(realCid) == qosHandler->getQosInfo().end()) {
//                throw cRuntimeError("Error in NRMacGnb");
//            }
            //qosHandler->deleteCid(cid);
            //qosHandler->deleteCid(realCid);
            //

            // removing visited element from scheduleList.
            carrierScheduleList.erase(it);

            if (granted > 0) {
                // increment number of allocated Cw
                ++codewords;
            }
            else {
                // active cw becomes the "other one"
                cw = otherCw;
            }

            std::pair<unsigned int, Codeword> otherPair(nodeId, otherCw);

            if ((ot = (carrierScheduleList.find(otherPair))) != (carrierScheduleList.end())) {
                // increment number of allocated Cw
                ++codewords;

                // removing visited element from scheduleList.
                carrierScheduleList.erase(ot);
            }

            if (granted == 0)
                continue; // avoiding transmission of 0 grant (0 grant should not be created)

            // get the direction of the grant, depending on which connection has been scheduled by the eNB
            Direction dir = (lcid == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : ((lcid == D2D_SHORT_BSR) ? D2D : UL);

            // TODO Grant is set aperiodic as default

            auto pkt = new Packet("LteGrant");
            auto grant = makeShared<LteSchedulingGrant>();
            grant->setDirection(dir);
            grant->setCodewords(codewords);

            // set total granted blocks
            grant->setTotalGrantedBlocks(granted);
            grant->setChunkLength(b(1));
            grant->setMacCid(cid);

            pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
            pkt->addTagIfAbsent<UserControlInfo>()->setDestId(nodeId);
            pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(GRANTPKT);
            pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(cit->first);
            //new

            pkt->addTagIfAbsent<UserControlInfo>()->setCid(cid);
            pkt->addTagIfAbsent<UserControlInfo>()->setLcid(lcid);
            //

            //new
            if (rtxMap[nodeId].size() == 1) {
                //one rtx scheduled
                auto temp = rtxMap[nodeId];
                unsigned short lastProcess = temp.begin()->second.processId;
                grant->setProcessId(lastProcess);
                grant->setNewTx(false);
                rtxMap[nodeId].erase(temp.begin()->second.processId);
                rtxMap.erase(nodeId);
            }
            else if (rtxMap[nodeId].size() == 0) {
                //no rtx
                grant->setNewTx(true);
                grant->setProcessId(-1);

            }
            else {
                auto &temp = rtxMap[nodeId];
                unsigned short order = 17;
                for (auto &var : temp) {
                    if (var.second.order < order) {
                        order = var.second.order;
                    }
                }

                unsigned short processId = temp.at(order).processId;
                grant->setProcessId(processId);
                grant->setNewTx(false);
                rtxMap[nodeId].erase(temp.erase(order));
                //rtxMap.erase(nodeId);
            }
            //

            const UserTxParams &ui = getAmc()->computeTxParams(nodeId, dir, cit->first);
            UserTxParams *txPara = new UserTxParams(ui);
            grant->setUserTxParams(txPara);

            // acquiring remote antennas set from user info
            const std::set<Remote> &antennas = ui.readAntennaSet();
            std::set<Remote>::const_iterator antenna_it = antennas.begin(), antenna_et = antennas.end();

            // get bands for this carrier
            const unsigned int firstBand = cellInfo_->getCarrierStartingBand(cit->first);
            const unsigned int lastBand = cellInfo_->getCarrierLastBand(cit->first);

            //  HANDLE MULTICW
            for (; cw < codewords; ++cw) {
                unsigned int grantedBytes = 0;

                for (Band b = firstBand; b <= lastBand; ++b) {
                    unsigned int bandAllocatedBlocks = 0;
                    // for (; antenna_it != antenna_et; ++antenna_it) // OLD FOR
                    for (antenna_it = antennas.begin(); antenna_it != antenna_et; ++antenna_it) {
                        bandAllocatedBlocks += enbSchedulerUl_->readPerUeAllocatedBlocks(nodeId, *antenna_it, b);
                    }
                    grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw, bandAllocatedBlocks, dir, cit->first);
                }

                grant->setGrantedCwBytes(cw, grantedBytes);

            }
            RbMap map;

            enbSchedulerUl_->readRbOccupation(nodeId, cit->first, map);

            grant->setGrantedBlocks(map);
            // send grant to PHY layer
            pkt->insertAtFront(grant);
            sendLowerPackets(pkt);
        }
    }

    //std::cout << "LteMacEnb::sendGrants end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::macPduUnmake(omnetpp::cPacket *pktAux)
{
    //std::cout << "NRMacEnb::macPduUnmake start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::macPduUnmake(pktAux);

    //std::cout << "NRMacEnb::macPduUnmake end at " << simTime().dbl() << std::endl;

}

void NRMacGNB::macSduRequest()
{
    //std::cout << "NRMacGnb::macSduRequest start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::macSduRequest();

    //std::cout << "NRMacGnb::macSduRequest end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::macPduMake(MacCid cid)
{
    //std::cout << "NRMacGnb::macPduMake start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::macPduMake(cid);

    //std::cout << "NRMacGnb::macPduMake end at " << simTime().dbl() << std::endl;
}

bool NRMacGNB::bufferizePacket(cPacket *pktAux)
{
    //std::cout << "NRMacGnb::bufferizePacket start at " << simTime().dbl() << std::endl;

    //std::cout << "NRMacGnb::bufferizePacket end at " << simTime().dbl() << std::endl;

    //return NRMacGnb::bufferizePacket(pkt);

    auto pkt = check_and_cast<Packet*>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet
        delete pktAux;
        return false;
    }

    pkt->setTimestamp();        // Add timestamp with current time to packet

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

        if (qosHandler->getQosInfo().find(cid) == qosHandler->getQosInfo().end()) {
            qosHandler->getQosInfo()[cid].appType = static_cast<ApplicationType>(lteInfo->getApplication());
            qosHandler->getQosInfo()[cid].destNodeId = lteInfo->getDestId();
            qosHandler->getQosInfo()[cid].lcid = lteInfo->getLcid();
            qosHandler->getQosInfo()[cid].dir = DL;
            qosHandler->getQosInfo()[cid].qfi = lteInfo->getQfi();
            qosHandler->getQosInfo()[cid].cid = lteInfo->getCid();
            qosHandler->getQosInfo()[cid].radioBearerId = lteInfo->getRadioBearerId();
            qosHandler->getQosInfo()[cid].senderNodeId = lteInfo->getSourceId();
            qosHandler->getQosInfo()[cid].trafficClass = (LteTrafficClass) lteInfo->getTraffic();
            qosHandler->getQosInfo()[cid].rlcType = lteInfo->getRlcType();
            qosHandler->getQosInfo()[cid].containsSeveralCids = lteInfo->getContainsSeveralCids();
        }

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
                throw cRuntimeError("LteMacEnb::bufferizePacket - cannot find mac buffer for cid %d", cid);
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
        return true; // this is only a new packet indication - only buffered in virtual queue
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
            // unable to buffer packet (packet is not enqueued and will be dropped): update statistics
            EV << "LteMacBuffers : queue" << cid << " is full - cannot buffer packet " << pkt->getId() << "\n";

            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double) totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());

            if (lteInfo->getDirection() == DL)
                emit(macBufferOverflowDl_, sample);
            else
                emit(macBufferOverflowUl_, sample);

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

void NRMacGNB::handleUpperMessage(cPacket *pkt)
{
    //std::cout << "NRMacGnb::handleUpperMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::handleUpperMessage(pkt);

    //std::cout << "NRMacGnb::handleUpperMessage end at " << simTime().dbl() << std::endl;
}

//incoming racs from Ues, first encouter of cid in UL
void NRMacGNB::macHandleRac(cPacket *pktAux)
{
    //NRMacGnb::macHandleRac(pkt);
    //std::cout << "LteMacEnb::macHandleRac start at " << simTime().dbl() << std::endl;

    auto pkt = check_and_cast<Packet*>(pktAux);

    auto racPkt = pkt->removeAtFront<LteRac>();
    auto uinfo = pkt->getTag<UserControlInfo>();

    //prepare QosHandler
    QosInfo tmp(UL);

    tmp.appType = (ApplicationType) uinfo->getApplication();
    tmp.cid = uinfo->getCid();
    tmp.lcid = uinfo->getLcid();
    tmp.qfi = uinfo->getQfi();
    tmp.radioBearerId = uinfo->getRadioBearerId();
    tmp.destNodeId = uinfo->getDestId();
    tmp.senderNodeId = uinfo->getSourceId();
    tmp.containsSeveralCids = uinfo->getContainsSeveralCids();
    tmp.rlcType = uinfo->getRlcType();
    tmp.trafficClass = (LteTrafficClass) uinfo->getTraffic();
    tmp.dir = (Direction) uinfo->getDirection();
    tmp.lastUpdate = NOW;
    qosHandler->insertQosInfo(uinfo->getCid(), tmp);

    MacNodeId nodeId = MacCidToNodeId(uinfo->getCid());
    MacCid realCid = idToMacCid(nodeId, uinfo->getLcid());

    QosInfo info(UL);

    info.appType = (ApplicationType) uinfo->getApplication();
    info.cid = uinfo->getCid();
    info.lcid = uinfo->getLcid();
    info.qfi = uinfo->getQfi();
    info.radioBearerId = uinfo->getRadioBearerId();
    info.destNodeId = uinfo->getDestId();
    info.senderNodeId = uinfo->getSourceId();
    info.containsSeveralCids = uinfo->getContainsSeveralCids();
    info.rlcType = uinfo->getRlcType();
    info.trafficClass = (LteTrafficClass) uinfo->getTraffic();
    info.dir = (Direction) uinfo->getDirection();
    info.lastUpdate = NOW;
    qosHandler->insertQosInfo(realCid, info);

    enbSchedulerUl_->signalRac(uinfo->getSourceId());
    enbSchedulerUl_->signalRacInfo(uinfo->getSourceId(), uinfo->dup());

    // TODO all RACs are marked "success"
    racPkt->setSuccess(true);

    uinfo->setDestId(uinfo->getSourceId());
    uinfo->setSourceId(nodeId_);
    uinfo->setDirection(DL);

    pkt->insertAtFront(racPkt);

    sendLowerPackets(pkt);

    //std::cout << "LteMacEnb::macHandleRac end at " << simTime().dbl() << std::endl;
}
