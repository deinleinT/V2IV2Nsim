//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2020
// Author: Thomas Deinlein
//

#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduling_modules/LteDrr.h"
#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduling_modules/LtePf.h"
#include "stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "stack/mac/scheduling_modules/LteMaxCiOptMB.h"
#include "stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"

LteSchedulerEnb::LteSchedulerEnb()
{
    direction_ = DL;
    mac_ = 0;
    allocator_ = 0;
    scheduler_ = 0;
    vbuf_ = 0;
    harqTxBuffers_ = 0;
    harqRxBuffers_ = 0;
    resourceBlocks_ = 0;

    // ********************************
    //    sleepSize_ = 0;
    //    sleepShift_ = 0;
    //    zeroLevel_ = .0;
    //    idleLevel_ = .0;
    //    powerUnit_ = .0;
    //    maxPower_ = .0;
}

LteSchedulerEnb::~LteSchedulerEnb()
{
    delete allocator_;
    if(scheduler_)
        delete scheduler_;
}

void LteSchedulerEnb::initialize(Direction dir, LteMacEnb* mac)
{
    direction_ = dir;
    mac_ = mac;

    binder_ = getBinder();

    vbuf_ = mac_->getMacBuffers();
    bsrbuf_ = mac_->getBsrVirtualBuffers();

    harqTxBuffers_ = mac_->getHarqTxBuffers();
    harqRxBuffers_ = mac_->getHarqRxBuffers();

    // Create LteScheduler
    SchedDiscipline discipline = mac_->getSchedDiscipline(direction_);

    scheduler_ = getScheduler(discipline);
    scheduler_->setEnbScheduler(this);

    // Create Allocator
    if (discipline == ALLOCATOR_BESTFIT)   // NOTE: create this type of allocator for every scheduler using Frequency Reuse
        allocator_ = new LteAllocationModuleFrequencyReuse(mac_, direction_);
    else
        allocator_ = new LteAllocationModule(mac_, direction_);

    // Initialize statistics
    cellBlocksUtilizationDl_ = mac_->registerSignal("cellBlocksUtilizationDl");
    cellBlocksUtilizationUl_ = mac_->registerSignal("cellBlocksUtilizationUl");
    lteAvgServedBlocksDl_ = mac_->registerSignal("avgServedBlocksDl");
    lteAvgServedBlocksUl_ = mac_->registerSignal("avgServedBlocksUl");
}

LteMacScheduleListWithSizes* LteSchedulerEnb::schedule()
{
    //std::cout << "LteSchedulerEnb::schedule start at " << simTime().dbl() << std::endl;

    //EV << "LteSchedulerEnb::schedule performed by Node: " << mac_->getMacNodeId() << endl;

    // clearing structures for new scheduling
    scheduleList_.clear();
    allocatedCws_.clear();//allocatedCwsNodeCid_.clear();

    // clean the allocator
    initAndResetAllocator();
    //reset AMC structures
    mac_->getAmc()->cleanAmcStructures(direction_,scheduler_->readActiveSet());

    // scheduling of retransmission and transmission
    //EV << "___________________________start RTX __________________________________" << endl;
    if(!(scheduler_->scheduleRetransmissions()))
    {
        //EV << "____________________________ end RTX __________________________________" << endl;
        //EV << "___________________________start SCHED ________________________________" << endl;
        scheduler_->updateSchedulingInfo();
        scheduler_->schedule();

        //EV << "____________________________ end SCHED ________________________________" << endl;
    }

    // record assigned resource blocks statistics
    resourceBlockStatistics();

    //std::cout << "LteSchedulerEnb::schedule end at " << simTime().dbl() << std::endl;

    return &scheduleList_;
}

/*  COMPLETE:        scheduleGrant(cid,bytes,terminate,active,eligible,band_limit,antenna);
 *  ANTENNA UNAWARE: scheduleGrant(cid,bytes,terminate,active,eligible,band_limit);
 *  BAND UNAWARE:    scheduleGrant(cid,bytes,terminate,active,eligible);
 */
unsigned int LteSchedulerEnb::scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    // Get the node ID and logical connection ID
    MacNodeId nodeId = MacCidToNodeId(cid);
    LogicalCid flowId = MacCidToLcid(cid);

    Direction dir = direction_;
//    if (dir == UL)
//    {
//        // check if this connection is a D2D connection
//        if (flowId == D2D_SHORT_BSR)
//            dir = D2D;           // if yes, change direction
//        if (flowId == D2D_MULTI_SHORT_BSR)
//            dir = D2D_MULTI;     // if yes, change direction
//        // else dir == UL
//    }
    // else dir == DL

    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir);
    //get the number of codewords
    unsigned int numCodewords = txParams.getLayers().size();

    // TEST: check the number of codewords
    //numCodewords = 1;

    std::string bands_msg = "BAND_LIMIT_SPECIFIED";
    std::vector<BandLimit> tempBandLim;
    if (bandLim == NULL )
    {
        bands_msg = "NO_BAND_SPECIFIED";

        txParams.print("grant()");
        // Create a vector of band limit using all bands
        if( emptyBandLim_.empty() )
        {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                // mark as unlimited
                for (unsigned int j = 0; j < numCodewords; j++)
                {
                    EV << "- Codeword " << j << endl;
                    elem.limit_.push_back(-1);
                }
                emptyBandLim_.push_back(elem);
            }
        }
        tempBandLim = emptyBandLim_;
        bandLim = &tempBandLim;
    }
    //EV << "LteSchedulerEnb::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

    unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)

    // === Perform normal operation for grant === //

    EV << "LteSchedulerEnb::grant --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "LteSchedulerEnb::grant Cell: " << mac_->getMacCellId() << endl;
    EV << "LteSchedulerEnb::grant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

    //! Multiuser MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER))
    {
        // request AMC for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, dir);
        if (peer != nodeId)
        {
            // this user has a valid pairing
            //1) register pairing  - if pairing is already registered false is returned
            if (allocator_->configureMuMimoPeering(nodeId, peer))
                EV << "LteSchedulerEnb::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            else
                EV << "LteSchedulerEnb::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
        }
        else
        {
            EV << "LteSchedulerEnb::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }

    // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    // search for already allocated codeword
    unsigned int cwAlredyAllocated = 0;
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
        cwAlredyAllocated = allocatedCws_.at(nodeId);

    // Check OFDM space
    // OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
    // if we are tryang to allocate a peer user in mu_mimo plane
    if (allocator_->computeTotalRbs() == 0 && (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING &&
        txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlredyAllocated == 0) &&
        (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE)))
    {
        terminate = true; // ODFM space ended, issuing terminate flag
        EV << "LteSchedulerEnb::grant Space ended, no schedulation." << endl;
        return 0;
    }

    // TODO This is just a BAD patch
    // check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
    // otherwise check why an UE is stopped being scheduled while its buffer is not empty
    if (cwAlredyAllocated > 0)
    {
        terminate = true;
        return 0;
    }

    // ===== DEBUG OUTPUT ===== //
    bool debug = false; // TODO: make this configurable
    if (debug)
    {
        if (limitBl)
            EV << "LteSchedulerEnb::grant blocks: " << bytes << endl;
        else
            EV << "LteSchedulerEnb::grant Bytes: " << bytes << endl;
        EV << "LteSchedulerEnb::grant Bands: {";
        unsigned int size = (*bandLim).size();
        if (size > 0)
        {
            EV << (*bandLim).at(0).band_;
            for(unsigned int i = 1; i < size; i++)
                EV << ", " << (*bandLim).at(i).band_;
        }
        EV << "}\n";
    }
    // ===== END DEBUG OUTPUT ===== //

    EV << "LteSchedulerEnb::grant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
    EV << "LteSchedulerEnb::grant Available codewords: " << numCodewords << endl;

    // Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
    Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function
    if (!checkEligibility(nodeId, cw) || cw >= numCodewords)
    {
        eligible = false;

        EV << "LteSchedulerEnb::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
        EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << "bytes" << endl;
        EV << "LteSchedulerEnb::grant NOT ELIGIBLE!!!" << endl;
        EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes; // return the total number of served bytes
    }

    // Get virtual buffer reference
    LteMacBuffer* conn = ((dir == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

    // get the buffer size
    unsigned int queueLength = conn->getQueueOccupancy(); // in bytes

    if (queueLength == 0)
    {
        active = false;
        EV << "LteSchedulerEnb::scheduleGrant - scheduled connection is no more active . Exiting grant " << endl;
        EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes;
    }

    bool stop = false;
    unsigned int toServe = 0;
    for (; cw < numCodewords; ++cw)
    {
        EV << "LteSchedulerEnb::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
        toServe = queueLength;
        EV << "LteSchedulerEnb::scheduleGrant bytes to be allocated: " << toServe << endl;

        unsigned int cwAllocatedBytes = 0;  // per codeword allocated bytes
        unsigned int cwAllocatedBlocks = 0; // used by uplink only, for signaling cw blocks usage to schedule list
        unsigned int vQueueItemCounter = 0; // per codeword MAC SDUs counter

        unsigned int allocatedCws = 0;
        unsigned int size = (*bandLim).size();
        for (unsigned int i = 0; i < size; ++i) // for each band
        {
            // save the band and the relative limit
            Band b = (*bandLim).at(i).band_;
            int limit = (*bandLim).at(i).limit_.at(cw);
            EV << "LteSchedulerEnb::grant --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // if the limit flag is set to skip, jump off
            if (limit == -2)
            {
                EV << "LteSchedulerEnb::grant skipping logical band according to limit value" << endl;
                continue;
            }

            // search for already allocated codeword
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
                allocatedCws = allocatedCws_.at(nodeId);

            unsigned int bandAvailableBytes = 0;
            unsigned int bandAvailableBlocks = 0;
            // if there is a previous blocks allocation on the first codeword, blocks allocation is already available
            if (allocatedCws != 0)
            {
                // get band allocated blocks
                int b1 = allocator_->getBlocks(antenna, b, nodeId);
                // limit eventually allocated blocks on other codeword to limit for current cw
                bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);
                bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, dir);
            }
            else // if limit is expressed in blocks, limit value must be passed to availableBytes function
            {
                bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, dir, (limitBl) ? limit : -1); // available space (in bytes)
                bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
            }

            // if no allocation can be performed, notify to skip the band on next processing (if any)
            if (bandAvailableBytes == 0)
            {
                EV << "LteSchedulerEnb::grant Band " << b << "will be skipped since it has no space left." << endl;
                (*bandLim).at(i).limit_.at(cw) = -2;
                continue;
            }

            //if bandLimit is expressed in bytes
            if (!limitBl)
            {
                // use the provided limit as cap for available bytes, if it is not set to unlimited
                if (limit >= 0 && limit < (int) bandAvailableBytes)
                {
                    bandAvailableBytes = limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else
            {
                // if bandLimit is expressed in blocks
                if(limit >= 0 && limit < (int) bandAvailableBlocks)
                {
                    bandAvailableBlocks=limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "LteSchedulerEnb::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

            unsigned int uBytes = (bandAvailableBytes > queueLength) ? queueLength : bandAvailableBytes;
            unsigned int uBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, uBytes, dir,bandAvailableBlocks);

            // allocate resources on this band
            if(allocatedCws == 0)
            {
                // mark here allocation
                allocator_->addBlocks(antenna,b,nodeId,uBlocks,uBytes);
                // add allocated blocks for this codeword
                cwAllocatedBlocks += uBlocks;
                totalAllocatedBlocks += uBlocks;
                cwAllocatedBytes+=uBytes;
            }

            // update limit
            if(uBlocks>0 && (*bandLim).at(i).limit_.at(cw) > 0)
            {
                (*bandLim).at(i).limit_.at(cw) -= uBlocks;
                if ((*bandLim).at(i).limit_.at(cw) < 0)
                    throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ",
                        b,(*bandLim).at(i).limit_.at(cw),uBlocks);
            }

            // update the counter of bytes to be served
            toServe = (uBytes > toServe) ? 0 : toServe - uBytes;
            if (toServe == 0)
            {
                // all bytes booked, go to allocation
                stop = true;
                active = false;
                break;
            }
            // continue allocating (if there are available bands)
        }// Closes loop on bands

        if (cwAllocatedBytes > 0)
            vQueueItemCounter++;  // increase counter of served SDU

        // === update virtual buffer === //

        // number of bytes to be consumed from the virtual buffer
        unsigned int consumedBytes = cwAllocatedBytes - (MAC_HEADER + RLC_HEADER_UM);  // TODO RLC may be either UM or AM
        while (!conn->isEmpty() && consumedBytes > 0)
        {
            unsigned int vPktSize = conn->front().first;
            if (vPktSize <= consumedBytes)
            {
                // serve the entire vPkt, remove pkt info
                conn->popFront();
                consumedBytes -= vPktSize;
                EV << "LteSchedulerEnb::grant - the first SDU/BSR is served entirely, remove it from the virtual buffer, remaining bytes to serve[" << consumedBytes << "]" << endl;
            }
            else
            {
                // serve partial vPkt, update pkt info
                PacketInfo newPktInfo = conn->popFront();
                newPktInfo.first = newPktInfo.first - consumedBytes;
                conn->pushFront(newPktInfo);
                consumedBytes = 0;
                EV << "LteSchedulerEnb::grant - the first SDU/BSR is partially served, update its size [" << newPktInfo.first << "]" << endl;
            }
        }

        EV << "LteSchedulerEnb::grant Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;
        if (cwAllocatedBytes > 0)
        {
            // mark codeword as used
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
                allocatedCws_.at(nodeId)++;
            else
                allocatedCws_[nodeId] = 1;

            totalAllocatedBytes += cwAllocatedBytes;

            // create entry in the schedule list
            std::pair<unsigned int, Codeword> scListId(cid, cw);
            if (scheduleList_.find(scListId) == scheduleList_.end())
                scheduleList_[scListId].first = 0;

            // if direction is DL , then schedule list contains number of to-be-trasmitted SDUs ,
            // otherwise it contains number of granted blocks
            //scheduleList_[scListId] += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);
            scheduleList_[scListId].first += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);
            scheduleList_[scListId].second = totalAllocatedBytes;

            EV << "LteSchedulerEnb::grant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
            if (allocatedCws_.at(nodeId) == MAX_CODEWORDS)
            {
                eligible = false;
                stop = true;
            }
        }
        else
        {
            EV << "LteSchedulerEnb::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;
    } // Closes loop on Codewords

    EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}


void LteSchedulerEnb::update()
{
    //std::cout << "LteSchedulerEnb::update start at " << simTime().dbl() << std::endl;

    scheduler_->updateSchedulingInfo();
}

void LteSchedulerEnb::backlog(MacCid cid)
{
    //std::cout << "LteSchedulerEnb::backlog start at " << simTime().dbl() << std::endl;

    //EV << "LteSchedulerEnb::backlog - backlogged data for Logical Cid " << cid << endl;
    if(cid == 1)
        return;

    scheduler_->notifyActiveConnection(cid);

    //std::cout << "LteSchedulerEnb::backlog end at " << simTime().dbl() << std::endl;
}

unsigned int LteSchedulerEnb::readPerUeAllocatedBlocks(const MacNodeId nodeId,
    const Remote antenna, const Band b)
{
    //std::cout << "LteSchedulerEnb::readPerUeAllocatedBlocks start at " << simTime().dbl() << std::endl;

    return allocator_->getBlocks(antenna, b, nodeId);
}

unsigned int LteSchedulerEnb::readPerBandAllocatedBlocks(Plane plane, const Remote antenna, const Band band)
{
    //std::cout << "LteSchedulerEnb::readPerBandAllocatedBlocks start at " << simTime().dbl() << std::endl;

    return allocator_->getAllocatedBlocks(plane, antenna, band);
}

unsigned int LteSchedulerEnb::getInterferringBlocks(Plane plane, const Remote antenna, const Band band)
{
    //std::cout << "LteSchedulerEnb::getInterferringBlocks start at " << simTime().dbl() << std::endl;

    return allocator_->getInterferringBlocks(plane, antenna, band);
}

unsigned int LteSchedulerEnb::readAvailableRbs(const MacNodeId id,
    const Remote antenna, const Band b)
{
    //std::cout << "LteSchedulerEnb::readAvailableRbs start at " << simTime().dbl() << std::endl;

    return allocator_->availableBlocks(id, antenna, b);
}

unsigned int LteSchedulerEnb::readTotalAvailableRbs()
{
    //std::cout << "LteSchedulerEnb::readTotalAvailableRbs start at " << simTime().dbl() << std::endl;

    return allocator_->computeTotalRbs();
}

unsigned int LteSchedulerEnb::readRbOccupation(const MacNodeId id, RbMap& rbMap)
{
    //std::cout << "LteSchedulerEnb::readRbOccupation start at " << simTime().dbl() << std::endl;

    return allocator_->rbOccupation(id, rbMap);
}

/*
 * OFDMA frame management
 */

void LteSchedulerEnb::initAndResetAllocator()
{
    //std::cout << "LteSchedulerEnb::initAndResetAllocator start at " << simTime().dbl() << std::endl;

    // initialize and reset the allocator
    allocator_->initAndReset(resourceBlocks_,
        mac_->getCellInfo()->getNumBands());
}

unsigned int LteSchedulerEnb::availableBytes(const MacNodeId id,
    Remote antenna, Band b, Codeword cw, Direction dir, int limit)
{
    //std::cout << "LteSchedulerEnb::availableBytes start at " << simTime().dbl() << std::endl;

    //EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << " cw " << cw << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id,antenna,b);
    //Consistency Check
    if (limit>blocks && limit!=-1)
    	throw cRuntimeError("LteSchedulerEnb::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d",b,limit,blocks);

    if (limit!=-1)
    	blocks=(blocks>limit)?limit:blocks;
    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(id, b, cw, blocks, dir);
    //EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    //std::cout << "LteSchedulerEnb::availableBytes end at " << simTime().dbl() << std::endl;

    return bytes;
}

std::set<Band> LteSchedulerEnb::getOccupiedBands()
{

    //std::cout << "LteSchedulerEnb::getOccupiedBands start at " << simTime().dbl() << std::endl;

   return allocator_->getAllocatorOccupiedBands();
}

void LteSchedulerEnb::storeAllocationEnb( std::vector<std::vector<AllocatedRbsPerBandMapA> > allocatedRbsPerBand,std::set<Band>* untouchableBands)
{
    //std::cout << "LteSchedulerEnb::storeAllocationEnb start at " << simTime().dbl() << std::endl;

    allocator_->storeAllocation(allocatedRbsPerBand, untouchableBands);
}

void LteSchedulerEnb::storeScListId(std::pair<unsigned int, Codeword> scList,unsigned int num_blocks)
{

    //std::cout << "LteSchedulerEnb::storeScListId start at " << simTime().dbl() << std::endl;

    scheduleList_[scList].first = num_blocks;
}

    /*****************
     * UTILITIES
     *****************/

LteScheduler* LteSchedulerEnb::getScheduler(SchedDiscipline discipline)
{
    //std::cout << "LteSchedulerEnb::getScheduler start at " << simTime().dbl() << std::endl;

    //EV << "Creating LteScheduler " << schedDisciplineToA(discipline) << endl;

    switch (discipline) {
    case DRR:
        return new LteDrr();
    case PF:
        return new LtePf(mac_->par("pfAlpha").doubleValue());
    case MAXCI:
        return new LteMaxCi();
    case MAXCI_MB:
        return new LteMaxCiMultiband();
    case MAXCI_OPT_MB:
        return new LteMaxCiOptMB();
    case MAXCI_COMP:
        return new LteMaxCiComp();
    case ALLOCATOR_BESTFIT:
        return new LteAllocatorBestFit();

    default:
        throw cRuntimeError("LteScheduler not recognized");
        return NULL;
    }
}

void LteSchedulerEnb::resourceBlockStatistics(bool sleep)
{
    //std::cout << "LteSchedulerEnb::resourceBlockStatistics start at " << simTime().dbl() << std::endl;

    if (sleep)
    {
        if (direction_ == DL)
        {
            mac_->emit(cellBlocksUtilizationDl_, 0.0);
            mac_->emit(lteAvgServedBlocksDl_, (long)0);
        }
        return;
    }
    // Get a reference to the begin and the end of the map which stores the blocks allocated
    // by each UE in each Band. In this case, is requested the pair of iterators which refers
    // to the per-Band (first key) per-Ue (second-key) map
    std::vector<std::vector<unsigned int> >::const_iterator planeIt =
        allocator_->getAllocatedBlocksBegin();
    std::vector<std::vector<unsigned int> >::const_iterator planeItEnd =
        allocator_->getAllocatedBlocksEnd();

    double utilization = 0.0;
    double allocatedBlocks = 0;
    unsigned int plane = 0;
    unsigned int antenna = 0;


    std::vector<unsigned int>::const_iterator antennaIt = planeIt->begin();
    std::vector<unsigned int>::const_iterator antennaItEnd = planeIt->end();

    // For each antenna (MACRO/RUs)
    for (; antennaIt != antennaItEnd; ++antennaIt)
    {
        if (plane == MAIN_PLANE && antenna == MACRO)
            if (direction_ == DL)
                mac_->allocatedRB(*antennaIt);
        // collect the antenna utilization for current Layer
        utilization += (double) (*antennaIt);

        allocatedBlocks += (double) (*antennaIt);

        antenna++;
    }
    plane++;
    //    }
    // antenna here is the number of antennas used; the same applies for plane;
    // Compute average OFDMA utilization between layers and antennas
    utilization /= (((double) (antenna)) * ((double) resourceBlocks_));
    if (direction_ == DL)
    {
        mac_->emit(cellBlocksUtilizationDl_, utilization);
        mac_->emit(lteAvgServedBlocksDl_, allocatedBlocks);
    }
    else if (direction_ == UL)
    {
        mac_->emit(cellBlocksUtilizationUl_, utilization);
        mac_->emit(lteAvgServedBlocksUl_, allocatedBlocks);
    }
    else
    {
        throw cRuntimeError("LteSchedulerEnb::resourceBlockStatistics(): Unrecognized direction %d", direction_);
    }

    //std::cout << "LteSchedulerEnb::resourceBlockStatistics end at " << simTime().dbl() << std::endl;
}
ActiveSet LteSchedulerEnb::readActiveConnections()
{
    //std::cout << "LteSchedulerEnb::readActiveConnections start at " << simTime().dbl() << std::endl;

    ActiveSet active = scheduler_->readActiveSet();
    ActiveSet::iterator it = active.begin();
    ActiveSet::iterator et = active.end();
    MacCid cid;
    for (; it != et; ++it)
    {
        cid = *it;
    }


    //std::cout << "LteSchedulerEnb::readActiveConnections end at " << simTime().dbl() << std::endl;
    return scheduler_->readActiveSet();
}

void LteSchedulerEnb::removeActiveConnections(MacNodeId nodeId)
{
    //std::cout << "LteSchedulerEnb::removeActiveConnections start at " << simTime().dbl() << std::endl;

    ActiveSet active = scheduler_->readActiveSet();
    ActiveSet::iterator it = active.begin();
    ActiveSet::iterator et = active.end();
    MacCid cid;
    for (; it != et; ++it)
    {
        cid = *it;
        if (MacCidToNodeId(cid) == nodeId)
        {
            scheduler_->removeActiveConnection(cid);
        }
    }

    //std::cout << "LteSchedulerEnb::removeActiveConnections end at " << simTime().dbl() << std::endl;
}


