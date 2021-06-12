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

Define_Module(NRMacGNB);

NRMacGNB::NRMacGNB() :
        NRMacGnb() {
	scheduleListDl_ = NULL;
}

NRMacGNB::~NRMacGNB() {
}

void NRMacGNB::initialize(int stage) {
    //NRMacGnb::initialize(stage);
    if (stage == inet::INITSTAGE_LINK_LAYER) {
        if (enbSchedulerUl_ != NULL) {
            delete enbSchedulerUl_;
        }
            enbSchedulerUl_ = new NRSchedulerGnbUL();
            enbSchedulerUl_->initialize(UL, this);
    }

    //LteMacBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        {
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
        }

    //LteMacEnb::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
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
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        /* Create and initialize AMC module */

        amc_ = new NRAmc(this, binder_, cellInfo_, numAntennas_, getSimulation()->getSystemModule()->par("useExtendedMcsTable").boolValue());

        std::string modeString = par("pilotMode").stdstringValue();

        if( modeString == "AVG_CQI" )
            amc_->setPilotMode(AVG_CQI);
        else if( modeString == "MAX_CQI" )
            amc_->setPilotMode(MAX_CQI);
        else if( modeString == "MIN_CQI" )
            amc_->setPilotMode(MIN_CQI);
        else if( modeString == "MEDIAN_CQI" )
            amc_->setPilotMode(MEDIAN_CQI);
        else if( modeString == "ROBUST_CQI" )
            amc_->setPilotMode(ROBUST_CQI);
        else
            throw cRuntimeError("LteMacEnb::initialize - Unknown Pilot Mode %s \n" , modeString.c_str());

        /* Insert EnbInfo in the Binder */
        EnbInfo* info = new EnbInfo();
        info->id = nodeId_;            // local mac ID
        info->nodeType = nodeType_;    // eNB or gNB
        info->type = MACRO_ENB;        // eNb Type
        info->init = false;            // flag for phy initialization
        info->eNodeB = this->getParentModule()->getParentModule();  // reference to the eNodeB module

        // register the pair <id,name> to the binder
        const char* moduleName = getParentModule()->getParentModule()->getFullName();
        binder_->registerName(nodeId_, moduleName);

        // get the reference to the PHY layer
        phy_ = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

        info->phy = phy_;
        info->mac = this;
        binder_->addEnbInfo(info);
        qosHandler = check_and_cast<QosHandlerGNB*>(getParentModule()->getSubmodule("qosHandler"));
    }
    else if (stage == inet::INITSTAGE_LINK_LAYER)
    {
        /* Create and initialize MAC Downlink scheduler */
        if (enbSchedulerDl_ == nullptr)
        {
            enbSchedulerDl_ = new NRSchedulerGnbDl();
            enbSchedulerDl_->initialize(DL, this);
        }

        /* Create and initialize MAC Uplink scheduler */
        if (enbSchedulerUl_ == nullptr)
        {
            enbSchedulerUl_ = new NRSchedulerGnbUL();
            enbSchedulerUl_->initialize(UL, this);
        }
        harqProcesses_ =
                getSystemModule()->par("numberHarqProcesses").intValue();
        harqProcessesNR_ =
                getSystemModule()->par("numberHarqProcessesNR").intValue();
        if (getSystemModule()->par("nrHarq").boolValue()) {
            harqProcesses_ = harqProcessesNR_;
        }
    }
    else if (stage == inet::INITSTAGE_LAST)
    {
        /* Start TTI tick */
        // the period is equal to the minimum period according to the numerologies used by the carriers in this node
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);                                              // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(cellInfo_->getMaxNumerologyIndex());
        scheduleAt(NOW + ttiPeriod_, ttiTick_);

        const CarrierInfoMap* carriers = cellInfo_->getCarrierInfoMap();
        CarrierInfoMap::const_iterator it = carriers->begin();
        for ( ; it != carriers->end(); ++it)
        {
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
    if (stage == inet::INITSTAGE_LOCAL + 1)
    {
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

        if (reuseD2D_ || reuseD2DMulti_)
        {
            conflictGraphUpdatePeriod_ = par("conflictGraphUpdatePeriod");

            CGType cgType = CG_DISTANCE;  // TODO make this parametric
            switch(cgType)
            {
                case CG_DISTANCE:
                {
                    conflictGraph_ = new DistanceBasedConflictGraph(this, reuseD2D_, reuseD2DMulti_, par("conflictGraphThreshold"));
                    check_and_cast<DistanceBasedConflictGraph*>(conflictGraph_)->setThresholds(par("conflictGraphD2DInterferenceRadius"), par("conflictGraphD2DMultiTxRadius"), par("conflictGraphD2DMultiInterferenceRadius"));
                    break;
                }
                default: { throw cRuntimeError("LteMacEnbD2D::initialize - CG type unknown. Aborting"); }
            }

            scheduleAt(NOW + 0.05, new cMessage("updateConflictGraph"));
        }
    }

}

void NRMacGNB::fromPhy(omnetpp::cPacket *pktIn) {
    //std::cout << "NRMacGnbRealistic::fromPhy start at " << simTime().dbl() << std::endl;

    auto pkt = check_and_cast<inet::Packet*> (pktIn);
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
            qosHandler->getQosInfo()[userInfo->getCid()] = tmp;
        }

    }

    NRMacGnb::fromPhy(pkt);

    //std::cout << "NRMacGnbRealistic::fromPhy end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::handleMessage(cMessage *msg) {

	//std::cout << "NRMacGnbRealistic::handleMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::handleMessage(msg);

	//std::cout << "NRMacGnbRealistic::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::handleSelfMessage() {
	//std::cout << "NRMacGnb::handleSelfMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::handleSelfMessage();

	//std::cout << "NRMacGnb::handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::sendGrants(std::map<double, LteMacScheduleList>* scheduleList) {
	//std::cout << "LteMacEnb::sendGrants start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::sendGrants(scheduleList);

	//std::cout << "LteMacEnb::sendGrants end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::macPduUnmake(omnetpp::cPacket* pktAux) {
	//std::cout << "NRMacEnb::macPduUnmake start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::macPduUnmake(pktAux);

	//std::cout << "NRMacEnb::macPduUnmake end at " << simTime().dbl() << std::endl;

}

void NRMacGNB::macSduRequest() {
	//std::cout << "NRMacGnb::macSduRequest start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::macSduRequest();

	//std::cout << "NRMacGnb::macSduRequest end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::macPduMake(MacCid cid) {
	//std::cout << "NRMacGnb::macPduMake start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::macPduMake(cid);

	//std::cout << "NRMacGnb::macPduMake end at " << simTime().dbl() << std::endl;
}

bool NRMacGNB::bufferizePacket(cPacket *pkt) {
	//std::cout << "NRMacGnb::bufferizePacket start at " << simTime().dbl() << std::endl;

	//std::cout << "NRMacGnb::bufferizePacket end at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
	return NRMacGnb::bufferizePacket(pkt);
}

void NRMacGNB::handleUpperMessage(cPacket *pkt) {
	//std::cout << "NRMacGnb::handleUpperMessage start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::handleUpperMessage(pkt);

	//std::cout << "NRMacGnb::handleUpperMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGNB::flushHarqBuffers() {
	//std::cout << "NRMacGnb::flushHarqBuffers start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    NRMacGnb::flushHarqBuffers();

	//std::cout << "NRMacGnb::flushHarqBuffers end at " << simTime().dbl() << std::endl;
}
