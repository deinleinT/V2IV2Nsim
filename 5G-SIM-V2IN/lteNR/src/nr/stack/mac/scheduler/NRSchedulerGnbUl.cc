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

#include "nr/stack/mac/scheduler/NRSchedulerGnbUl.h"

LteMacScheduleListWithSizes* NRSchedulerGnbUl::schedule() {
	//std::cout << "NRSchedulerGnbUl::schedule start at " << simTime().dbl() << std::endl;

	//EV << "NRSchedulerGnbUl::schedule performed by Node: " << mac_->getMacNodeId() << endl;

	// clearing structures for new scheduling
	scheduleList_.clear();
	allocatedCws_.clear(); //allocatedCwsNodeCid_.clear();

	// clean the allocator
	initAndResetAllocator();
	//reset AMC structures
	mac_->getAmc()->cleanAmcStructures(direction_, scheduler_->readActiveSet());

	// scheduling of retransmission and transmission
	//EV << "___________________________start RTX __________________________________" << endl;
	if (!(scheduler_->scheduleRetransmissions())) {
		//EV << "____________________________ end RTX __________________________________" << endl;
		//EV << "___________________________start SCHED ________________________________" << endl;
		scheduler_->updateSchedulingInfo();
		scheduler_->schedule();

		//EV << "____________________________ end SCHED ________________________________" << endl;
	}

	// record assigned resource blocks statistics
	resourceBlockStatistics();

	//std::cout << "NRSchedulerGnbUl::schedule end at " << simTime().dbl() << std::endl;

	return &scheduleList_;
}

bool NRSchedulerGnbUl::racschedule() {
	//std::cout << "NRSchedulerGnbUl::racschedule start at " << simTime().dbl() << std::endl;

	//EV << NOW << " NRSchedulerGnbUl::racschedule --------------------::[ START RAC-SCHEDULE ]::--------------------" << endl;
	//EV << NOW << " NRSchedulerGnbUl::racschedule eNodeB: " << mac_->getMacCellId() << endl;
	//EV << NOW << " NRSchedulerGnbUl::racschedule Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

	RacStatus::iterator it = racStatus_.begin(), et = racStatus_.end();

	for (; it != et; ++it) {

		// get current nodeId
		MacNodeId nodeId = it->first;
		//EV << NOW << " NRSchedulerGnbUl::racschedule handling RAC for node " << nodeId << endl;

		// Get number of logical bands
		unsigned int numBands = mac_->getCellInfo()->getNumBands();
		const unsigned int cw = 0;
		bool allocation = false;
		unsigned int blocks = 0;
		unsigned int reqBlocks = 0;
		unsigned int sumReqBlocks = 0;
		unsigned int bytes = 0;
		unsigned int sumBytes = 0;
		unsigned int bytesize = racStatusInfo_[nodeId]->getBytesize();    //bytes UE want to send
		int restBytes = bytesize;

		for (Band b = 0; b < numBands; ++b) {
			blocks = allocator_->availableBlocks(nodeId, MACRO, b); //in this band available blocks

			if (blocks > 0) {
				reqBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bytesize, UL, blocks); //required Blocks
				sumReqBlocks += reqBlocks;
				bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, reqBlocks, UL); //required Bytes
				sumBytes += bytes;

				if (bytes > 0) {

					allocator_->addBlocks(MACRO, b, nodeId, reqBlocks, bytes);

					restBytes -= bytes;
					if (restBytes <= 0 || reqBlocks == blocks) {
						allocation = true;
						break;
					}
				}
			}
		}

		if (allocation) {
			// create scList id for current cid/codeword
			//MacCid cid = racStatusInfo_[nodeId]->getCid();
			MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
			// we use the LCID corresponding to the SHORT_BSR
			std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(cid, cw);
			scheduleList_[scListId].first = sumReqBlocks;
			scheduleList_[scListId].second = sumBytes;
			allocation = false;
		}
	}

	// clean up all requests
	for (auto var : racStatusInfo_) {
		delete var.second;
	}
	racStatus_.clear();
	racStatusInfo_.clear();

	//EV << NOW << " NRSchedulerGnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

	int availableBlocks = allocator_->computeTotalRbs();

	//std::cout << "NRSchedulerGnbUl::racschedule end at " << simTime().dbl() << std::endl;

	return (availableBlocks == 0);
}

/**
 * changed the behaviour
 * racschedule is called after checking whether a rtx is needed
 */
bool NRSchedulerGnbUl::rtxschedule() {

	//std::cout << "NRSchedulerGnbUl::rtxschedule start at " << simTime().dbl() << std::endl;

	// try to handle RAC requests first and abort rtx scheduling if no OFDMA space is left after

	bool rtxNeeded = false;
    try
    {
//        EV << NOW << " NRSchedulerGnbUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
//        EV << NOW << " NRSchedulerGnbUl::rtxschedule eNodeB: " << mac_->getMacCellId() << endl;
//        EV << NOW << " NRSchedulerGnbUl::rtxschedule Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        HarqRxBuffers::iterator it= harqRxBuffers_->begin() , et=harqRxBuffers_->end();

        for(; it != et; ++it)
        {
            // get current nodeId
            MacNodeId nodeId = it->first;

            if(nodeId == 0){
                // UE has left the simulation - erase queue and continue
                harqRxBuffers_->erase(nodeId);
                continue;
            }
            OmnetId id = binder_->getOmnetId(nodeId);
            if(id == 0){
                harqRxBuffers_->erase(nodeId);
                continue;
            }

            // get current Harq Process for nodeId
            unsigned char currentAcid = harqStatus_.at(nodeId);

            // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
            bool skip = true;
            unsigned char acid = (currentAcid + 2) % (it->second->getProcesses());
            LteHarqProcessRx* currentProcess = it->second->getProcess(acid);
            std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
            std::vector<RxUnitStatus>::iterator pit = procStatus.begin();
            for (; pit != procStatus.end(); ++pit )
            {
                if (pit->second == RXHARQ_PDU_CORRUPTED)
                {
                    skip = false;

                    break;
                }
            }
            if (skip)
                continue;

            EV << NOW << "LteSchedulerEnbUl::rtxschedule UE: " << nodeId << "Acid: " << (unsigned int)currentAcid << endl;

            // Get user transmission parameters
            const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);// get the user info

            unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
            unsigned int allocatedBytes =0;

            // TODO handle the codewords join case (sizeof(cw0+cw1) < currentTbs && currentLayers ==1)

            for(Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords>0); ++cw)
            {
                unsigned int rtxBytes=0;
                // FIXME PERFORMANCE: check for rtx status before calling rtxAcid

                // perform a retransmission on available codewords for the selected acid
                rtxBytes=NRSchedulerGnbUl::schedulePerAcidRtx(nodeId, cw,currentAcid);
                if (rtxBytes>0)
                {
                    --codewords;
                    allocatedBytes+=rtxBytes;
                    rtxNeeded = true;
                }
            }
            EV << NOW << "LteSchedulerEnbUl::rtxschedule user " << nodeId << " allocated bytes : " << allocatedBytes << endl;
        }
        if(rtxNeeded)
        	return true;

//        if (mac_->isD2DCapable())
//        {
//            // --- START Schedule D2D retransmissions --- //
//            Direction dir = D2D;
//            HarqBuffersMirrorD2D* harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D*>(mac_)->getHarqBuffersMirrorD2D();
//            HarqBuffersMirrorD2D::iterator it_d2d = harqBuffersMirrorD2D->begin() , et_d2d=harqBuffersMirrorD2D->end();
//            for(; it_d2d != et_d2d; ++it_d2d)
//            {
//
//                // get current nodeIDs
//                MacNodeId senderId = (it_d2d->first).first; // Transmitter
//                MacNodeId destId = (it_d2d->first).second;  // Receiver
//
//                // get current Harq Process for nodeId
//                unsigned char currentAcid = harqStatus_.at(senderId);
//
//                // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
//                bool skip = true;
//                unsigned char acid = (currentAcid + 2) % (it_d2d->second->getProcesses());
//                LteHarqProcessMirrorD2D* currentProcess = it_d2d->second->getProcess(acid);
//                std::vector<TxHarqPduStatus> procStatus = currentProcess->getProcessStatus();
//                std::vector<TxHarqPduStatus>::iterator pit = procStatus.begin();
//                for (; pit != procStatus.end(); ++pit )
//                {
//                    if (*pit == TXHARQ_PDU_BUFFERED)
//                    {
//                        skip = false;
//                        break;
//                    }
//                }
//                if (skip)
//                    continue;
//
//                EV << NOW << " LteSchedulerEnbUl::rtxschedule - D2D UE: " << senderId << " Acid: " << (unsigned int)currentAcid << endl;
//
//                // Get user transmission parameters
//                const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir);// get the user info
//
//                unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
//                unsigned int allocatedBytes =0;
//
//                // TODO handle the codewords join case (size of(cw0+cw1) < currentTbs && currentLayers ==1)
//
//                for(Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords>0); ++cw)
//                {
//                    unsigned int rtxBytes=0;
//                    // FIXME PERFORMANCE: check for rtx status before calling rtxAcid
//
//                    // perform a retransmission on available codewords for the selected acid
//                    rtxBytes = LteSchedulerEnbUl::schedulePerAcidRtxD2D(destId, senderId, cw, currentAcid);
//                    if (rtxBytes>0)
//                    {
//                        --codewords;
//                        allocatedBytes+=rtxBytes;
//                    }
//                }
//                EV << NOW << " LteSchedulerEnbUl::rtxschedule - D2D UE: " << senderId << " allocated bytes : " << allocatedBytes << endl;
//
//            }
//            // --- END Schedule D2D retransmissions --- //
//        }

        racschedule();

        int availableBlocks = allocator_->computeTotalRbs();

        //EV << NOW << " NRSchedulerGnbUl::rtxschedule residual OFDM Space: " << availableBlocks << endl;

        //EV << NOW << " NRSchedulerGnbUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

        return (availableBlocks == 0);
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in NRSchedulerEnbUl::rtxschedule(): %s", e.what());
    }
    return 0;

}

unsigned int NRSchedulerGnbUl::schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	try {
		std::string bands_msg = "BAND_LIMIT_SPECIFIED";
		if (bandLim == NULL) {
			bands_msg = "NO_BAND_SPECIFIED";
			// Create a vector of band limit using all bands
			// FIXME: bandlim is never deleted
			bandLim = new std::vector<BandLimit>();

			unsigned int numBands = mac_->getCellInfo()->getNumBands();
			// for each band of the band vector provided
			for (unsigned int i = 0; i < numBands; i++) {
				BandLimit elem;
				// copy the band
				elem.band_ = Band(i);
				//EV << "Putting band " << i << endl;
				// mark as unlimited
				for (Codeword i = 0; i < MAX_CODEWORDS; ++i) {
					elem.limit_.push_back(-1);
				}
				bandLim->push_back(elem);
			}
		}

		//EV << NOW << "NRSchedulerGnbUl::rtxAcid - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

		// Get the current active HARQ process
//        unsigned char currentAcid = harqStatus_.at(nodeId) ;

		unsigned char currentAcid = (harqStatus_.at(nodeId) + 2) % (harqRxBuffers_->at(nodeId)->getProcesses());
		//EV << "\t the acid that should be considered is " << currentAcid << endl;

		LteHarqProcessRx *currentProcess = harqRxBuffers_->at(nodeId)->getProcess(currentAcid);

		if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
			// exit if the current active HARQ process is not ready for retransmission
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid User is on ACID " << (unsigned int)currentAcid << " HARQ process is IDLE. No RTX scheduled ." << endl;
			delete (bandLim);
			return 0;
		}

		Codeword allocatedCw = 0;
//        Codeword allocatedCw = MAX_CODEWORDS;
		// search for already allocated codeword
		// create "mirror" scList ID for other codeword than current
		std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), MAX_CODEWORDS - cw - 1);
		if (scheduleList_.find(scListMirrorId) != scheduleList_.end()) {
			allocatedCw = MAX_CODEWORDS - cw - 1;
		}
		// get current process buffered PDU byte length
		unsigned int bytes = currentProcess->getByteLength(cw);
		// bytes to serve
		int toServe = bytes;
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
			unsigned int availableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
			unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_);

			// use the provided limit as cap for available bytes, if it is not set to unlimited
			if (limit >= 0)
				bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

			//EV << NOW << " NRSchedulerGnbUl::rtxAcid BAND " << b << endl;
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid Available: " << bandAvailableBytes << " bytes" << endl;

			unsigned int servedBytes = 0;
			// there's no room on current band for serving the entire request
			if (bandAvailableBytes < toServe) {
				// record the amount of served bytes
				servedBytes = bandAvailableBytes;
				// the request can be fully satisfied
			} else {
				// record the amount of served bytes
				servedBytes = toServe;
				// signal end loop - all data have been serviced
				finish = true;
			}

			unsigned int servedBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, servedBytes, direction_, availableBlocks);
//			if (servedBlocks + 2 <= availableBlocks)
//				servedBlocks += 2;
			servedBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, servedBlocks, UL);
			//

			// update the bytes counter
			toServe -= servedBytes;
			// update the structures
			assignedBlocks.push_back(servedBlocks);
			assignedBytes.push_back(servedBytes);
		}

		if (toServe > 0) {
			// process couldn't be served - no sufficient space on available bands
			//EV << NOW << " NRSchedulerGnbUl::rtxAcid Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)currentAcid << " on codeword " << cw << endl;
			delete (bandLim);
			return 0;
		} else {
			// record the allocation
			unsigned int size = assignedBlocks.size();
			unsigned int cwAllocatedBlocks = 0;
			unsigned int cwAllocatedBytes = 0;

			// create scList id for current node/codeword
			std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), cw);

			for (unsigned int i = 0; i < size; ++i) {
				// For each LB for which blocks have been allocated
				Band b = bandLim->at(i).band_;

				cwAllocatedBlocks += assignedBlocks.at(i);
				cwAllocatedBytes += assignedBytes.at(i);
				//EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
				//! handle multi-codeword allocation
				if (allocatedCw != MAX_CODEWORDS) {
					//EV << NOW << " NRSchedulerGnbUl::rtxAcid - adding " << assignedBlocks.at(i) << " to band " << i << endl;
					allocator_->addBlocks(antenna, b, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
				}
				//! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
			}

			// signal a retransmission
			// schedule list contains number of granted blocks

			scheduleList_[scListId].first = cwAllocatedBlocks;
			scheduleList_[scListId].second = cwAllocatedBytes;
			bytes = cwAllocatedBytes;

			mac_->insertRtxMap(nodeId, currentAcid);

			// mark codeword as used
			if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
				allocatedCws_.at(nodeId)++;}
			else {
				allocatedCws_[nodeId] = 1;
			}

			//EV << NOW << " NRSchedulerGnbUl::rtxAcid HARQ Process " << (unsigned int)currentAcid << " : " << bytes << " bytes served! " << endl;

			delete (bandLim);

			//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

			return bytes;
		}
	} catch (std::exception &e) {
		throw cRuntimeError("Exception in NRSchedulerGnbUl::rtxAcid(): %s", e.what());
	}
	delete (bandLim);

	//std::cout << "NRSchedulerGnbUl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

	return 0;
}

