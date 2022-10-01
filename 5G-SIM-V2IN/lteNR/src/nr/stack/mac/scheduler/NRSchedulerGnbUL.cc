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

#include "nr/stack/mac/scheduler/NRSchedulerGnbUL.h"
#include "nr/stack/mac/layer/NRMacGNB.h"
#include "nr/stack/mac/layer/NRMacUE.h"

std::map<double, LteMacScheduleList>* NRSchedulerGnbUL::schedule()
{
    //std::cout << "NRSchedulerGnbUL::schedule start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    schedulingNodeSet.clear();
    return NRSchedulerGnbUl::schedule();

    //std::cout << "NRSchedulerGnbUL::schedule end at " << simTime().dbl() << std::endl;

}

bool NRSchedulerGnbUL::racschedule(double carrierFrequency)
{
    //std::cout << "NRSchedulerGnbUL::racschedule start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
//    return NRSchedulerGnbUl::racschedule(carrierFrequency);

    RacStatus::iterator it = racStatus_.begin(), et = racStatus_.end();

    if (fairSchedule) {

        unsigned int numberOfNodes = racStatus_.size(); //nodes which need resources
        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        unsigned int totalBlocks = numBands;
        unsigned int distrBlock = 0;
        if (numberOfNodes > 0)
            distrBlock = floor(totalBlocks / numberOfNodes);

        for (; it != et; ++it) {
            // get current nodeId
            MacNodeId nodeId = it->first;
            unsigned int bytesize = racStatusInfo_[nodeId]->getBytesize();    //bytes UE want to send
            int reqBlocks = -1;
            unsigned int addedBlocks = 0;

            const unsigned int cw = 0;

            bool allocation = false;

            for (Band b = 0; b < numBands; ++b) {

                if (reqBlocks == -1) {
                    reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, UL, carrierFrequency); //required Blocks
                    reqBlocks = (distrBlock < reqBlocks) ? distrBlock : reqBlocks;
                }

                unsigned int availableBlocks = allocator_->availableBlocks(nodeId, MACRO, b);

                if (availableBlocks > 0) {

                    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, availableBlocks, UL,
                            carrierFrequency);
                    allocator_->addBlocks(MACRO, b, nodeId, availableBlocks, bytes);
                    addedBlocks += availableBlocks;
                    reqBlocks -= availableBlocks;
                    allocation = true;

                    if (reqBlocks == 0) {
                        break;
                    }
                }

            }

            if (allocation) {
                // create scList id for current cid/codeword
                MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
                                                             // we use the LCID corresponding to the SHORT_BSR
                std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
                scheduleList_[carrierFrequency][scListId] = addedBlocks;
            }
        }
    }
    else {
        //default behavior
        for (; it != et; ++it) {
            // get current nodeId
            MacNodeId nodeId = it->first;

            // Get number of logical bands
            unsigned int numBands = mac_->getCellInfo()->getNumBands();

            // FIXME default behavior
            //try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

            const unsigned int cw = 0;
            const unsigned int blocks = 1;

            bool allocation = false;

            for (Band b = 0; b < numBands; ++b) {
                if (allocator_->availableBlocks(nodeId, MACRO, b) > 0) {
                    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, blocks, UL,
                            carrierFrequency);
                    if (bytes > 0) {
                        allocator_->addBlocks(MACRO, b, nodeId, 1, bytes);

                        allocation = true;
                        break;
                    }
                }
            }

            if (allocation) {
                // create scList id for current cid/codeword
                MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
                                                             // we use the LCID corresponding to the SHORT_BSR
                std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
                scheduleList_[carrierFrequency][scListId] = blocks;
            }
        }
    }

    // clean up all requests
    racStatus_.clear();
    racStatusInfo_.clear();

    int availableBlocks = allocator_->computeTotalRbs();

    return (availableBlocks == 0);

}

/**
 * changed the behavior
 * racschedule is called after checking whether a rtx is needed
 *
 */
bool NRSchedulerGnbUL::rtxschedule(double carrierFrequency, BandLimitVector *bandLim)
{

    //std::cout << "NRSchedulerGnbUl::rtxscheduleWith start at " << simTime().dbl() << std::endl;

    if (useQosModel) {
        qosModelSchedule(carrierFrequency);
        //return always true to avoid calling other scheduling functions
        return true;
    }

    if (newTxbeforeRtx) {
        if (racschedule(carrierFrequency))        //racschedule() returns true if no resource blocks are available
            return true;
    }

    try {

        HarqRxBuffers *harqQueues = mac_->getHarqRxBuffers(carrierFrequency);

        if (harqQueues != NULL) {
            HarqRxBuffers::iterator it = harqQueues->begin();
            HarqRxBuffers::iterator et = harqQueues->end();
            //
            std::map<std::pair<MacNodeId, Codeword>, std::pair<unsigned short, omnetpp::simtime_t>> macNodeProcessRxTime;
            for (; it != et; ++it) {

                // get current nodeId
                MacNodeId nodeId = it->first;

                if (nodeId == 0) {
                    // UE has left the simulation - erase queue and continue
                    harqRxBuffers_->erase(nodeId);
                    continue;
                }
                OmnetId id = binder_->getOmnetId(nodeId);
                if (id == 0) {
                    harqRxBuffers_->erase(nodeId);
                    continue;
                }

                if (schedulingNodeSet.find(nodeId) != schedulingNodeSet.end()) {
                    //already one cid scheduled for this node
                    continue;
                }

                LteHarqBufferRx *currHarq = it->second;
                std::vector<LteHarqProcessRx*> processes = currHarq->getProcessesRx();

                unsigned short process = 0;
                unsigned int maxProcesses = currHarq->getNumHarqProcesses();

                //find oldest process
                for (process = 0; process < maxProcesses; ++process) {
                    LteHarqProcessRx *currentProcess = it->second->getProcess(process);
                    const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_,
                            carrierFrequency); // get the user info
                    unsigned int codewords = txParams.getLayers().size();

                    for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
                        if (currentProcess->getByteLength(cw) <= 0) {
                            continue;
                        }
                        if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
                            continue;
                        }
                        std::pair<MacNodeId, Codeword> nodeIdCw = std::make_pair(nodeId, cw);
                        omnetpp::simtime_t timestamp = currentProcess->getRxTimeForCodeWord(cw);
                        std::pair<unsigned short, omnetpp::simtime_t> processRxTime = std::make_pair(process,
                                timestamp);
                        macNodeProcessRxTime[nodeIdCw] = processRxTime;
                    }
                }
                //
            }

            while (macNodeProcessRxTime.size() > 0) {
                std::map<std::pair<MacNodeId, Codeword>, std::pair<unsigned short, simtime_t>>::iterator it;
                it = macNodeProcessRxTime.begin();
                std::pair<MacNodeId, Codeword> oldestNodeIdCw = it->first;
                std::pair<unsigned short, simtime_t> oldestProcessRxTime = it->second;

                for (auto &var : macNodeProcessRxTime) {
                    std::pair<MacNodeId, Codeword> nodeIdCw = var.first;
                    std::pair<unsigned short, simtime_t> processRxTime = var.second;

                    if (processRxTime < oldestProcessRxTime) {
                        oldestNodeIdCw = nodeIdCw;
                        oldestProcessRxTime = processRxTime;
                    }
                }

                unsigned int rtxBytes = 0;
                // perform a retransmission on available codewords for the selected acid
                rtxBytes = schedulePerAcidRtx(oldestNodeIdCw.first, carrierFrequency, oldestNodeIdCw.second,
                        oldestProcessRxTime.first, bandLim);

                if (rtxBytes > 0) {
                    macNodeProcessRxTime.erase(oldestNodeIdCw);
                    schedulingNodeSet.insert(oldestNodeIdCw.first);
                }
                else {
                    break;
                }
            }
        }
        //

        int availableBlocks = allocator_->computeTotalRbs();

        if (!newTxbeforeRtx) {
            racschedule(carrierFrequency);
        }

        return (availableBlocks == 0);

        //return (availableBlocks == 0);
    }
    catch (std::exception &e) {
        throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxschedule(): %s", e.what());
    }
    //std::cout << "NRSchedulerGnbUl::rtxscheduleWith end at " << simTime().dbl() << std::endl;

    return 0;
}

void NRSchedulerGnbUL::qosModelSchedule(double carrierFrequency)
{

    //std::cout << "NRSchedulerGnbUL::qosModelSchedule start at " << simTime().dbl() << std::endl;

    //retrieve the lambdaValues from the ini file
    double lambdaPriority = getSimulation()->getSystemModule()->par("lambdaPriority").doubleValue();
    double lambdaRemainDelayBudget = getSimulation()->getSystemModule()->par("lambdaRemainDelayBudget").doubleValue();
    double lambdaCqi = getSimulation()->getSystemModule()->par("lambdaCqi").doubleValue();
    double lambdaRtx = getSimulation()->getSystemModule()->par("lambdaRtx").doubleValue();
    double lambdaByteSize = getSimulation()->getSystemModule()->par("lambdaByteSize").doubleValue(); //consider also the size of one packet
    //

    std::map<double, std::vector<ScheduleInfo>> combinedMap;
    std::vector<MacNodeId> neglectUe;

    // use code from rtxschedule to find out the harqprocesses which need a rtx
    // retrieving reference to HARQ entities

    HarqRxBuffers *harqQueues = mac_->getHarqRxBuffers(carrierFrequency);
    if (harqQueues != NULL) {
        HarqRxBuffers::iterator harqIt = harqQueues->begin();
        HarqRxBuffers::iterator et = harqQueues->end();

        for (; harqIt != et; ++harqIt) {

            // get current nodeId
            MacNodeId nodeId = harqIt->first;

            if (nodeId == 0) {
                // UE has left the simulation - erase queue and continue
                harqRxBuffers_->erase(nodeId);
                continue;
            }
            OmnetId id = binder_->getOmnetId(nodeId);
            if (id == 0) {
                harqRxBuffers_->erase(nodeId);
                continue;
            }

            LteHarqBufferRx *currHarq = harqIt->second;
            std::vector<LteHarqProcessRx*> processes = currHarq->getProcessesRx();

            unsigned short process = 0;
            unsigned int maxProcesses = currHarq->getNumHarqProcesses();
            const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency); // get the user info
            bool breakFlag = false; // used for break if one process was selected

            //
            for (process = 0; process < maxProcesses; ++process) {
                // for each HARQ process
                LteHarqProcessRx *currentProcess = harqIt->second->getProcess(process);

                std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
                std::vector<RxUnitStatus>::iterator pit = procStatus.begin();

                for (; pit != procStatus.end(); ++pit) {
                    if (pit->second == RXHARQ_PDU_EVALUATING) {
                        neglectUe.push_back(nodeId);
                        breakFlag = true;
                        break;
                    }
                }
            }
            if (breakFlag)
                continue;
            //

            for (process = 0; process < maxProcesses; ++process) {
                LteHarqProcessRx *currentProcess = harqIt->second->getProcess(process);
                //first is codeword
                //second is status
                std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
                std::vector<RxUnitStatus>::iterator pit = procStatus.begin();
                QosHandler * qosHandler = check_and_cast<QosHandlerGNB*>(mac_->getParentModule()->getSubmodule("qosHandler"));
                for (; pit != procStatus.end(); ++pit) {
                    if (pit->second == RXHARQ_PDU_CORRUPTED) {
                        inet::Packet *pduPacket = currentProcess->getPdu(pit->first);
                        auto lteInfo = pduPacket->getTag<UserControlInfo>();
                        auto pdu = pduPacket->peekAtFront<LteMacPdu>();
//                        MacCid cid = idToMacCid(lteInfo->getSourceId(), lteInfo->getLcid());
                        MacCid cid = pdu->getMacCid();
//                        unsigned short qfi = lteInfo->getQfi();
                        unsigned short qfi = qosHandler->getQfi(cid);
                        unsigned short _5qi = qosHandler->get5Qi(qfi);
                        double prio = qosHandler->getPriority(_5qi);
                        double pdb = qosHandler->getPdb(_5qi);
                        ASSERT(pduPacket->getCreationTime() != 0);
                        simtime_t delay = NOW - pduPacket->getCreationTime();
                        simtime_t remainDelayBudget = SimTime(pdb) - delay;

                        //create a scheduleInfo
                        ScheduleInfo tmp;
                        tmp.remainDelayBudget = remainDelayBudget;
                        tmp.category = "rtx";
                        tmp.cid = cid;
                        tmp.codeword = pit->first;
                        tmp.harqProcessRx = currentProcess;
                        tmp.pdb = pdb;
                        tmp.priority = prio;
                        tmp.numberRtx = currentProcess->getTransmissions();
                        tmp.nodeId = nodeId;
                        tmp.process = process;
                        tmp.sizeOnePacketUL = pdu->getByteLength();

                        double calcPrio = lambdaPriority * (prio / 90.0)
                                + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget)
                                + lambdaCqi * (1.0 / (txParams.readCqiVector().at(0) + 1))
                                + (lambdaRtx * (1.0 / (1.0 + tmp.numberRtx)))
                                + ((70.0 / tmp.sizeOnePacketUL) * lambdaByteSize);

                        combinedMap[calcPrio].push_back(tmp);
                        //breakFlag = true;

                    }
                    else {
                        continue;
                    }
//              if (breakFlag) {
//                  break;
//              }
                }
//          if (breakFlag) {
//              break;
//          }
            }
            //
        }
    }

    //here we consider the active connections which want to send new data
    //use code from QosModel to find out most prio cid
    //get the activeSet of connections
    //find out which cid has highest priority via qosHandler
    if (racStatus_.size() != 0 && racStatusInfo_.size() != 0) {
        QosHandler *qosHandler = check_and_cast<QosHandlerGNB*>(mac_->getParentModule()->getSubmodule("qosHandler"));
        std::map<double, std::vector<QosInfo>> sortedCids = qosHandler->getEqualPriorityMap(UL);

        //create a map with cids which are in the racStatusMap
        std::map<double, std::vector<QosInfo>> racCids;
        ASSERT(racStatusInfo_.size() == racStatus_.size());
        //
        for (auto &var : sortedCids) {
            //qosinfo
            for (auto &qosinfo : var.second) {

                //find the qosinfo sortedCids and the corresponding racStatusInfo
                //racstatusinfo is a map, key is the nodeid of the ue!
                for (auto &racs : racStatusInfo_) {

                    //
                    bool conFlag = false;
                    for (auto &ueId : neglectUe) {
                        if (ueId == qosinfo.senderNodeId) {
                            conFlag = true;
                            break;
                        }
                    }
                    if (conFlag) {
                        continue;
                    }
                    //

                    MacCid realCidRac = idToMacCid(racs.second->getSourceId(), racs.second->getLcid());
                    MacCid realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid);
                    if (realCidRac == realCidQosInfo) {
                        //we have the QosInfos and the cids in the racStatus
                        racCids[var.first].push_back(qosinfo);
                    }

                }
            }
        }

        std::map<double, std::vector<QosInfo>>::iterator it = racCids.begin();
        //sorted by priority, all active cids which are compete in rac
        for (; it != racCids.end(); ++it) {

            //iterate over all cids with the same priority, find out available delay budget
            std::vector<QosInfo>::iterator qosinfoit = it->second.begin();
            for (; qosinfoit != it->second.end(); ++qosinfoit) {

                // get current nodeId
                MacCid cid = qosinfoit->cid;
                MacNodeId nodeId = MacCidToNodeId(cid);

                //
                bool conFlag = false;
                for (auto &ueId : neglectUe) {
                    if (ueId == nodeId) {
                        conFlag = true;
                        break;
                    }
                }
                if (conFlag) {
                    continue;
                }
                //

                MacCid realCid = idToMacCid(nodeId, qosinfoit->lcid);

                unsigned int bytesize = racStatusInfo_[nodeId]->getBytesize(); // the bytesize of the whole queue or single packet

                //this is the creationTime of the first packet in the buffer
                auto mac = check_and_cast<NRMacUE*>(getMacByMacNodeId(nodeId));
                auto macBuffer = mac->getMacBuffers();
                auto creationTimeFirstPacket = macBuffer->at(realCid)->getHolTimestamp();
                ASSERT(creationTimeFirstPacket != 0);
                unsigned int sizeOfOnePacketInBytes = racStatusInfo_[nodeId]->getBytesizeOfOnePacket();

                const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency); // get the user info

                //qos relevant values
                unsigned short qfi = qosinfoit->qfi;
                unsigned short _5qi = qosHandler->get5Qi(qfi);
                double prio = qosHandler->getPriority(_5qi);
                double pdb = qosHandler->getPdb(_5qi);

                //delay of this packet
                simtime_t delay = NOW - creationTimeFirstPacket;

                simtime_t remainDelayBudget = SimTime(pdb) - delay;

                //create a scheduleInfo
                ScheduleInfo tmp;
                tmp.remainDelayBudget = remainDelayBudget;
                tmp.category = "newTx";
                tmp.cid = cid;
                tmp.pdb = pdb;
                tmp.priority = prio;
                tmp.numberRtx = 0;
                tmp.nodeId = nodeId;
                tmp.bytesizeUL = bytesize;
                tmp.sizeOnePacketUL = sizeOfOnePacketInBytes;

                double calcPrio = lambdaPriority * (prio / 90.0)
                        + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget)
                        + lambdaCqi * (1.0 / (txParams.readCqiVector().at(0) + 1.0))
                        + (lambdaRtx * (1.0 / (1.0 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketUL) * lambdaByteSize);

                combinedMap[calcPrio].push_back(tmp);
            }
        }

        //this is needed to consider the bsr connections

        //here we consider the active connections which want to send new data
        //use code from QosModel to find out most prio cid
        //get the activeSet of connections
        ActiveSet activeConnectionTempSet_ = *readActiveConnections();

        //create a map with cids sorted by priority
        std::map<double, std::vector<QosInfo>> activeCids;
        for (auto &var : sortedCids) {
            for (auto &qosinfo : var.second) {
                for (auto &cid : activeConnectionTempSet_) {

                    MacNodeId nodeId = MacCidToNodeId(cid);
                    OmnetId id = binder_->getOmnetId(nodeId);

                    //
                    bool conFlag = false;
                    for (auto &ueId : neglectUe) {
                        if (ueId == nodeId) {
                            conFlag = true;
                            break;
                        }
                    }
                    if (conFlag) {
                        continue;
                    }
                    //

                    if (nodeId == 0 || id == 0) {
                        // node has left the simulation - erase corresponding CIDs
                        //activeConnectionTempSet_.erase(cid);
                        continue;
                    }

                    // check if node is still a valid node in the simulation - might have been dynamically removed
                    if (getBinder()->getOmnetId(nodeId) == 0) {
                        //activeConnectionTempSet_.erase(cid);
                        continue;
                    }

                    MacCid realCidQosInfo;
                    realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid);
                    if (realCidQosInfo == cid) {
                        //we have the QosInfos
                        activeCids[var.first].push_back(qosinfo);
                    }
                }
            }
        }

        for (auto &var : activeCids) {

            //loop over all qosinfos with same prio

            for (auto &qosinfos : var.second) {
                MacCid cid = idToMacCid(qosinfos.senderNodeId, qosinfos.lcid);
                MacNodeId nodeId = MacCidToNodeId(cid);

                //access the macbuffer to retrieve the creationTime of the pdu
                if (mac_->getMacBuffers()->find(cid) != mac_->getMacBuffers()->end()) {
                    LteMacBuffer *macBuffer = mac_->getMacBuffers()->at(cid);
                    QosHandler *qosHandler = check_and_cast<QosHandlerGNB*>(mac_->getParentModule()->getSubmodule("qosHandler"));

                    if (macBuffer->getQueueLength() <= 0) {
                        continue;
                    }

                    const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_,
                            carrierFrequency);

                    //this is the creationTime of the first packet in the buffer
                    simtime_t creationTimeFirstPacket = macBuffer->getHolTimestamp();
                    ASSERT(creationTimeFirstPacket != 0);
                    //delay of this packet
                    simtime_t delay = NOW - creationTimeFirstPacket;

                    //get the pdb and prio of this cid
                    unsigned short qfi = qosinfos.qfi;
                    unsigned short _5qi = mac_->getQosHandler()->get5Qi(qfi);
                    double prio = mac_->getQosHandler()->getPriority(_5qi);
                    double pdb = mac_->getQosHandler()->getPdb(_5qi);

                    simtime_t remainDelayBudget = SimTime(pdb) - delay;

                    ScheduleInfo tmp;
                    tmp.nodeId = nodeId;
                    tmp.remainDelayBudget = remainDelayBudget;
                    tmp.category = "newTx";
                    tmp.cid = cid;
                    tmp.pdb = pdb;
                    tmp.priority = prio;
                    tmp.numberRtx = 0;
                    tmp.nodeId = qosinfos.destNodeId;

                    tmp.bytesizeDL = macBuffer->getQueueOccupancy();
                    tmp.sizeOnePacketDL = macBuffer->front().first;

                    double calcPrio = lambdaPriority * (prio / 90.0)
                            + ((remainDelayBudget.dbl() / 0.5) * lambdaRemainDelayBudget)
                            + lambdaCqi * (1 / (txParams.readCqiVector().at(0) + 1))
                            + (lambdaRtx * (1 / (1 + tmp.numberRtx))) + ((70.0 / tmp.sizeOnePacketDL) * lambdaByteSize);

                    combinedMap[calcPrio].push_back(tmp);
                }
            }
        }
    }

    //schedule
    std::map<MacNodeId, std::vector<ScheduledInfo>> scheduledInfoMap;

    if (combineQosWithRac) {

        //how many resources available for all?
        //how many resources needed per priority?
        //ensure that highest priority is fully satisfied
        //go to next prio
        //key --> calculated weight
        for (auto &prio : combinedMap) {

            int countedRtx = 0;

            //schedule only the rtxs first (all have same prio)
            for (auto &schedInfo : prio.second) {

                //only the connection with the highest Prio of the same UE gets resources
                if (scheduledInfoMap[schedInfo.nodeId].size() == 1) {
                    continue;
                }

                if (schedInfo.category == "rtx") {

                    unsigned int rtxBytes = schedulePerAcidRtx(schedInfo.nodeId, carrierFrequency, schedInfo.codeword,
                            schedInfo.process);

                    ScheduledInfo tmp;
                    tmp.nodeId = schedInfo.nodeId;
                    tmp.info = schedInfo;
                    scheduledInfoMap[schedInfo.nodeId].push_back(tmp);
                    countedRtx++;

                }

            }

            int blocks = allocator_->computeTotalRbs();
            //values --> vector with ScheduleInfo with the same weight, mix of rtx and newTx
            int numberOfSchedInfosWithSamePrio = prio.second.size();
            int newTxWithEqualPrio = numberOfSchedInfosWithSamePrio - countedRtx;

            if (newTxWithEqualPrio <= 0) {
                continue;
            }

            //share the available blocks in a fair manner
            unsigned int blocksPerSchedInfo = blocks / newTxWithEqualPrio;

            //than schedule newRtx
            for (auto &schedInfo : prio.second) {

                //only the connection with the highest Prio of the same UE gets resources
                if (scheduledInfoMap[schedInfo.nodeId].size() == 1) {
                    continue;
                }

                if (schedInfo.category == "newTx") {
                    MacCid cid = schedInfo.cid;
                    MacNodeId nodeId = schedInfo.nodeId;
                    unsigned int numBands = mac_->getCellInfo()->getNumBands();
                    const unsigned int cw = 0;
                    bool allocation = false;
                    blocks = allocator_->computeTotalRbs();
                    unsigned int reqBlocks = 0;
                    unsigned int reqBlocksTmp = 0;
                    unsigned int bytes = 0;
                    unsigned int bytesize = schedInfo.bytesizeUL;    //bytes UE want to send
                    unsigned int sizePerPacket = schedInfo.sizeOnePacketUL; // bytes for one Packet
                    unsigned int schedBytesPerSchedInfo = 0;

                    if (blocks > 0) {

                        //real required blocks
                        if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
                            reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, cw, sizePerPacket, UL, blocks,
                                    carrierFrequency); //required Blocks one Packet
                        }
                        else {
                            reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, cw, bytesize, UL, blocks,
                                    carrierFrequency); //required Blocks whole Qeue
                        }

                        reqBlocksTmp = reqBlocks;
                        //real required bytes
                        //consider all bands --> band set to 0
                        bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, 0, cw, reqBlocks, UL, carrierFrequency); //required Bytes
                        //shared available blocks
                        schedBytesPerSchedInfo = mac_->getAmc()->computeBytesOnNRbs(nodeId, 0, cw, blocksPerSchedInfo, UL, carrierFrequency); //required Bytes

                        //required blocks fit the size of the shared blocks
                        if (reqBlocks <= blocksPerSchedInfo) {
                            for (unsigned int i = 0; i < numBands; i++) {
                                if (allocator_->availableBlocks(nodeId, MACRO, i) == 1) {
                                    allocator_->addBlocks(MACRO, i, nodeId, 1, bytes / reqBlocks);
                                    reqBlocksTmp--;
                                }
                                if (reqBlocksTmp == 0) {
                                    allocation = true;
                                    break;
                                }
                            }
                        }
                        else {
                            //required blocks larger, schedule only the shared blocks
                            reqBlocks = blocksPerSchedInfo;
                            reqBlocksTmp = reqBlocks;
                            bytes = schedBytesPerSchedInfo;

                            for (unsigned int i = 0; i < numBands; i++) {
                                if (allocator_->availableBlocks(nodeId, MACRO, i) == 1) {
                                    allocator_->addBlocks(MACRO, i, nodeId, 1, bytes / reqBlocks);
                                    reqBlocksTmp--;
                                }
                                if (reqBlocksTmp == 0) {
                                    allocation = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (allocation) {

                        // create scList id for current cid/codeword
                        MacCid cidBsr = idToMacCid(nodeId, SHORT_BSR); // build the cid. Since this grant will be used for a BSR,
                                                                    // we use the LCID corresponding to the SHORT_BSR
                        std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
                        scheduleList_[carrierFrequency][scListId] = reqBlocks;

                        LteMacBufferMap::iterator bsr = bsrbuf_->find(cidBsr);

                        if (!(bsr->second->getQueueLength() <= 0)) {
                            if (bsr->second->front().first - bytes <= 0) {
                                activeConnectionSet_.erase(cidBsr);
                            }
                        }
                    }

                    ScheduledInfo tmp;
                    tmp.nodeId = schedInfo.nodeId;
                    tmp.info = schedInfo;
                    scheduledInfoMap[schedInfo.nodeId].push_back(tmp);

                }
            }
        }

    }
    else {
        //key --> calculated weight
        for (auto &var : combinedMap) {

            //values --> vector with ScheduleInfo with the same weight
            for (auto &vec : var.second) {

                //only the connection with the highest Prio of the same UE gets resources
                if (scheduledInfoMap[vec.nodeId].size() == 1) {
                    continue;
                }

                if (vec.category == "rtx") {

                    unsigned int rtxBytes = schedulePerAcidRtx(vec.nodeId, carrierFrequency, vec.codeword, vec.process);

                }
                else if (vec.category == "newTx") {

                    MacCid cid = vec.cid;
                    MacNodeId nodeId = vec.nodeId;
                    unsigned int numBands = mac_->getCellInfo()->getNumBands();
                    const unsigned int cw = 0;
                    bool allocation = false;
                    unsigned int blocks = allocator_->computeTotalRbs();
                    unsigned int reqBlocks = 0;
                    unsigned int reqBlocksTmp = 0;
                    unsigned int bytes = 0;
                    unsigned int bytesize = vec.bytesizeUL;    //bytes UE want to send
                    unsigned int sizePerPacket = vec.sizeOnePacketUL;

                        if (blocks > 0) {

                            if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
                                reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, cw, sizePerPacket, UL, blocks, carrierFrequency); //required Blocks for packet
                            }
                            else {
                                reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, cw, bytesize, UL, blocks, carrierFrequency); //required Blocks for Queue
                            }

                            reqBlocksTmp =  reqBlocks;
                            bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, 0, cw, reqBlocks, UL, carrierFrequency); //required Bytes

                            if (bytes > 0) {

                                for (unsigned int i = 1; i < numBands; i++) {
                                if (allocator_->availableBlocks(nodeId, MACRO, i) == 1) {
                                    allocator_->addBlocks(MACRO, i, nodeId, 1, bytes / reqBlocks);
                                    reqBlocksTmp--;
                                }
                                if (reqBlocksTmp == 0) {
                                    allocation = true;
                                    break;
                                }
                            }

                            }
                        }


                    if (allocation) {

                        // create scList id for current cid/codeword
                        MacCid cidBsr = idToMacCid(nodeId, SHORT_BSR); // build the cid. Since this grant will be used for a BSR,
                                                                    // we use the LCID corresponding to the SHORT_BSR
                        std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
                        scheduleList_[carrierFrequency][scListId] = reqBlocks;

                        LteMacBufferMap::iterator bsr = bsrbuf_->find(cidBsr);

                        if (!(bsr->second->getQueueLength() <= 0)) {
                            if (bsr->second->front().first - bytes <= 0) {
                                activeConnectionSet_.erase(cidBsr);
                            }
                        }
                    }

                    //EV << NOW << " NRSchedulerGnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

                    int availableBlocks = allocator_->computeTotalRbs();

                }
                else {
                    throw cRuntimeError("Error");
                }
                ScheduledInfo tmp;
                tmp.nodeId = vec.nodeId;
                tmp.info = vec;
                scheduledInfoMap[vec.nodeId].push_back(tmp);
            }
        }
    }
    //cleanUp
    // clean up all requests
    for (auto &var : racStatusInfo_) {
        delete var.second;
    }
    racStatus_.clear();
    racStatusInfo_.clear();

    //std::cout << "NRSchedulerGnbUL::qosModelSchedule end at " << simTime().dbl() << std::endl;
}

unsigned int NRSchedulerGnbUL::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw,
        unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    //std::cout << "NRSchedulerGnbUL::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

    //TODO check if code has to be overwritten
    //return NRSchedulerGnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim, antenna, limitBl);

    try {
        const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency); // get the user info
        const std::set<Band> &allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        tempBandLim.clear();
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr) {
            // Create a vector of band limit using all bands
            // FIXME: bandlim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        elem.limit_[j] = -1;
                    }
                    else {
                        elem.limit_[j] = -2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit &elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        elem.limit_[j] = -1;
                    }
                    else {
                        elem.limit_[j] = -2;
                    }
                }
            }
        }

        LteHarqProcessRx *currentProcess = harqRxBuffers_->at(carrierFrequency).at(nodeId)->getProcess(acid);

        if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
            // exit if the current active HARQ process is not ready for retransmission
            return 0;
        }

        Codeword allocatedCw = 0;
        // search for already allocated codeword
        // create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(
                idToMacCid(nodeId, SHORT_BSR), MAX_CODEWORDS - cw - 1);
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end()) {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end()) {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getByteLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i) {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support to multi CW
            //            unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
            //                    ((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe) {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
            }
            unsigned int servedBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, servedBytes, direction_,
                    carrierFrequency);
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0) {
            // process couldn't be served - no sufficient space on available bands
            return 0;
        }
        else {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks = 0;

            // create scList id for current node/codeword
            std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(
                    idToMacCid(nodeId, SHORT_BSR), cw);

            for (unsigned int i = 0; i < size; ++i) {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks += assignedBlocks.at(i);
                //! handle multi-codeword allocation
                if (allocatedCw != MAX_CODEWORDS) {
                    allocator_->addBlocks(antenna, b, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks

            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;

            check_and_cast<NRMacGNB*>(mac_)->insertRtxMap(nodeId, acid, cw);

            // mark codeword as used
            if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
                allocatedCws_.at(nodeId)++;}
            else {
                allocatedCws_[nodeId] = 1;
            }

            return bytes;
        }
    }
    catch (std::exception &e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxAcid(): %s", e.what());
    }
    return 0;
}

void NRSchedulerGnbUL::removePendingRac(MacNodeId nodeId)
{
    //std::cout << "NRSchedulerGnbUl::removePendingRac start at " << simTime().dbl() << std::endl;

    racStatus_.erase(nodeId);
    delete racStatusInfo_[nodeId];
    racStatusInfo_.erase(nodeId);

    //std::cout << "NRSchedulerGnbUl::removePendingRac end at " << simTime().dbl() << std::endl;
}

void NRSchedulerGnbUL::initialize(Direction dir, LteMacEnb *mac)
{
    //std::cout << "NRSchedulerGnbUl::initialize start at " << simTime().dbl() << std::endl;

    newTxbeforeRtx = getSimulation()->getSystemModule()->par("newTxbeforeRtx").boolValue();
    useQosModel = getSimulation()->getSystemModule()->par("useQosModel").boolValue();
    fairSchedule = getSimulation()->getSystemModule()->par("fairRacScheduleInUL").boolValue();
    combineQosWithRac = getSimulation()->getSystemModule()->par("combineQosWithRac").boolValue();

    direction_ = dir;
    mac_ = check_and_cast<NRMacGNB*>(mac);

    binder_ = getBinder();

    vbuf_ = mac_->getMacBuffers();
    bsrbuf_ = mac_->getBsrVirtualBuffers();

    harqTxBuffers_ = mac_->getHarqTxBuffers();
    harqRxBuffers_ = mac_->getHarqRxBuffers();

    // Create LteScheduler. One per carrier
    SchedDiscipline discipline = mac_->getSchedDiscipline(direction_);

    if (useQosModel) {

        for(auto & var : scheduler_){
            delete var;
        }
        scheduler_.clear();

        const CarrierInfoMap *carriers = mac_->getCellInfo()->getCarrierInfoMap();
        CarrierInfoMap::const_iterator it = carriers->begin();
        for (; it != carriers->end(); ++it) {
            LteScheduler *newSched = new NRQoSModel(UL);
            newSched->setEnbScheduler(this);
            newSched->setCarrierFrequency(it->second.carrierFrequency);
            newSched->setNumerologyIndex(it->second.numerologyIndex); // set periodicity for this scheduler according to numerology
            scheduler_.push_back(newSched);
        }
    }
    else {

        for(auto & var : scheduler_){
            delete var;
        }
        scheduler_.clear();

        LteScheduler *newSched = NULL;
        const CarrierInfoMap *carriers = mac_->getCellInfo()->getCarrierInfoMap();
        CarrierInfoMap::const_iterator it = carriers->begin();
        for (; it != carriers->end(); ++it) {
            newSched = getScheduler(discipline);
            newSched->setEnbScheduler(this);
            newSched->setCarrierFrequency(it->second.carrierFrequency);
            newSched->setNumerologyIndex(it->second.numerologyIndex); // set periodicity for this scheduler according to numerology
            scheduler_.push_back(newSched);
        }
    }

    // Create Allocator
    if (discipline == ALLOCATOR_BESTFIT) // NOTE: create this type of allocator for every scheduler using Frequency Reuse
        allocator_ = new LteAllocationModuleFrequencyReuse(mac_, direction_);
    else
        allocator_ = new LteAllocationModule(mac_, direction_);

    // Initialize statistics
    cellBlocksUtilizationDl_ = mac_->registerSignal("cellBlocksUtilizationDl");
    cellBlocksUtilizationUl_ = mac_->registerSignal("cellBlocksUtilizationUl");
    lteAvgServedBlocksDl_ = mac_->registerSignal("avgServedBlocksDl");
    lteAvgServedBlocksUl_ = mac_->registerSignal("avgServedBlocksUl");

    //std::cout << "NRSchedulerGnbUl::initialize end at " << simTime().dbl() << std::endl;
}
