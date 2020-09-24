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

#include "corenetwork/binder/LteBinder.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <cctype>

#include "../lteCellInfo/LteCellInfo.h"
#include "corenetwork/nodes/InternetMux.h"

using namespace std;

Define_Module(LteBinder);

void LteBinder::unregisterNode(MacNodeId id)
{
    Enter_Method_Silent("unregisterNode");

    //std::cout << "LteBinder::unregisterNode start at " << simTime().dbl() << std::endl;

    //EV << NOW << " LteBinder::unregisterNode - unregistering node " << id << endl;

    if(nodeIds_.erase(id) != 1){
        EV_ERROR << "Cannot unregister node - node id \"" << id << "\" - not found";
    }
    std::map<IPv4Address, MacNodeId>::iterator it;
    for(it = macNodeIdToIPAddress_.begin(); it != macNodeIdToIPAddress_.end(); )
    {
        if(it->second == id)
        {
            macNodeIdToIPAddress_.erase(it++);
        }
        else
        {
            it++;
        }
    }

    //std::cout << "LteBinder::unregisterNode end at " << simTime().dbl() << std::endl;
}

MacNodeId LteBinder::registerNode(cModule *module, LteNodeType type,
    MacNodeId masterId)
{
    Enter_Method_Silent("registerNode");

    //std::cout << "LteBinder::registerNode start at " << simTime().dbl() << std::endl;

    MacNodeId macNodeId = -1;

    if (type == UE)
    {
        //
        for(auto & var : nodeIds_) {
            if(module->getId() == var.second)
                macNodeId = var.first;
        }
        //
        if(macNodeId = -1)
            macNodeId = macNodeIdCounter_[2]++;
    }
    else if (type == RELAY)
    {
        macNodeId = macNodeIdCounter_[1]++;
    }
    else if (type == ENODEB || type == GNODEB)
    {
        macNodeId = macNodeIdCounter_[0]++;
    }

    //std::cout << "LteBinder : Assigning to module " << module->getName() << " with OmnetId: " << module->getId() << " and MacNodeId " << macNodeId << "\n" << std::endl;

    // registering new node to LteBinder

    nodeIds_[macNodeId] = module->getId();

    module->par("macNodeId") = macNodeId;

    if (type == RELAY || type == UE)
    {
        registerNextHop(masterId, macNodeId);
    }
    else if (type == ENODEB || type == GNODEB)
    {
        module->par("macCellId") = macNodeId;
        registerNextHop(macNodeId, macNodeId);
    }

    //std::cout << "LteBinder::registerNode end at " << simTime().dbl() << std::endl;

    return macNodeId;
}

void LteBinder::registerNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("registerNextHop");

    //std::cout << "LteBinder::registerNextHop start at " << simTime().dbl() << std::endl;

    //EV << "LteBinder : Registering slave " << slaveId << " to master " << masterId << "\n";

    if (masterId != slaveId)
    {
        dMap_[masterId][slaveId] = true;
    }

    if (nextHop_.size() <= slaveId)
        nextHop_.resize(slaveId + 1);
    nextHop_[slaveId] = masterId;

    //std::cout << "LteBinder::registerNextHop end at " << simTime().dbl() << std::endl;
}

void LteBinder::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        numBands_ = par("numBands");
        numerology = par("numerology").intValue();
    }
}

void LteBinder::unregisterNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("unregisterNextHop");

    //std::cout << "LteBinder::unregisterNextHop start at " << simTime().dbl() << std::endl;

    //EV << "LteBinder : Unregistering slave " << slaveId << " from master " << masterId << "\n";
    dMap_[masterId][slaveId] = false;

    if (nextHop_.size() <= slaveId)
        return;
    nextHop_[slaveId] = 0;

    //std::cout << "LteBinder::unregisterNextHop end at " << simTime().dbl() << std::endl;
}

OmnetId LteBinder::getOmnetId(MacNodeId nodeId)
{
    Enter_Method_Silent("getOmnetId");

    //std::cout << "LteBinder::getOmnetId start at " << simTime().dbl() << std::endl;

    std::map<int, OmnetId>::iterator it = nodeIds_.find(nodeId);
    if(it != nodeIds_.end())
        return it->second;

    //std::cout << "LteBinder::getOmnetId end at " << simTime().dbl() << std::endl;

    return 0;
}

std::map<int, OmnetId>::const_iterator LteBinder::getNodeIdListBegin()
{
    return nodeIds_.begin();
}

std::map<int, OmnetId>::const_iterator LteBinder::getNodeIdListEnd()
{
    return nodeIds_.end();
}

MacNodeId LteBinder::getMacNodeIdFromOmnetId(OmnetId id){

    Enter_Method_Silent("getMacNodeIdFromOmnetId");

    //std::cout << "LteBinder::getMacNodeIdFromOmnetId start at " << simTime().dbl() << std::endl;

	std::map<int, OmnetId>::iterator it;
	for (it = nodeIds_.begin(); it != nodeIds_.end(); ++it )
	    if (it->second == id)
	        return it->first;

	//std::cout << "LteBinder::getMacNodeIdFromOmnetId end at " << simTime().dbl() << std::endl;

	return 0;
}

LteMacBase* LteBinder::getMacFromMacNodeId(MacNodeId id)
{

    Enter_Method_Silent("getMacFromMacNodeId");

    //std::cout << "LteBinder::getMacFromMacNodeId start at " << simTime().dbl() << std::endl;

    if (id == 0)
        return NULL;

    LteMacBase* mac;
    if (macNodeIdToModule_.find(id) == macNodeIdToModule_.end())
    {
        mac = check_and_cast<LteMacBase*>(getMacByMacNodeId(id));
        macNodeIdToModule_[id] = mac;
    }
    else
    {
        mac = macNodeIdToModule_[id];
    }

    //std::cout << "LteBinder::getMacFromMacNodeId end at " << simTime().dbl() << std::endl;

    return mac;
}

MacNodeId LteBinder::getNextHop(MacNodeId slaveId)
{
    Enter_Method_Silent("getNextHop");

    //std::cout << "LteBinder::getNextHop start at " << simTime().dbl() << std::endl;

    if (slaveId >= nextHop_.size())
        throw cRuntimeError("LteBinder::getNextHop(): bad slave id %d", slaveId);

    //std::cout << "LteBinder::getNextHop end at " << simTime().dbl() << std::endl;

    return nextHop_[slaveId];
}

void LteBinder::registerName(MacNodeId nodeId, const char* moduleName)
{
    Enter_Method_Silent("registerName");

    //std::cout << "LteBinder::registerName start at " << simTime().dbl() << std::endl;

    int len = strlen(moduleName);
    macNodeIdToModuleName_[nodeId] = new char[len+1];
    strcpy(macNodeIdToModuleName_[nodeId], moduleName);

    //std::cout << "LteBinder::registerName end at " << simTime().dbl() << std::endl;
}

const char* LteBinder::getModuleNameByMacNodeId(MacNodeId nodeId)
{
    Enter_Method_Silent("registerName");

    //std::cout << "LteBinder::getModuleNameByMacNodeId start at " << simTime().dbl() << std::endl;

    if (macNodeIdToModuleName_.find(nodeId) == macNodeIdToModuleName_.end())
        throw cRuntimeError("LteBinder::getModuleNameByMacNodeId - node ID not found");

    //std::cout << "LteBinder::getModuleNameByMacNodeId end at " << simTime().dbl() << std::endl;

    return macNodeIdToModuleName_[nodeId];
}

ConnectedUesMap LteBinder::getDeployedUes(MacNodeId localId)
{
    Enter_Method_Silent("getDeployedUes");

    //std::cout << "LteBinder::getDeployedUes  at " << simTime().dbl() << std::endl;

    return dMap_[localId];
}

simtime_t LteBinder::getLastUpdateUlTransmissionInfo()
{
    return lastUpdateUplinkTransmissionInfo_;
}

void LteBinder::initAndResetUlTransmissionInfo()
{
    ulTransmissionMap_[PREV_TTI] = ulTransmissionMap_[1];
    ulTransmissionMap_[CURR_TTI].clear();
    ulTransmissionMap_[CURR_TTI].resize(numBands_);

    lastUpdateUplinkTransmissionInfo_ = NOW;
}

void LteBinder::storeUlTransmissionMap(Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase* phy, Direction dir)
{
    UeAllocationInfo info;
    info.nodeId = nodeId;
    info.cellId = cellId;
    info.phy = phy;
    info.dir = dir;

    // for each allocated band, store the UE info
    std::map<Band, unsigned int>::iterator it = rbMap[antenna].begin(), et = rbMap[antenna].end();
    for ( ; it != et; ++it)
    {
        Band b = it->first;
        if (it->second > 0)
        		ulTransmissionMap_[CURR_TTI][b].push_back(info);
    }
}

const std::vector<UeAllocationInfo>* LteBinder::getUlTransmissionMap(UlTransmissionMapTTI t, Band b)
{
    return &(ulTransmissionMap_[t][b]);
}

void LteBinder::registerX2Port(X2NodeId nodeId, int port)
{

    Enter_Method_Silent("registerX2Port");

    //std::cout << "LteBinder::registerX2Port start at " << simTime().dbl() << std::endl;

    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
    {
        // no port has yet been registered
        std::list<int> ports;
        ports.push_back(port);
        x2ListeningPorts_[nodeId] = ports;
    }
    else
    {
        x2ListeningPorts_[nodeId].push_back(port);
    }

    //std::cout << "LteBinder::registerX2Port end at " << simTime().dbl() << std::endl;
}

int LteBinder::getX2Port(X2NodeId nodeId)
{
    Enter_Method_Silent("getX2Port");

    //std::cout << "LteBinder::getX2Port start at " << simTime().dbl() << std::endl;

    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
        throw cRuntimeError("LteBinder::getX2Port - No ports available on node %d", nodeId);

    int port = x2ListeningPorts_[nodeId].front();
    x2ListeningPorts_[nodeId].pop_front();

    //std::cout << "LteBinder::getX2Port end at " << simTime().dbl() << std::endl;

    return port;
}

Cqi LteBinder::meanCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir)
{
    Enter_Method_Silent("meanCqi");

    //std::cout << "LteBinder::meanCqi start at " << simTime().dbl() << std::endl;

    std::vector<Cqi>::iterator it;
    Cqi mean=0;
    for (it=bandCqi.begin();it!=bandCqi.end();++it)
    {
        mean+=*it;
    }
    mean/=bandCqi.size();

    if(mean==0)
        mean = 1;

    //std::cout << "LteBinder::meanCqi end at " << simTime().dbl() << std::endl;

    return mean;
}

bool LteBinder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    Enter_Method_Silent("addD2DCapability");

    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("LteBinder::checkD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    // if the entry is missing, check if the receiver is D2D capable and update the map
    if (d2dPeeringMap_.find(src) == d2dPeeringMap_.end() || d2dPeeringMap_[src].find(dst) == d2dPeeringMap_[src].end())
    {
        LteMacBase* dstMac = getMacFromMacNodeId(dst);
        if (dstMac->isD2DCapable())
        {
            // set the initial mode
            if (nextHop_[src] == nextHop_[dst])
            {
                // if served by the same cell, then the mode is selected according to the corresponding parameter
                LteMacBase* srcMac = getMacFromMacNodeId(src);
                bool d2dInitialMode = srcMac->getAncestorPar("d2dInitialMode").boolValue();
                d2dPeeringMap_[src][dst] = (d2dInitialMode) ? DM : IM;
            }
            else
            {
                // if served by different cells, then the mode can be IM only
                d2dPeeringMap_[src][dst] = IM;
            }

            //EV << "LteBinder::checkD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D (current mode " << ((d2dPeeringMap_[src][dst] == DM) ? "DM)" : "IM)") << endl;

            // this is a D2D-capable flow
            return true;
        }
        else
        {
            EV << "LteBinder::checkD2DCapability - UE " << src << " may not transmit to UE " << dst << " using D2D (UE " << dst << " is not D2D capable)" << endl;
            // this is not a D2D-capable flow
            return false;
        }

    }

    // an entry is present, hence this is a D2D-capable flow
    return true;
}

bool LteBinder::getD2DCapability(MacNodeId src, MacNodeId dst)
{
    Enter_Method_Silent("checkD2DCapability");

    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("LteBinder::addD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    // if the entry is missing, returns false
    if (d2dPeeringMap_.find(src) == d2dPeeringMap_.end() || d2dPeeringMap_[src].find(dst) == d2dPeeringMap_[src].end())
        return false;

    // the entry exists, no matter if it is DM or IM
    return true;
}

std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >* LteBinder::getD2DPeeringMap()
{
    return &d2dPeeringMap_;
}

LteD2DMode LteBinder::getD2DMode(MacNodeId src, MacNodeId dst)
{
    Enter_Method_Silent("getD2DMode");

    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("LteBinder::getD2DMode - Node Id not valid. Src %d Dst %d", src, dst);

    return d2dPeeringMap_[src][dst];
}

bool LteBinder::isFrequencyReuseEnabled(MacNodeId nodeId)
{
    Enter_Method_Silent("isFrequencyReuseEnabled");

    //std::cout << "LteBinder::isFrequencyReuseEnabled start at " << simTime().dbl() << std::endl;

    // a d2d-enabled UE can use frequency reuse if it can communicate using DM with all its peers
    // in fact, the scheduler does not know to which UE it will communicate when it grants some RBs
    if (d2dPeeringMap_.find(nodeId) == d2dPeeringMap_.end())
        return false;

    std::map<MacNodeId, LteD2DMode>::iterator it = d2dPeeringMap_[nodeId].begin();
    if (it == d2dPeeringMap_[nodeId].end())
        return false;

    for (; it != d2dPeeringMap_[nodeId].end(); ++it)
    {
        if (it->second == IM)
            return false;
    }

    //std::cout << "LteBinder::isFrequencyReuseEnabled end at " << simTime().dbl() << std::endl;

    return true;
}


void LteBinder::registerMulticastGroup(MacNodeId nodeId, int32 groupId)
{
    Enter_Method_Silent("registerMulticastGroup");

    //std::cout << "LteBinder::registerMulticastGroup start at " << simTime().dbl() << std::endl;

    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
    {
        MulticastGroupIdSet newSet;
        newSet.insert(groupId);
        multicastGroupMap_[nodeId] = newSet;
    }
    else
    {
        multicastGroupMap_[nodeId].insert(groupId);
    }

    //std::cout << "LteBinder::registerMulticastGroup start at " << simTime().dbl() << std::endl;
}

bool LteBinder::isInMulticastGroup(MacNodeId nodeId, int32 groupId)
{
    Enter_Method_Silent("isInMulticastGroup");

    //std::cout << "LteBinder::isInMulticastGroup start at " << simTime().dbl() << std::endl;

    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
        return false;   // the node is not enrolled in any group
    if (multicastGroupMap_[nodeId].find(groupId) == multicastGroupMap_[nodeId].end())
        return false;   // the node is not enrolled in the given group

    //std::cout << "LteBinder::isInMulticastGroup end at " << simTime().dbl() << std::endl;

    return true;
}

void LteBinder::addD2DMulticastTransmitter(MacNodeId nodeId)
{
    multicastTransmitterSet_.insert(nodeId);
}

std::set<MacNodeId>& LteBinder::getD2DMulticastTransmitters()
{
    return multicastTransmitterSet_;
}


void LteBinder::updateUeInfoCellId(MacNodeId id, MacCellId newCellId)
{

    Enter_Method_Silent("updateUeInfoCellId");

    //std::cout << "LteBinder::updateUeInfoCellId start at " << simTime().dbl() << std::endl;

    std::vector<UeInfo*>::iterator it = ueList_.begin();
    for (; it != ueList_.end(); ++it)
    {
        if ((*it)->id == id)
        {
            (*it)->cellId = newCellId;
            //std::cout << "LteBinder::updateUeInfoCellId end at " << simTime().dbl() << std::endl;
            return;
        }
    }
    //std::cout << "LteBinder::updateUeInfoCellId end at " << simTime().dbl() << std::endl;
}

void LteBinder::addUeHandoverTriggered(MacNodeId nodeId)
{
    Enter_Method_Silent("addUeHandoverTriggered");

    //std::cout << "LteBinder::addUeHandoverTriggered start at " << simTime().dbl() << std::endl;

    ueHandoverTriggered_.insert(nodeId);

    //std::cout << "LteBinder::addUeHandoverTriggered end at " << simTime().dbl() << std::endl;
}

bool LteBinder::hasUeHandoverTriggered(MacNodeId nodeId)
{
    Enter_Method_Silent("hasUeHandoverTriggered");

    //std::cout << "LteBinder::hasUeHandoverTriggered start at " << simTime().dbl() << std::endl;

    if (ueHandoverTriggered_.find(nodeId) == ueHandoverTriggered_.end())
        return false;

    //std::cout << "LteBinder::hasUeHandoverTriggered end at " << simTime().dbl() << std::endl;

    return true;
}

void LteBinder::removeUeHandoverTriggered(MacNodeId nodeId)
{
    Enter_Method_Silent("removeUeHandoverTriggered");

    //std::cout << "LteBinder::removeUeHandoverTriggered start at " << simTime().dbl() << std::endl;

    ueHandoverTriggered_.erase(nodeId);

    //std::cout << "LteBinder::removeUeHandoverTriggered end at " << simTime().dbl() << std::endl;
}
