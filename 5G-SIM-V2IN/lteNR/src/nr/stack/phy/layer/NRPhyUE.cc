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

#include "nr/stack/phy/layer/NRPhyUE.h"
#include "veins/base/modules/BaseWorldUtility.h"
#include "nr/apps/TrafficGenerator/TrafficGenerator.h"
#include "stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "nr/stack/phy/ChannelModel/NRRealisticChannelModel.h"

Define_Module(NRPhyUE);

NRPhyUE::NRPhyUE() :
        NRPhyUe()
{

}
NRPhyUE::~NRPhyUE()
{
    cancelAndDelete(checkConnectionTimer);
    checkConnectionTimer = nullptr;
}

void NRPhyUE::initialize(int stage)
{
    ChannelAccess::initialize(stage);

    //LtePhyBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_ = getBinder();
        cellInfo_ = nullptr;
        // get gate ids
        upperGateIn_ = findGate("upperGateIn");
        upperGateOut_ = findGate("upperGateOut");
        radioInGate_ = findGate("radioIn");

        // Initialize and watch statistics
        numAirFrameReceived_ = numAirFrameNotReceived_ = 0;
        ueTxPower_ = par("ueTxPower");
        eNodeBtxPower_ = par("eNodeBTxPower");
        microTxPower_ = par("microTxPower");

        //carrierFrequency_ = 2.1e+9;
        WATCH(numAirFrameReceived_);
        WATCH(numAirFrameNotReceived_);

        multicastD2DRange_ = par("multicastD2DRange");
        enableMulticastD2DRangeCheck_ = par("enableMulticastD2DRangeCheck");
    }

    //LtePhyUe::initialize(stage);
    if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT) {

        txPower_ = ueTxPower_;

        lastFeedback_ = 0;

        handoverStarter_ = new cMessage("handoverStarter");

        minRssi_ = binder_->phyPisaData.minSnr();

        mac_ = check_and_cast<LteMacUe*>(getParentModule()-> // nic
        getSubmodule("mac"));
        rlcUm_ = check_and_cast<LteRlcUm*>(getParentModule()-> // nic
        getSubmodule("rlc")->getSubmodule("um"));

        pdcp_ = check_and_cast<LtePdcpRrcBase*>(getParentModule()-> // nic
        getSubmodule("pdcpRrc"));

        nodeId_ = getAncestorPar("macNodeId");

    }
    else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        initializeChannelModel();
        // get serving cell from configuration
        // TODO find a more elegant way
        masterId_ = getAncestorPar("masterId");
        candidateMasterId_ = masterId_;

        // find the best candidate master cell
        if (dynamicCellAssociation_) {
            // this is a fictitious frame that needs to compute the SINR
            LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
            UserControlInfo *cInfo = new UserControlInfo();

            // get the list of all eNodeBs in the network
            std::vector<EnbInfo*> *enbList = binder_->getEnbList();
            std::vector<EnbInfo*>::iterator it = enbList->begin();
            for (; it != enbList->end(); ++it) {
                // the NR phy layer only checks signal from gNBs

                MacNodeId cellId = (*it)->id;
                LtePhyBase *cellPhy = check_and_cast<LtePhyBase*>(
                        (*it)->eNodeB->getSubmodule("cellularNic")->getSubmodule("phy"));
                double cellTxPower = cellPhy->getTxPwr();
                Coord cellPos = cellPhy->getCoord();

                // TODO need to check if the eNodeB uses the same carrier frequency as the UE
                if (!cellPhy->getChannelModel(primaryChannelModel_->getCarrierFrequency())) {
                    throw omnetpp::cRuntimeError("error in dynamic cell association");
                }
                // build a control info
                cInfo->setSourceId(cellId);
                cInfo->setTxPower(cellTxPower);
                cInfo->setCoord(cellPos);
                cInfo->setDirection(DL);
                cInfo->setFrameType(FEEDBACKPKT);

                // get RSSI from the eNB
                std::vector<double>::iterator it;
                double rssi = 0;
                std::vector<double> rssiV = check_and_cast<NRRealisticChannelModel*>(primaryChannelModel_)->getSINR(
                        frame, cInfo, false);
                for (it = rssiV.begin(); it != rssiV.end(); ++it)
                    rssi += *it;
                rssi /= rssiV.size();   // compute the mean over all RBs

                if (rssi > candidateMasterRssi_ || candidateMasterId_ == 0) {
                    candidateMasterId_ = cellId;
                    candidateMasterRssi_ = rssi;
                }
            }
            delete cInfo;
            delete frame;

            // binder calls
            // if dynamicCellAssociation selected a different master
            if (candidateMasterId_ != 0 && candidateMasterId_ != masterId_) {
                binder_->unregisterNextHop(masterId_, nodeId_);
                binder_->registerNextHop(candidateMasterId_, nodeId_);
            }
            masterId_ = candidateMasterId_;
            // set serving cell
            getAncestorPar("masterId").setIntValue(masterId_);
            currentMasterRssi_ = candidateMasterRssi_;
            updateHysteresisTh(candidateMasterRssi_);
        }
        else {
            // get serving cell from configuration
            masterId_ = getAncestorPar("masterId");
            candidateMasterId_ = masterId_;
        }

        das_->setMasterRuSet(masterId_);
        emit(servingCell_, (long) masterId_);
    }
    else if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION) {
        // get cellInfo at this stage because the next hop of the node is registered in the IP2Nic module at the INITSTAGE_NETWORK_LAYER
        if (masterId_ > 0) {
            cellInfo_ = getCellInfo(nodeId_);
            int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
            if (cellInfo_ != NULL) {
                cellInfo_->lambdaInit(nodeId_, index);
                cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
            }
        }
        else
            cellInfo_ = NULL;
    }

    //LtePhyUeD2D::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        averageCqiD2D_ = registerSignal("averageCqiD2D");
        d2dTxPower_ = par("d2dTxPower");
        d2dMulticastEnableCaptureEffect_ = par("d2dMulticastCaptureEffect");
        d2dDecodingTimer_ = nullptr;
    }

    //NRPhyUe::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        otherPhy_ = check_and_cast<NRPhyUe*>(getParentModule()->getSubmodule("phy"));
    }

    //std::cout << "NRPhyUe::init start at " << simTime().dbl() << std::endl;
    if (stage == inet::INITSTAGE_LOCAL) {

        nodeType_ = UE;
        useBattery_ = false;  // disabled
        enableHandover_ = par("enableHandover");
        handoverLatency_ = par("handoverLatency").doubleValue();
        dynamicCellAssociation_ = par("dynamicCellAssociation");
        currentMasterRssi_ = 0;
        candidateMasterRssi_ = 0;
        candidateMasterId_ = 0;
        hysteresisTh_ = 0;
        hysteresisFactor_ = 10;
        handoverDetachment_ = handoverLatency_ / 2.0;
        handoverAttachment_ = handoverLatency_ - handoverDetachment_;
        handoverDelta_ = par("handoverDelta").doubleValue();
        dasRssiThreshold_ = par("dasRssiThreshold").doubleValue();

        das_ = new DasFilter(this, binder_, NULL, dasRssiThreshold_);

        servingCell_ = registerSignal("servingCell");
        averageCqiDl_ = registerSignal("averageCqiDl");
        averageCqiUl_ = registerSignal("averageCqiUl");

        averageTxPower = registerSignal("averageTxPower");
        attenuation = registerSignal("attenuation");
        snir = registerSignal("snir");
        totalPer = registerSignal("totalPer");
        d2d = registerSignal("d2d");
        d3d = registerSignal("d3d");
        bler = registerSignal("bler");
        speed = registerSignal("speed");

        emit(averageTxPower, txPower_);
        errorCount = 0;

        //check Connection procedure
        checkConnectionTimer = new cMessage("checkConnectionTimer");
        useSINRThreshold = getSimulation()->getSystemModule()->par("useSINRThreshold").boolValue();
        checkConnectionInterval = getSimulation()->getSystemModule()->par("checkConnectionInterval").doubleValue();
        //

        WATCH(nodeType_);
        WATCH(masterId_);
        WATCH(candidateMasterId_);
        WATCH(dasRssiThreshold_);
        WATCH(currentMasterRssi_);
        WATCH(candidateMasterRssi_);
        WATCH(hysteresisTh_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);
        WATCH(das_);
    }
    else if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION) {

        qosHandler = check_and_cast<QosHandlerUE*>(getParentModule()->getSubmodule("qosHandler"));

    }
    else if (stage == inet::INITSTAGE_LAST) {

        checkConnection();

        carrierFrequency_ = primaryChannelModel_->getCarrierFrequency();

    }
    //std::cout << "NRPhyUE::init end at " << simTime().dbl() << std::endl;
}

void NRPhyUE::handoverHandler(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    lteInfo->setDestId(nodeId_);
    if (!enableHandover_) {
        // Even if handover is not enabled, this call is necessary
        // to allow Reporting Set computation.
        if ((getNodeTypeById(lteInfo->getSourceId()) == ENODEB || getNodeTypeById(lteInfo->getSourceId()) == GNODEB)
                && lteInfo->getSourceId() == masterId_) {
            // Broadcast message from my master enb
            das_->receiveBroadcast(frame, lteInfo);
        }

        delete frame;
        delete lteInfo;
        return;
    }

    frame->setControlInfo(lteInfo);
    double rssi;

    if ((getNodeTypeById(lteInfo->getSourceId()) == ENODEB || getNodeTypeById(lteInfo->getSourceId()) == GNODEB)
            && lteInfo->getSourceId() == masterId_) {
        // Broadcast message from my master enb
        rssi = das_->receiveBroadcast(frame, lteInfo);
    }
    else {
        // Broadcast message from not-master enb
        std::vector<double>::iterator it;
        rssi = 0;
        std::vector<double> rssiV = check_and_cast<NRRealisticChannelModel*>(primaryChannelModel_)->getSINR(frame,
                lteInfo, false);
        for (it = rssiV.begin(); it != rssiV.end(); ++it)
            rssi += *it;
        rssi /= rssiV.size();
    }

    if (lteInfo->getSourceId() != masterId_ && rssi < minRssi_) {

        delete frame;
        return;
    }

    if (rssi > candidateMasterRssi_ + hysteresisTh_) {
        if (lteInfo->getSourceId() == masterId_) {
            // receiving even stronger broadcast from current master
            currentMasterRssi_ = rssi;
            candidateMasterId_ = masterId_;
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);
            cancelEvent(handoverStarter_);
        }
        else {
            // broadcast from another master with higher rssi
            candidateMasterId_ = lteInfo->getSourceId();
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(rssi);
            binder_->addHandoverTriggered(nodeId_, masterId_, candidateMasterId_);

            // schedule self message to evaluate handover parameters after
            // all broadcast messages are arrived
            if (!handoverStarter_->isScheduled()) {
                // all broadcast messages are scheduled at the very same time, a small delta
                // guarantees the ones belonging to the same turn have been received
                scheduleAt(simTime() + handoverDelta_, handoverStarter_);
            }
        }
    }
    else {
        if (lteInfo->getSourceId() == masterId_) {
            if (rssi >= minRssi_) {
                currentMasterRssi_ = rssi;
                candidateMasterRssi_ = rssi;
                hysteresisTh_ = updateHysteresisTh(rssi);
            }
            else  // lost connection from current master
            {
                if (candidateMasterId_ == masterId_)  // trigger detachment
                        {
                    candidateMasterId_ = 0;
                    candidateMasterRssi_ = 0;
                    hysteresisTh_ = updateHysteresisTh(0);
                    binder_->addHandoverTriggered(nodeId_, masterId_, candidateMasterId_);

                    if (!handoverStarter_->isScheduled()) {
                        // all broadcast messages are scheduled at the very same time, a small delta
                        // guarantees the ones belonging to the same turn have been received
                        scheduleAt(simTime() + handoverDelta_, handoverStarter_);
                    }
                }
                // else do nothing, a stronger RSSI from another nodeB has been found already
            }
        }
    }

    delete frame;
}

void NRPhyUE::initializeChannelModel()
{
    std::string moduleName = "nrChannelModel";
    primaryChannelModel_ = check_and_cast<NRRealisticChannelModel*>(
            getParentModule()->getSubmodule(moduleName.c_str(), 0));
    primaryChannelModel_->setPhy(this);
    double carrierFreq = primaryChannelModel_->getCarrierFrequency();
    unsigned int numerologyIndex = primaryChannelModel_->getNumerologyIndex();
    channelModel_[carrierFreq] = primaryChannelModel_;

    if (nodeType_ == UE)
        binder_->registerCarrierUe(carrierFreq, numerologyIndex, nodeId_);

    int vectSize = primaryChannelModel_->getVectorSize();
    NRRealisticChannelModel *chanModel = NULL;
    for (int index = 1; index < vectSize; index++) {
        chanModel = check_and_cast<NRRealisticChannelModel*>(
                getParentModule()->getSubmodule(moduleName.c_str(), index));
        chanModel->setPhy(this);
        carrierFreq = chanModel->getCarrierFrequency();
        numerologyIndex = chanModel->getNumerologyIndex();
        channelModel_[carrierFreq] = chanModel;
        if (nodeType_ == UE)
            binder_->registerCarrierUe(carrierFreq, numerologyIndex, nodeId_);
    }
}

void NRPhyUE::checkConnection()
{

    if (useSINRThreshold) {

        // this is a fictitious frame that needs to compute the SINR
        LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
        UserControlInfo *cInfo = new UserControlInfo();

        // get the list of all eNodeBs in the network
        std::vector<EnbInfo*> *enbList = binder_->getEnbList();
        std::vector<EnbInfo*>::iterator it = enbList->begin();

        it = enbList->begin();

        double maxRssiUL = -20;
        int bestGNB = 0;
        double bestGNBSINR = 0;
        std::vector<double> rssiV;

        for (; it != enbList->end(); ++it) {

            MacNodeId cellId = (*it)->id;
            LtePhyBase *cellPhy = check_and_cast<LtePhyBase*>(
                    (*it)->eNodeB->getSubmodule("cellularNic")->getSubmodule("phy"));

            Coord cellPos = cellPhy->getCoord();

            // build a control info
            cInfo->setSourceId(cellId);
            cInfo->setDestId(nodeId_);
            cInfo->setTxPower(cellPhy->getTxPwr());
            cInfo->setCoord(cellPos);
            cInfo->setDirection(DL);
            cInfo->setFrameType(FEEDBACKPKT);
            //DL direction --> receiver is UE
            rssiV = check_and_cast<NRRealisticChannelModel*>(primaryChannelModel_)->getSINR(frame, cInfo, false);

            // get RSSI from the eNB
            std::vector<double>::iterator it;
            double rssi = 0;
            for (it = rssiV.begin(); it != rssiV.end(); ++it)
                rssi += *it;
            rssi /= rssiV.size();   // compute the mean over all RBs

            if (rssi > maxRssiUL) {
                bestGNB = cellId;
                bestGNBSINR = rssi;
            }

            maxRssiUL = std::max(rssi, maxRssiUL);

        }
        delete cInfo;
        delete frame;
        //

        //signal to all nodeBs is weak

        if (maxRssiUL
                <= std::min(minRssi_, double(getSimulation()->getSystemModule()->par("SINRThreshold").intValue()))) {
            getNRBinder()->insertUeToNotConnectedList(nodeId_);
            deleteOldBuffers(masterId_);
        }
        else {
            //best sinr from last master --> reattach
            if (bestGNB == masterId_ && getBinder()->isNotConnected(nodeId_)) {
                getNRBinder()->deleteFromUeNotConnectedList(nodeId_);
                //to guarantee that no old packets are in the buffers
                deleteOldBuffers(masterId_);
            }

        }
        cancelEvent(checkConnectionTimer);
        scheduleAt(
                simTime() + getSimulation()->getSystemModule()->par("checkConnectionInterval").doubleValue()
                        + uniform(0, 0.005), checkConnectionTimer);
    }

}

void NRPhyUE::recordAttenuation(const double &att)
{
    emit(attenuation, att);
}

void NRPhyUE::recordSNIR(const double &snirVal)
{
    emit(snir, snirVal);
}

void NRPhyUE::recordDistance3d(const double &d3dVal)
{
    emit(d3d, d3dVal);
}

void NRPhyUE::recordDistance2d(const double &d2dVal)
{
    emit(d2d, d2dVal);
}

void NRPhyUE::handleMessage(cMessage *msg)
{

    //std::cout << "NRPhyUe handleMessage start at " << simTime().dbl() << std::endl;

    if (msg->isName("checkConnectionTimer")) {
        checkConnection();
        return;
    }

    NRPhyUe::handleMessage(msg);

    //std::cout << "NRPhyUe handleMessage end at " << simTime().dbl() << std::endl;
}

void NRPhyUE::triggerHandover()
{

    assert(masterId_ != candidateMasterId_);

    MacNodeId masterNode = binder_->getMasterNode(candidateMasterId_);
    if (masterNode != candidateMasterId_)  // the candidate is a secondary node
            {
        if (otherPhy_ != this) {
            if (otherPhy_->getMasterId() == masterNode) {
                MacNodeId otherNodeId = otherPhy_->getMacNodeId();
                const std::pair<MacNodeId, MacNodeId> *handoverPair = binder_->getHandoverTriggered(otherNodeId);
                if (handoverPair != NULL) {
                    if (handoverPair->second == candidateMasterId_) {
                        // delay this handover
                        double delta = handoverDelta_;
                        if (handoverPair->first != 0) // the other "stack" is performing a complete handover
                            delta += handoverDetachment_ + handoverAttachment_;
                        else
                            // the other "stack" is attaching to an eNodeB
                            delta += handoverAttachment_;

                        // need to wait for the other stack to complete handover
                        scheduleAt(simTime() + delta, handoverStarter_);
                        return;
                    }
                    else {
                        // cancel this handover
                        binder_->removeHandoverTriggered(nodeId_);

                        return;
                    }
                }
            }
        }
    }

    if (otherPhy_ != this) {
        if (otherPhy_->getMasterId() != 0) {
            // check if there are secondary nodes connected
            MacNodeId otherMasterId = binder_->getMasterNode(otherPhy_->getMasterId());
            if (otherMasterId == masterId_) {

                // need to wait for the other stack to complete detachment
                scheduleAt(simTime() + handoverDetachment_ + handoverDelta_, handoverStarter_);

                // the other stack is connected to a node which is a secondary node of the master from which this stack is leaving
                // trigger detachment (handover to node 0)
                otherPhy_->forceHandover();

                return;
            }
        }
    }

    binder_->addUeHandoverTriggered(nodeId_);

    // inform the UE's IP2Nic module to start holding downstream packets
    IP2NR *ip2nic = check_and_cast<IP2NR*>(getParentModule()->getSubmodule("ip2nic"));
    ip2nic->triggerHandoverUe(candidateMasterId_, false);

    // inform the eNB's IP2Nic module to forward data to the target eNB
    if (masterId_ != 0 && candidateMasterId_ != 0) {
        IP2NR *enbIp2nic = check_and_cast<IP2NR*>(
                getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("cellularNic")->getSubmodule(
                        "ip2nic"));
        enbIp2nic->triggerHandoverSource(nodeId_, candidateMasterId_);
    }

    double handoverLatency;
    if (masterId_ == 0)                        // attachment only
        handoverLatency = handoverAttachment_;
    else if (candidateMasterId_ == 0)          // detachment only
        handoverLatency = handoverDetachment_;
    else
        // complete handover time
        handoverLatency = handoverDetachment_ + handoverAttachment_;

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency, handoverTrigger_);
}

void NRPhyUE::doHandover()
{
    // if masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (masterId_ != 0) {
        // Delete Old Buffers
        deleteOldBuffers(masterId_);

        // amc calls
        LteAmc *oldAmc = getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, UL);
        oldAmc->detachUser(nodeId_, DL);
        oldAmc->detachUser(nodeId_, D2D);
    }

    if (candidateMasterId_ != 0) {
        LteAmc *newAmc = getAmcModule(candidateMasterId_);
        assert(newAmc != NULL);
        newAmc->attachUser(nodeId_, UL);
        newAmc->attachUser(nodeId_, DL);
        newAmc->attachUser(nodeId_, D2D);
    }

    // binder calls
    if (masterId_ != 0)
        binder_->unregisterNextHop(masterId_, nodeId_);

    if (candidateMasterId_ != 0) {
        binder_->registerNextHop(candidateMasterId_, nodeId_);
        das_->setMasterRuSet(candidateMasterId_);
    }
    binder_->updateUeInfoCellId(nodeId_, candidateMasterId_);

    // change masterId and notify handover to the MAC layer
    MacNodeId oldMaster = masterId_;
    masterId_ = candidateMasterId_;
    mac_->doHandover(candidateMasterId_);  // do MAC operations for handover
    currentMasterRssi_ = candidateMasterRssi_;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    // update cellInfo
    if (masterId_ != 0)
        cellInfo_->detachUser(nodeId_);

    if (candidateMasterId_ != 0) {
        CellInfo *oldCellInfo = cellInfo_;
        LteMacEnb *newMacEnb =
                check_and_cast<LteMacEnb*>(
                        getSimulation()->getModule(binder_->getOmnetId(candidateMasterId_))->getSubmodule("cellularNic")->getSubmodule(
                                "mac"));
        CellInfo *newCellInfo = newMacEnb->getCellInfo();
        newCellInfo->attachUser(nodeId_);
        cellInfo_ = newCellInfo;
        if (oldCellInfo == NULL) {
            // first time the UE is attached to someone
            int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
            cellInfo_->lambdaInit(nodeId_, index);
            cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
        }
    }

    // update DL feedback generator
    LteDlFeedbackGenerator *fbGen = check_and_cast<LteDlFeedbackGenerator*>(getParentModule()->getSubmodule("dlFbGen"));
    fbGen->handleHandover(masterId_);

    // collect stat
    emit(servingCell_, (long) masterId_);

    binder_->removeUeHandoverTriggered(nodeId_);
    binder_->removeHandoverTriggered(nodeId_);

    // inform the UE's IP2Nic module to forward held packets
    IP2NR *ip2nic = check_and_cast<IP2NR*>(getParentModule()->getSubmodule("ip2nic"));
    ip2nic->signalHandoverCompleteUe(false);

    // inform the eNB's IP2Nic module to forward data to the target eNB
    if (oldMaster != 0 && candidateMasterId_ != 0) {
        IP2NR *enbIp2nic = check_and_cast<IP2NR*>(
                getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("cellularNic")->getSubmodule(
                        "ip2nic"));
        enbIp2nic->signalHandoverCompleteTarget(nodeId_, oldMaster);
    }
}

void NRPhyUE::finish()
{
    recordScalar("#errorCounts", errorCount);
    LtePhyUe::finish();
}

void NRPhyUE::recordBler(const double &blerVal)
{
    emit(bler, blerVal);
}

void NRPhyUE::recordSpeed(const double &speedVal)
{
    emit(speed, speedVal);
}

void NRPhyUE::errorDetected()
{
    ++errorCount;
}

void NRPhyUE::recordTotalPer(const double &totalPerVal)
{
    emit(totalPer, totalPerVal);
}

//when UE is leaving the simulation and for simplified Handover
void NRPhyUE::deleteOldBuffers(MacNodeId masterId)
{

    //std::cout << "NRPhyUe deleteOldBuffers start at " << simTime().dbl() << std::endl;

    OmnetId masterOmnetId = binder_->getOmnetId(masterId);

    for (auto &var : channelModel_) {
        check_and_cast<NRRealisticChannelModel*>(var.second)->resetOnHandover(nodeId_, masterId);
    }

    LtePhyBase *masterPhy = check_and_cast<LtePhyBase*>(
            getSimulation()->getModule(masterOmnetId)->getSubmodule("cellularNic")->getSubmodule("phy"));
    std::map<double, LteChannelModel*>::const_iterator it = masterPhy->getChannelModels()->begin();
    for (; it != masterPhy->getChannelModels()->end(); ++it) {
        check_and_cast<NRRealisticChannelModel*>(it->second)->resetOnHandover(nodeId_, masterId);
    }

    // delete macBuffer[nodeId_] at old master
    NRMacGNB *masterMac = check_and_cast<NRMacGNB*>(
            getSimulation()->getModule(masterOmnetId)->getSubmodule("cellularNic")->getSubmodule("mac"));
    masterMac->deleteQueues(nodeId_);
    //qosHandler GNB
    masterMac->deleteNodeFromQosHandler(nodeId_);
    masterMac->deleteFromRtxMap(nodeId_);

    // delete queues for master at this ue
    mac_->deleteQueues(masterId_);
    //qosHandler UE
    check_and_cast<NRMacUE*>(mac_)->getQosHandler()->clearQosInfos();
    check_and_cast<NRMacUE*>(mac_)->resetParamsOnHandover();

    /////////////////////////////////////////////////////////////////////////////////

    /* Delete Rlc UM Buffers */
    LteRlcUm *masterRlcUmRealistic = check_and_cast<LteRlcUm*>(
            getSimulation()->getModule(masterOmnetId)->getSubmodule("cellularNic")->getSubmodule("rlc")->getSubmodule(
                    "um"));
    masterRlcUmRealistic->deleteQueues(nodeId_);

    // delete queues for master at this ue
    rlcUm_->deleteQueues(nodeId_);

    ////////////////////////////////////////////////////////////////////////////////

    //pdcp_rrc --> connectionTable reset, masterId == oldENB --> delete entry
    // nodeId_ --> delete whole
    NRPdcpRrcGnb *masterPdcp = check_and_cast<NRPdcpRrcGnb*>(
            getSimulation()->getModule(masterOmnetId)->getSubmodule("cellularNic")->getSubmodule("pdcpRrc"));
    masterPdcp->resetConnectionTable(masterId, nodeId_);
    masterPdcp->deleteEntities(nodeId_);

    check_and_cast<NRPdcpRrcUE*>(pdcp_)->resetConnectionTable(masterId, nodeId_);
    pdcp_->deleteEntities(masterId_);

    //SDAP///////////////////////////////////////////////////////////////////////////
    NRsdapUE *sdapUe = check_and_cast<NRsdapUE*>(getParentModule()-> // nic
    getSubmodule("sdap"));

    sdapUe->deleteEntities(masterId);

    NRsdapGNB *sdapGNB = check_and_cast<NRsdapGNB*>(
            getSimulation()->getModule(masterOmnetId)->getSubmodule("cellularNic")->getSubmodule("sdap"));

    sdapGNB->deleteEntities(nodeId_);

    //std::cout << "NRPhyUe deleteOldBuffers end at " << simTime().dbl() << std::endl;
}

