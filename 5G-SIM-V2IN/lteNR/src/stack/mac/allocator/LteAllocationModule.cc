//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/layer/LteMacEnb.h"

using namespace omnetpp;
using namespace inet;

LteAllocationModule::LteAllocationModule(LteMacEnb *mac, Direction direction)
{
    mac_ = mac;
    dir_ = direction;
    bands_ = 0;
    usedInLastSlot_ = false;
}

void LteAllocationModule::init(const unsigned int resourceBlocks, const unsigned int bands)
{
    // clear the OFDMA available blocks and set available planes to 1 (just the main OFDMA space)
    totalRbsMatrix_.clear();
    totalRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    totalRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1);
    // initialize main OFDMA space
    totalRbsMatrix_[MAIN_PLANE][MACRO] = resourceBlocks;

    // initialize number of bands
    bands_ = bands;

    // clear the OFDMA allocated blocks and set available planes to 1 (just the main OFDMA space)
    allocatedRbsMatrix_.clear();
    allocatedRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    allocatedRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1, 0);

    // clean and copy stored block-allocation info
    prevAllocatedRbsPerBand_.clear();
    prevAllocatedRbsPerBand_ = allocatedRbsPerBand_;

    // clear and reinitialize the allocatedBlocks structures and set available planes to 1 (just the main OFDMA space)
    allocatedRbsPerBand_.clear();
    allocatedRbsPerBand_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    allocatedRbsPerBand_.at(MAIN_PLANE).resize(MACRO + 1);

    // clear and reinitialize the freeResourceBlocks vector and set available planes to 1 (just the main OFDMA space)
    freeRbsMatrix_.clear();
    freeRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    freeRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1);
    // initializes all free blocks for each band in the main plane, MACRO antenna.
    freeRbsMatrix_.at(MAIN_PLANE).at(MACRO).resize(bands_, 0);

    // clear UE,LB Map
    allocatedRbsUe_.clear();
}

void LteAllocationModule::reset(const unsigned int resourceBlocks, const unsigned int bands)
{
    // clean and copy stored block-allocation info
    prevAllocatedRbsPerBand_ = allocatedRbsPerBand_;

    // reset structures only if they were used in the previous time slot
    if (usedInLastSlot_)
    {
        std::vector<std::vector<unsigned int> >::iterator plane_it;
        std::vector<unsigned int>::iterator antenna_it;

        // initialize main OFDMA space
        for (plane_it = totalRbsMatrix_.begin(); plane_it != totalRbsMatrix_.end(); ++plane_it)
        {
            for (antenna_it = plane_it->begin(); antenna_it != plane_it->end(); ++antenna_it)
            {
                (*antenna_it) = resourceBlocks;
            }
        }

        // set the available antennas of MAIN plane to 1 (just MACRO antenna)
        for (plane_it = allocatedRbsMatrix_.begin(); plane_it != allocatedRbsMatrix_.end(); ++plane_it)
        {
            for (antenna_it = plane_it->begin(); antenna_it != plane_it->end(); ++antenna_it)
            {
                (*antenna_it) = 0;
            }
        }



        // clear and reinitialize the allocatedBlocks structures and set available planes to 1 (just the main OFDMA space)
        std::vector<std::vector<AllocatedRbsPerBandMap> >::iterator plane_jt;
        std::vector<AllocatedRbsPerBandMap>::iterator antenna_jt;
        for (plane_jt = allocatedRbsPerBand_.begin(); plane_jt != allocatedRbsPerBand_.end(); ++plane_jt)
        {
            for (antenna_jt = plane_jt->begin(); antenna_jt != plane_jt->end(); ++antenna_jt)
            {
                antenna_jt->clear();
            }
        }

        // clear and reinitialize the freeResourceBlocks vector and set available planes to 1 (just the main OFDMA space)
        std::vector<std::vector<std::vector<unsigned int> > >::iterator plane_zt;
        std::vector<std::vector<unsigned int> >::iterator antenna_zt;
        std::vector<unsigned int>::iterator band_zt;
        for (plane_zt = freeRbsMatrix_.begin(); plane_zt != freeRbsMatrix_.end(); ++plane_zt)
        {
            for (antenna_zt = plane_zt->begin(); antenna_zt != plane_zt->end(); ++antenna_zt)
            {
                for (band_zt = antenna_zt->begin(); band_zt != antenna_zt->end(); ++band_zt)
                {
                    (*band_zt) = 0;
                }
            }
        }

        // clear UE,LB Map
        allocatedRbsUe_.clear();
    }

    usedInLastSlot_ = false;
}

void LteAllocationModule::configureOFDMplane(const Plane plane)
{
    // check if an OFDMA space exists with given plane ID
    if (totalRbsMatrix_.size() < (unsigned int) (plane + 1))
    {
        // here we have to add missing planes to the OFDMA space and create the MACRO antenna entry for this plane
        totalRbsMatrix_.resize(plane + 1);
        totalRbsMatrix_.at(plane).resize(MACRO + 1);

        allocatedRbsMatrix_.resize(plane + 1);
        allocatedRbsMatrix_.at(plane).resize(MACRO + 1);

        allocatedRbsPerBand_.resize(plane + 1);
        allocatedRbsPerBand_.at(plane).resize(MACRO + 1);

        freeRbsMatrix_.resize(plane + 1);
        freeRbsMatrix_.at(plane).resize(MACRO + 1);
        freeRbsMatrix_.at(plane).at(MACRO).resize(bands_, 0);

        // we set newly created OFDMA space equal to its peer space
        totalRbsMatrix_[plane][MACRO] = totalRbsMatrix_[MAIN_PLANE][MACRO];

        usedInLastSlot_ = true;
    }
}

void LteAllocationModule::setRemoteAntenna(const Plane plane, const Remote antenna)
{
    /**
     * Check if antenna already exists in given OFDMA space,
     * otherwise creates all antennas between last one and given one.
     */
    for (int i = totalRbsMatrix_.at(plane).size(); i < antenna + 1; ++i)
    {
        // here we have to add missing antennas to the given plane and to set the number of RB for each antenna in this plane
        totalRbsMatrix_.at(plane).resize(i + 1);
        allocatedRbsMatrix_.at(plane).resize(i + 1);
        allocatedRbsPerBand_.at(plane).resize(i + 1);
        freeRbsMatrix_.at(plane).resize(i + 1);
        freeRbsMatrix_.at(plane).at(i).resize(bands_, 0);
        // initialize new antenna space with macro space
        totalRbsMatrix_[plane][i] = totalRbsMatrix_[plane][MACRO];

        usedInLastSlot_ = true;
    }
}

bool LteAllocationModule::configureMuMimoPeering(const MacNodeId nodeId, const MacNodeId peer)
{
    //---------- Peering availability Check ----------
    // peer user already set for the specified nodeId
    if (allocatedRbsUe_[nodeId].muMimoEnabled_ == true)
        return false;
    // peer user already set for the specified peer
    if (allocatedRbsUe_[peer].muMimoEnabled_ == true)
        return false;

    //---- If we reach this point, we can use MuMimo peering by setting the allocator properly ----
    // set direct peering
    allocatedRbsUe_[nodeId].muMimoEnabled_ = true;
    allocatedRbsUe_[peer].muMimoEnabled_ = true;

    // set peers for each side
    allocatedRbsUe_[nodeId].peerId_ = peer;
    allocatedRbsUe_[peer].peerId_ = nodeId;

    allocatedRbsUe_[nodeId].secondaryUser_ = false; // primary MU-MIMO user
    allocatedRbsUe_[peer].secondaryUser_ = true;   // secondary MU-MIMO user

    // set the peer's antennas  to the main user's one.
    RemoteSet peerAntennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;
    allocatedRbsUe_[peer].availableAntennaSet_ = peerAntennas;

    // check if the mirror MIMO plane has to be created.
    configureOFDMplane(MU_MIMO_PLANE);

    // for each antenna of main user, create a mirror MU-MIMO antenna space for peer user
    RemoteSet::iterator it = peerAntennas.begin(), et = peerAntennas.end();
    for (; it != et; ++it)
    {
        setRemoteAntenna(MU_MIMO_PLANE, *it);
    }

    usedInLastSlot_ = true;

    // peering configured successfully
    return true;
}

Plane LteAllocationModule::getOFDMPlane(const MacNodeId nodeId)
{
    return (allocatedRbsUe_[nodeId].secondaryUser_) ? MU_MIMO_PLANE : MAIN_PLANE;
}

MacNodeId LteAllocationModule::getMuMimoPeer(const MacNodeId nodeId) const
    {
    if (allocatedRbsUe_.find(nodeId) != allocatedRbsUe_.end())
    {
        return (allocatedRbsUe_.at(nodeId).muMimoEnabled_) ? allocatedRbsUe_.at(nodeId).peerId_ : nodeId;
    }
    return nodeId;
}

unsigned int LteAllocationModule::computeTotalRbs()
{
    // The amount of available blocks in all the OFDM spaces is obtained from the overall
    // amount of RBs in every plane of every antenna, decreased by the number of the allocated blocks

    int resourceBlocks = 0;
    int allocatedBlocks = 0;

    // for each plane
    for (unsigned int i = 0; i < totalRbsMatrix_.size(); ++i)
    {
        // for each antenna
        for (unsigned int j = 0; j < totalRbsMatrix_.at(i).size(); ++j)
        {
            resourceBlocks += totalRbsMatrix_.at(i).at(j);
            allocatedBlocks += allocatedRbsMatrix_.at(i).at(j);
        }
    }

    int availableBlocks = resourceBlocks - allocatedBlocks;

    // available blocks MUST be a non negative amount
    if (availableBlocks < 0)
        throw cRuntimeError("LteAllocator::checkOFDMspace(): Negative OFDM space");

    // DEBUG
    EV << NOW << " LteAllocationModule::computeTotalRbs " << dirToA(dir_) << " - total " << resourceBlocks <<
    ", allocated " << allocatedBlocks << ", " << availableBlocks << " blocks available" << endl;

    return ((unsigned int) availableBlocks);
}

unsigned int LteAllocationModule::availableBlocks(const MacNodeId nodeId, const Remote antenna, const Band band)
{
    Plane plane = getOFDMPlane(nodeId);

    unsigned int blocksPerBand = (totalRbsMatrix_[plane][antenna]) / bands_;
    // blocks allocated in the current band
    unsigned int allocatedBlocks = allocatedRbsPerBand_[plane][antenna][band].allocated_;

    if (blocksPerBand >= allocatedBlocks)
    {
        // DEBUG
        EV << NOW << " LteAllocator::availableBlocks " << dirToA(dir_) << " - Band " << band <<
        " has " << blocksPerBand - allocatedBlocks <<
        " blocks available [total " << blocksPerBand << ", allocated " << allocatedBlocks << "]" << endl;

        return (blocksPerBand - allocatedBlocks);
    }
    else
    {
        // no space available on current antenna.
        return 0;
    }
}

unsigned int LteAllocationModule::getAllocatedBlocks(Plane plane, const Remote antenna, const Band band)
{
    return allocatedRbsPerBand_[plane][antenna][band].allocated_;
}

unsigned int LteAllocationModule::getInterferringBlocks(Plane plane, const Remote antenna, const Band band)
{
    if (!prevAllocatedRbsPerBand_.empty())
        return prevAllocatedRbsPerBand_[plane][antenna][band].allocated_;
    else
        return 1000;
}

unsigned int LteAllocationModule::availableBlocks(const MacNodeId nodeId, const Plane plane, const Band band)
{
    // compute available blocks on all antennas for given user and plane.
    RemoteSet antennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;
    RemoteSet::iterator it = antennas.begin(), et = antennas.end();

    unsigned int available = 0;

    for (; it != et; ++it)
    {
        available += availableBlocks(nodeId, *it, band);
    }
    // returning available blocks
    return available;
}

bool LteAllocationModule::addBlocks(const Band band, const MacNodeId nodeId, const unsigned int blocks,
    const unsigned int bytes)
{
    //  all antennas for given user and plane.
    RemoteSet antennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;
    RemoteSet::iterator it = antennas.begin(), et = antennas.end();
    bool ret = false;
    for (; it != et; ++it)
    {
        ret = addBlocks(*it, band, nodeId, blocks, bytes);
        if (ret)
            break;
    }
    // returning allocation result
    return ret;
}

bool LteAllocationModule::addBlocks(const Remote antenna, const Band band, const MacNodeId nodeId,
    const unsigned int blocks, const unsigned int bytes)
{
    // Check if the band exists
    if (band >= bands_)
        throw cRuntimeError("LteAllocator::addBlocks(): Invalid band %d", (int) band);

    // Check if there's enough OFDM space
    // retrieving user's plane
    Plane plane = getOFDMPlane(nodeId);

    // Obtain the available blocks on the given band
    int availableBlocksOnBand = availableBlocks(nodeId, antenna, band);

    // Check if the band can satisfy the request
    if ((availableBlocksOnBand - (int) blocks) < 0)
    {
        EV << NOW << " LteAllocator::addBlocks " << dirToA(dir_) << " - Node " << nodeId <<
        ", not enough space on band " << band << ": requested " << blocks <<
        " available " << availableBlocksOnBand << " " << endl;
        return false;
    }
        // check if UE is out of range. (CQI=0 => bytes=0)
    if (bytes == 0)
    {
        EV << NOW << " LteAllocator::addBlocks " << dirToA(dir_) << " - Node " << nodeId << " - 0 bytes available with " << blocks << " blocks" << endl;
        return false;
    }

        // Note the request on the allocator structures
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId] += blocks;
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedBytesMap_[nodeId] += bytes;
    allocatedRbsPerBand_[plane][antenna][band].allocated_ += blocks;

    allocatedRbsUe_[nodeId].ueAllocatedRbsMap_[antenna][band] += blocks;
    allocatedRbsUe_[nodeId].allocatedBlocks_ += blocks;
    allocatedRbsUe_[nodeId].allocatedBytes_ += bytes;
    allocatedRbsUe_[nodeId].antennaAllocatedRbs_[antenna] += blocks;

    // Store the request in the allocationList
    AllocationElem elem;
    elem.resourceBlocks_ = blocks;
    elem.bytes_ = bytes;
    allocatedRbsUe_[nodeId].allocationMap_[antenna][band].push_back(elem);

    // update the allocatedBlocks counter
    allocatedRbsMatrix_[plane][antenna] += blocks;

    usedInLastSlot_ = true;

    EV << NOW << " LteAllocator::addBlocks " << dirToA(dir_) << " - Node " << nodeId << ", the request of " << blocks << " blocks on band " << band << " satisfied" << endl;

    return true;
}

unsigned int LteAllocationModule::removeBlocks(const Remote antenna, const Band band, const MacNodeId nodeId)
{
    // Check if the band exists
    if (band >= bands_)
    {
        EV << NOW << " LteAllocator::removeBlocks " << dirToA(dir_) << " - Node " << nodeId << ", invalid band " << band << endl;
        return 0;
    }

    // retrieving user's plane
    Plane plane = getOFDMPlane(nodeId);

    unsigned int toDrain = allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId];

    // If the number of blocks allocated by the nodeId in the band is zero, do nothing!
    if(toDrain == 0)
    return toDrain;

    // Note the removal on the allocator structures
    allocatedRbsPerBand_[plane][antenna][band].allocated_ -= toDrain;
    allocatedRbsUe_[nodeId].allocatedBlocks_-= toDrain;

    allocatedRbsUe_[nodeId].ueAllocatedRbsMap_[antenna][band] = 0;
    allocatedRbsUe_[nodeId].allocatedBytes_=0;
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId] = 0;

    // drop the allocation list
    allocatedRbsUe_[nodeId].allocationMap_[antenna][band].clear();

    // update the allocatedBlocks counter
    allocatedRbsMatrix_[plane][antenna] -= toDrain;

    usedInLastSlot_ = true;

    // DEBUG
    EV << NOW << " LteAllocator::removeBlocks " << dirToA(dir_) << " - Node " << nodeId << ", " << toDrain << " blocks drained from band " << band << endl;

    return toDrain;
}

unsigned int
LteAllocationModule::rbOccupation(const MacNodeId nodeId, RbMap& rbMap)
{
    // compute allocated blocks on all antennas for given user and logical band.
    RemoteSet antennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;
    RemoteSet::iterator it = antennas.begin(), et = antennas.end();

    unsigned int blocks = 0;

    for (; it != et; ++it)
    {
        for (Band b = 0; b < bands_; ++b)
        {
            blocks += (rbMap[*it][b] = getBlocks(*it, b, nodeId));
        }
    }
    return blocks;
}
