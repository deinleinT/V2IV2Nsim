//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU),
// Computer Science 7 - Computer Networks and Communication Systems
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

#include "NRSchedulerGnbDl.h"

NRSchedulerGnbDl::NRSchedulerGnbDl() {
	// TODO Auto-generated constructor stub

}

NRSchedulerGnbDl::~NRSchedulerGnbDl() {
	// TODO Auto-generated destructor stub
}

LteMacScheduleListWithSizes* NRSchedulerGnbDl::schedule() {
	//std::cout << "NRSchedulerGnbDl::schedule start at " << simTime().dbl() << std::endl;

	//EV << "NRSchedulerGnbDl::schedule performed by Node: " << mac_->getMacNodeId() << endl;

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

	//std::cout << "NRSchedulerGnbDl::schedule end at " << simTime().dbl() << std::endl;

	return &scheduleList_;
}

bool NRSchedulerGnbDl::rtxschedule() {
	//std::cout << "NRSchedulerGnbDl::rtxschedule start at " << simTime().dbl() << std::endl;

	// retrieving reference to HARQ entities
	HarqTxBuffers *harqQueues = mac_->getHarqTxBuffers();

	HarqTxBuffers::iterator it = harqQueues->begin();
	HarqTxBuffers::iterator et = harqQueues->end();

	std::vector<BandLimit> usableBands;

	// examination of HARQ process in rtx status, adding them to scheduling list
	for (; it != et; ++it) {
		// For each UE
		MacNodeId nodeId = it->first;

		OmnetId id = binder_->getOmnetId(nodeId);
		if (id == 0) {
			// UE has left the simulation, erase HARQ-queue
			it = harqQueues->erase(it);
			if (it == et)
				break;
			else
				continue;
		}
		LteHarqBufferTx *currHarq = it->second;
		std::vector<LteHarqProcessTx*> *processes = currHarq->getHarqProcesses();

		// Get user transmission parameters
		const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);        // get the user info
		// TODO SK Get the number of codewords - FIX with correct mapping
		unsigned int codewords = txParams.getLayers().size();        // get the number of available codewords

		//EV << NOW << " NRSchedulerGnbDl::rtxschedule  UE: " << nodeId << endl;
		//EV << NOW << " NRSchedulerGnbDl::rtxschedule Number of codewords: " << codewords << endl;
		unsigned int process = 0;
		unsigned int maxProcesses = currHarq->getNumProcesses();
		for (process = 0; process < maxProcesses; ++process) {
			// for each HARQ process
			LteHarqProcessTx *currProc = (*processes)[process];

			if (allocatedCws_[nodeId] == codewords)
				break;
			for (Codeword cw = 0; cw < codewords; ++cw) {
				if (allocatedCws_[nodeId] == codewords)
					break;
				//EV << NOW << " NRSchedulerGnbDl::rtxschedule process " << process << endl;
				//EV << NOW << " NRSchedulerGnbDl::rtxschedule ------- CODEWORD " << cw << endl;

				// skip processes which are not in rtx status
				if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED) {
					//EV << NOW << " NRSchedulerGnbDl::rtxschedule detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
					continue;
				}

				//EV << NOW << " NRSchedulerGnbDl::rtxschedule " << endl;
				//EV << NOW << " NRSchedulerGnbDl::rtxschedule detected RTX Acid: " << process << endl;

				// Get the bandLimit for the current user
				std::vector<BandLimit> *bandLim;
				bool ret = getBandLimit(&usableBands, nodeId);
				if (!ret)
					bandLim = NULL;
				else
					bandLim = &usableBands;

				// perform the retransmission
				unsigned int bytes = schedulePerAcidRtx(nodeId, cw, process, bandLim);

				// if a value different from zero is returned, there was a service
				if (bytes > 0) {
					//EV << NOW << " NRSchedulerGnbDl::rtxschedule CODEWORD IS NOW BUSY!!!" << endl;
					// do not process this HARQ process anymore
					// go to next codeword
					break;
				}
			}
		}
	}

	unsigned int availableBlocks = allocator_->computeTotalRbs();

	//std::cout << "NRSchedulerGnbDl::rtxschedule end at " << simTime().dbl() << std::endl;

	return (availableBlocks == 0);
}

unsigned int NRSchedulerGnbDl::schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbDl::schedulePerAcidRtx start at " << simTime().dbl() << std::endl;

	// Get user transmission parameters
	const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);    // get the user info
	// TODO SK Get the number of codewords - FIX with correct mapping
	unsigned int codewords = txParams.getLayers().size();                // get the number of available codewords

	std::string bands_msg = "BAND_LIMIT_SPECIFIED";

	std::vector<BandLimit> tempBandLim;

	Codeword remappedCw = (codewords == 1) ? 0 : cw;

	if (bandLim == NULL) {
		bands_msg = "NO_BAND_SPECIFIED";
		// Create a vector of band limit using all bands
		bandLim = &tempBandLim;

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
	//EV << NOW << "NRSchedulerGnbDl::rtxAcid - Node [" << mac_->getMacNodeId() << "], User[" << nodeId << "],  Codeword [" << cw << "]  of [" << codewords << "] , ACID [" << (int)acid << "] " << endl;
	//! \test REALISTIC!!!  Multi User MIMO support
	if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER)) {
		// request amc for MU_MIMO pairing
		MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId);
		if (peer != nodeId) {
			// this user has a valid pairing
			//1) register pairing  - if pairing is already registered false is returned
			if (allocator_->configureMuMimoPeering(nodeId, peer)) {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
			} else {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
			}
		} else {
			//EV << "NRSchedulerGnbDl::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
		}
	}
	//!\test experimental DAS support
	// registering DAS spaces to the allocator
	Plane plane = allocator_->getOFDMPlane(nodeId);
	allocator_->setRemoteAntenna(plane, antenna);

	// blocks to allocate for each band
	std::vector<unsigned int> assignedBlocks;
	// bytes which blocks from the preceding vector are supposed to satisfy
	std::vector<unsigned int> assignedBytes;
	LteHarqBufferTx *currHarq = mac_->getHarqTxBuffers()->at(nodeId);

	// bytes to serve
	unsigned int bytes = currHarq->pduLength(acid, cw);

	// check selected process status.
	std::vector<UnitStatus> pStatus = currHarq->getProcess(acid)->getProcessStatus();
	std::vector<UnitStatus>::iterator vit = pStatus.begin(), vet = pStatus.end();

	Codeword allocatedCw = 0;
	// search for already allocated codeword

	if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
		allocatedCw = allocatedCws_.at(nodeId);

	}
	// for each band
	unsigned int size = bandLim->size();

	for (unsigned int i = 0; i < size; ++i) {
		// save the band and the relative limit
		Band b = bandLim->at(i).band_;
		int limit = bandLim->at(i).limit_.at(remappedCw);

		//EV << "NRSchedulerGnbDl::schedulePerAcidRtx --- BAND " << b << " LIMIT " << limit << "---" << endl;
		// if the limit flag is set to skip, jump off
		if (limit == -2) {
			//EV << "LteSchedulerEnbDl::schedulePerAcidRtx - skipping logical band according to limit value" << endl;
			continue;
		}

		unsigned int available = 0;
		// if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
		if ((allocatedCw != 0)) {
			// get band allocated blocks
			int b1 = allocator_->getBlocks(antenna, b, nodeId);

			// limit eventually allocated blocks on other codeword to limit for current cw
			//b1 = (limitBl ? (b1>limit?limit:b1) : b1);
			available = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, remappedCw, b1, direction_);
		} else
			available = availableBytes(nodeId, antenna, b, remappedCw, direction_, (limitBl) ? limit : -1);    // available space

		// use the provided limit as cap for available bytes, if it is not set to unlimited
		if (limit >= 0 && !limitBl)
			available = limit < (int) available ? limit : available;

		//EV << NOW << "NRSchedulerGnbDl::rtxAcid ----- BAND " << b << "-----" << endl;
		//EV << NOW << "NRSchedulerGnbDl::rtxAcid To serve: " << bytes << " bytes" << endl;
		//EV << NOW << "NRSchedulerGnbDl::rtxAcid Available: " << available << " bytes" << endl;

		unsigned int allocation = 0;
		if (available < bytes) {
			allocation = available;
			bytes -= available;
		} else {
			allocation = bytes;
			bytes = 0;
		}

		if ((allocatedCw == 0)) {
			unsigned int blocks = mac_->getAmc()->computeReqRbs(nodeId, b, remappedCw, allocation, direction_, allocator_->getBlocks(antenna, b, nodeId));
			//unsigned int blocks = mac_->getAmc()->computeReqRbs(nodeId, b, remappedCw, allocation, direction_,allocator_->availableBlocks(nodeId,antenna,b)) + 1;

			//EV << NOW << "NRSchedulerGnbDl::rtxAcid Assigned blocks: " << blocks << "  blocks" << endl;

			// assign only on the first codeword
			assignedBlocks.push_back(blocks);
			assignedBytes.push_back(allocation);
		}

		if (bytes == 0)
			break;
	}

	if (bytes > 0) {
		// process couldn't be served
		//EV << NOW << "NRSchedulerGnbDl::rtxAcid Cannot serve HARQ Process" << acid << endl;
		return 0;
	}

	// record the allocation if performed
	size = assignedBlocks.size();
	// For each LB with assigned blocks
	for (unsigned int i = 0; i < size; ++i) {
		if (allocatedCw == 0) {
			// allocate the blocks
			allocator_->addBlocks(antenna, bandLim->at(i).band_, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
		}
		// store the amount
		bandLim->at(i).limit_.at(remappedCw) = assignedBytes.at(i);
	}

	UnitList signal;
	signal.first = acid;
	signal.second.push_back(cw);

	//EV << NOW << " NRSchedulerGnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << " marking for retransmission " << endl;

	// if allocated codewords is not MAX_CODEWORDS, then there's another allocated codeword , update the codewords variable :

	if (allocatedCw != 0) {
		// TODO fixme this only works if MAX_CODEWORDS ==2
		--codewords;
		if (codewords <= 0)
			throw cRuntimeError("LteSchedulerEnbDl::rtxAcid(): erroneus codeword count %d", codewords);
	}

	// signal a retransmission
	currHarq->markSelected(signal, codewords);

	// mark codeword as used
	if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
		allocatedCws_.at(nodeId)++;

		}
	else {
		allocatedCws_[nodeId] = 1;

	}

	bytes = currHarq->pduLength(acid, cw);

	//EV << NOW << " NRSchedulerGnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << ", " << bytes << " bytes served!" << endl;

	//std::cout << "NRSchedulerGnbDl::schedulePerAcidRtx end at " << simTime().dbl() << std::endl;

	return bytes;
}

unsigned int NRSchedulerGnbDl::scheduleGrant(MacCid cid, unsigned int bytes, bool &terminate, bool &active, bool &eligible, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl) {
	//std::cout << "NRSchedulerGnbDl::scheduleGrant start at " << simTime().dbl() << std::endl;

	// Get the node ID and logical connection ID
	MacNodeId nodeId = MacCidToNodeId(cid);
	LogicalCid flowId = MacCidToLcid(cid);
	// Get user transmission parameters
	const UserTxParams &txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);
	//get the number of codewords
	unsigned int numCodewords = txParams.getLayers().size();

	std::string bands_msg = "BAND_LIMIT_SPECIFIED";

	std::vector<BandLimit> tempBandLim;

	if (bandLim == NULL) {
		bands_msg = "NO_BAND_SPECIFIED";
		// Create a vector of band limit using all bands
		bandLim = &tempBandLim;

		txParams.print("grant()");

		unsigned int numBands = mac_->getCellInfo()->getNumBands();
		// for each band of the band vector provided
		for (unsigned int i = 0; i < numBands; i++) {
			BandLimit elem;
			// copy the band
			elem.band_ = Band(i);
			//EV << "Putting band " << i << endl;
			// mark as unlimited
			for (unsigned int j = 0; j < numCodewords; j++) {
				//EV << "- Codeword " << j << endl;
				elem.limit_.push_back(-1);
			}
			bandLim->push_back(elem);
		}
	}
	//EV << "NRSchedulerGnbDl::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

	// Perform normal operation for grant

	// Get virtual buffer reference (it shouldn't be direction UL)
	vbuf_ = mac_->getMacBuffers();
	bsrbuf_ = mac_->getBsrVirtualBuffers();
	LteMacBuffer *conn = ((direction_ == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

	// get the buffer size
	unsigned int queueLength = conn->getQueueOccupancy(); // in bytes

	// get traffic descriptor
	FlowControlInfo connDesc;
	try {
		connDesc = mac_->getConnDesc().at(cid);
	} catch (...) {
		//std::cout << "NRSchedulerGnbDl::scheduleGrant end at " << simTime().dbl() << std::endl;
		terminate = true;
		return 0;
	}
	//

	//EV << "NRSchedulerGnbDl::grant --------------------::[ START GRANT ]::--------------------" << endl;
	//EV << "NRSchedulerGnbDl::grant Cell: " << mac_->getMacCellId() << endl;
	//EV << "NRSchedulerGnbDl::grant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

	//! Multiuser MIMO support
	if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER)) {
		// request AMC for MU_MIMO pairing
		MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, direction_);
		if (peer != nodeId) {
			// this user has a valid pairing
			//1) register pairing  - if pairing is already registered false is returned
			if (allocator_->configureMuMimoPeering(nodeId, peer)) {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
			} else {
				//EV << "NRSchedulerGnbDl::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
			}
		} else {
			//EV << "NRSchedulerGnbDl::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
		}
	}

	// registering DAS spaces to the allocator
	Plane plane = allocator_->getOFDMPlane(nodeId);
	allocator_->setRemoteAntenna(plane, antenna);

	unsigned int cwAlredyAllocated = 0;
	// search for already allocated codeword

	if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
		cwAlredyAllocated = allocatedCws_.at(nodeId);
		//allocatedCwsNodeCid_[cid] = cwAlredyAllocated;
	}
	// Check OFDM space
	// OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
	// if we are tryang to allocate a peer user in mu_mimo plane
	if (allocator_->computeTotalRbs() == 0
			&& (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING && txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlredyAllocated == 0)
					&& (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE))) {
		terminate = true; // ODFM space ended, issuing terminate flag
		//EV << "NRSchedulerGnbDl::grant Space ended, no schedulation." << endl;
		return 0;
	}

	// TODO This is just a BAD patch
	// check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
	// otherwise check why an UE is stopped being scheduled while its buffer is not empty
	if (cwAlredyAllocated > 0) {
		terminate = true;
		return 0;
	}

	// DEBUG OUTPUT
	if (limitBl) {
		//EV << "NRSchedulerGnbDl::grant blocks: " << bytes << endl;
	} else {
		//EV << "NRSchedulerGnbDl::grant Bytes: " << bytes << endl;
	}
	//EV << "NRSchedulerGnbDl::grant Bands: {";
	unsigned int size = (*bandLim).size();
	if (size > 0) {
		//EV << (*bandLim).at(0).band_;
		for (unsigned int i = 1; i < size; i++) {
			//EV << ", " << (*bandLim).at(i).band_;
		}
	}
	//EV << "}\n";

	//EV << "NRSchedulerGnbDl::grant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
	//EV << "NRSchedulerGnbDl::grant Available codewords: " << numCodewords << endl;

	bool stop = false;
	unsigned int totalAllocatedBytes = 0; // total allocated data (in bytes)
	unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)
	Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function

	// Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
	if (!checkEligibility(nodeId, cw) || cw >= numCodewords) {
		eligible = false;

		//EV << "NRSchedulerGnbDl::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
		//EV << "NRSchedulerGnbDl::grant Total allocation: " << totalAllocatedBytes << "bytes" << endl;
		//EV << "NRSchedulerGnbDl::grant NOT ELIGIBLE!!!" << endl;
		//EV << "NRSchedulerGnbDl::grant --------------------::[  END GRANT  ]::--------------------" << endl;
		return totalAllocatedBytes; // return the total number of served bytes
	}

	for (; cw < numCodewords; ++cw) {
		unsigned int cwAllocatedBytes = 0;
		// used by uplink only, for signaling cw blocks usage to schedule list
		unsigned int cwAllocatedBlocks = 0;
		// per codeword vqueue item counter (UL: BSRs DL: SDUs)
		unsigned int vQueueItemCounter = 0;

		unsigned int allocatedCws = 0;

		std::list<Request> bookedRequests;

		// band limit structure

		//EV << "NRSchedulerGnbDl::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

		unsigned int size = (*bandLim).size();

		unsigned int toBook;
		// Check whether the virtual buffer is empty
		if (queueLength == 0) {
			active = false; // all user data have been served
			//EV << "NRSchedulerGnbDl::grant scheduled connection is no more active . Exiting grant " << endl;
			stop = true;
			break;
		} else {
			// we need to consider also the size of RLC and MAC headers

			if (connDesc.getRlcType() == UM)
				queueLength += RLC_HEADER_UM;
			else if (connDesc.getRlcType() == AM)
				queueLength += RLC_HEADER_AM;
			queueLength += MAC_HEADER;

			toBook = queueLength;
		}

		//EV << "NRSchedulerGnbDl::grant bytes to be allocated: " << toBook << endl;

		// Book bands for this connection
		for (unsigned int i = 0; i < size; ++i) {
//            // for sulle bande
			unsigned int bandAllocatedBytes = 0;

			// save the band and the relative limit
			Band b = (*bandLim).at(i).band_;
			int limit = (*bandLim).at(i).limit_.at(cw);

			//EV << "NRSchedulerGnbDl::grant --- BAND " << b << " LIMIT " << limit << "---" << endl;

			// if the limit flag is set to skip, jump off
			if (limit == -2) {
				//EV << "NRSchedulerGnbDl::grant skipping logical band according to limit value" << endl;
				continue;
			}

			// search for already allocated codeword

			if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
				allocatedCws = allocatedCws_.at(nodeId);
				//allocatedCws = allocatedCwsNodeCid_.at(cid);
			}
			unsigned int bandAvailableBytes = 0;
			unsigned int bandAvailableBlocks = 0;
			// if there is a previous blocks allocation on the first codeword, blocks allocation is already available
			if (allocatedCws != 0) {
				// get band allocated blocks
				int b1 = allocator_->getBlocks(antenna, b, nodeId);
				// limit eventually allocated blocks on other codeword to limit for current cw
				bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);

				//    bandAvailableBlocks=b1;

				bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, direction_);
			}
			// if limit is expressed in blocks, limit value must be passed to availableBytes function
			else {            //TODO Check bandAvailableBytes
				bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_, (limitBl) ? limit : -1); // available space (in bytes)
				bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
			}

			// if no allocation can be performed, notify to skip
			// the band on next processing (if any)

			if (bandAvailableBytes == 0) {
				//EV << "NRSchedulerGnbDl::grant Band " << b << "will be skipped since it has no space left." << endl;
				(*bandLim).at(i).limit_.at(cw) = -2;
				continue;
			}
			//if bandLimit is expressed in bytes
			if (!limitBl) {
				// use the provided limit as cap for available bytes, if it is not set to unlimited
				if (limit >= 0 && limit < (int) bandAvailableBytes) {
					bandAvailableBytes = limit;
					//EV << "NRSchedulerGnbDl::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
				}
			} else {
				// if bandLimit is expressed in blocks
				if (limit >= 0 && limit < (int) bandAvailableBlocks) {
					bandAvailableBlocks = limit;
					//EV << "NRSchedulerGnbDl::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
				}
			}

			//EV << "NRSchedulerGnbDl::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

			// book resources on this band

			unsigned int blocksAdded = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bandAllocatedBytes, direction_, bandAvailableBlocks);
			//unsigned int blocksAdded = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bandAvailableBytes, direction_,bandAvailableBlocks);

			if (blocksAdded > bandAvailableBlocks)
				throw cRuntimeError("band %d GRANT allocation overflow : avail. blocks %d alloc. blocks %d", b, bandAvailableBlocks, blocksAdded);

			//EV << "NRSchedulerGnbDl::grant Booking band available blocks" << (bandAvailableBlocks-blocksAdded) << " [" << bandAvailableBytes << " bytes] for future use, going to next band" << endl;
			// enable booking  here
			bookedRequests.push_back(Request(b, bandAvailableBytes, bandAvailableBlocks - blocksAdded));

			// update the counter of bytes to be served
			toBook = (bandAvailableBytes > toBook) ? 0 : toBook - bandAvailableBytes;

			if (toBook == 0) {
				// all bytes booked, go to allocation
				stop = true;
				active = false;
				break;
			}
			// else continue booking (if there are available bands)
		}

		// allocation of the booked resources

		// compute here total booked requests
		unsigned int totalBooked = 0;
		unsigned int bookedUsed = 0;

		std::list<Request>::iterator li = bookedRequests.begin(), le = bookedRequests.end();
		for (; li != le; ++li) {
			totalBooked += li->bytes_;
			//EV << "NRSchedulerGnbDl::grant Band " << li->b_ << " can contribute with " << li->bytes_ << " of booked resources " << endl;
		}

		// get resources to allocate
		unsigned int toServe = queueLength - toBook;

		//EV << "NRSchedulerGnbDl::grant servicing " << toServe << " bytes with " << totalBooked << " booked bytes " << endl;

		// decrease booking value - if totalBooked is greater than 0, we used booked resources for scheduling the pdu
		if (totalBooked > 0) {
			// reset booked resources iterator.
			li = bookedRequests.begin();

			//EV << "NRSchedulerGnbDl::grant Making use of booked resources [" << totalBooked << "] for inter-band data allocation" << endl;
			// updating booked requests structure
			while ((li != le) && (bookedUsed <= toServe)) {
				Band u = li->b_;
				unsigned int uBytes = ((li->bytes_ > toServe) ? toServe : li->bytes_);

				//EV << "NRSchedulerGnbDl::grant allocating " << uBytes << " prev. booked bytes on band " << (unsigned short)u << endl;

				// mark here the usage of booked resources
				bookedUsed += uBytes;
				cwAllocatedBytes += uBytes;

				//changed, Thomas Deinlein
				unsigned int uBlocks = mac_->getAmc()->computeReqRbs(nodeId, u, cw, uBytes, direction_, li->blocks_);
				//

				if (uBlocks <= li->blocks_) {
					li->blocks_ -= uBlocks;
				} else {
					li->blocks_ = uBlocks = 0;
				}

				// add allocated blocks for this codeword
				cwAllocatedBlocks += uBlocks;

				// update limit
				if (uBlocks > 0) {
					unsigned int j = 0;
					for (; j < (*bandLim).size(); ++j)
						if ((*bandLim).at(j).band_ == u)
							break;

					if ((*bandLim).at(j).limit_.at(cw) > 0) {
						(*bandLim).at(j).limit_.at(cw) -= uBlocks;

						if ((*bandLim).at(j).limit_.at(cw) < 0)
							throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ", u, (*bandLim).at(j).limit_.at(cw), uBlocks);
					}
				}

				if (allocatedCws == 0) {
					// mark here allocation
					allocator_->addBlocks(antenna, u, nodeId, uBlocks, uBytes);
				}

				// update reserved status

				if (li->bytes_ > toServe) {
					li->bytes_ -= toServe;

					li++;
				} else {
					std::list<Request>::iterator erase = li;
					// increment pointer.
					li++;
					// erase element from list
					bookedRequests.erase(erase);

					//EV << "NRSchedulerGnbDl::grant band " << (unsigned short)u << " depleted all its booked resources " << endl;
				}
			}
			vQueueItemCounter++;
		}

		// update virtual buffer
		unsigned int alloc = toServe;
		alloc -= MAC_HEADER;
		if (connDesc.getRlcType() == UM)
			alloc -= RLC_HEADER_UM;
		else if (connDesc.getRlcType() == AM)
			alloc -= RLC_HEADER_AM;
		// alloc is the number of effective bytes allocated (without overhead)
		while (!conn->isEmpty() && alloc > 0) {
			unsigned int vPktSize = conn->front().first;
			if (vPktSize <= alloc) {
				// serve the entire vPkt
				conn->popFront();
				alloc -= vPktSize;
			} else {
				// serve partial vPkt

				// update pkt info
				PacketInfo newPktInfo = conn->popFront();
				newPktInfo.first = newPktInfo.first - alloc;
				conn->pushFront(newPktInfo);
				alloc = 0;
			}
		}

		//EV << "NRSchedulerGnbDl::grant Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;

		if (cwAllocatedBytes > 0) {
			// mark codeword as used
			if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
				allocatedCws_.at(nodeId)++;
				//allocatedCwsNodeCid_.at(cid) = allocatedCws_.at(nodeId);
				}
			else {
				allocatedCws_[nodeId] = 1;
				//allocatedCwsNodeCid_.at(cid) = 1;
			}

			totalAllocatedBytes += cwAllocatedBytes;

			std::pair<MacCid, Codeword> scListId;
			scListId.first = cid;
			scListId.second = cw;

			if (scheduleList_.find(scListId) == scheduleList_.end())
				scheduleList_[scListId].first = 0;

			// if direction is DL , then schedule list contains number of to-be-trasmitted SDUs ,
			// otherwise it contains number of granted blocks
			scheduleList_[scListId].first += ((direction_ == DL) ? vQueueItemCounter : cwAllocatedBlocks);
			scheduleList_[scListId].second = cwAllocatedBytes;

			//EV << "NRSchedulerGnbDl::grant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
			if (allocatedCws_.at(nodeId) == MAX_CODEWORDS) {
				eligible = false;
				stop = true;
			}
		} else {
			//EV << "NRSchedulerGnbDl::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
			eligible = false;
			stop = true;
		}
		if (stop)
			break;

	} // end for codeword

	//EV << "NRSchedulerGnbDl::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
	//EV << "NRSchedulerGnbDl::grant --------------------::[  END GRANT  ]::--------------------" << endl;

	//std::cout << "NRSchedulerGnbDl::scheduleGrant end at " << simTime().dbl() << std::endl;

	return totalAllocatedBytes;
}
