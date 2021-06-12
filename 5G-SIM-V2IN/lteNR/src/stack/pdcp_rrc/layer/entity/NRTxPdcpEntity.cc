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

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2021
// Author: Thomas Deinlein
//

#include "stack/pdcp_rrc/layer/entity/NRTxPdcpEntity.h"

Define_Module(NRTxPdcpEntity);

NRTxPdcpEntity::NRTxPdcpEntity()
{
}

void NRTxPdcpEntity::initialize()
{
    LteTxPdcpEntity::initialize();
}

void NRTxPdcpEntity::deliverPdcpPdu(Packet* pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (getNodeTypeById(pdcp_->nodeId_) == UE)
    {
        EV << NOW << " NRTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] - send packet to lower layer" << endl;
        LteTxPdcpEntity::deliverPdcpPdu(pkt);
    }
    else   // ENODEB
    {
        if (!pdcp_->isDualConnectivityEnabled())
        {
            MacNodeId destId = lteInfo->getDestId();
            if (getNodeTypeById(destId) != UE)
                throw cRuntimeError("NRTxPdcpEntity::deliverPdcpPdu - the destination is not a UE but Dual Connectivity is not enabled.");

            EV << NOW << " NRTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] - the destination is a UE. Send packet to lower layer" << endl;
            LteTxPdcpEntity::deliverPdcpPdu(pkt);
        }
        else
        {
            MacNodeId destId = lteInfo->getDestId();
            bool useNR = lteInfo->getUseNR();
            if (!useNR)
            {
                if(getNodeTypeById(destId) != UE)
                    throw cRuntimeError("NRTxPdcpEntity::deliverPdcpPdu - the destination is a UE under the control of a secondary node, but the packet has not been marked as NR packet.");;

                EV << NOW << " NRTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] useNR[" << useNR << "] - the destination is a UE. Send packet to lower layer." << endl;
                LteTxPdcpEntity::deliverPdcpPdu(pkt);
            }
            else  // useNR
            {
                if(getNodeTypeById(destId) == UE)
                    throw cRuntimeError("NRTxPdcpEntity::deliverPdcpPdu - the packet has been marked as NR packet, but destination is not the secondary node");

                EV << NOW << " NRTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] - the destination is under the control of a secondary node" << endl;
                pdcp_->forwardDataToTargetNode(pkt, destId);
            }
        }
    }
}

void NRTxPdcpEntity::setIds(FlowControlInfo* lteInfo)
{
    //changed
    if((lteInfo->getDestId() >= ENB_MIN_ID && lteInfo->getDestId() <= UE_MAX_ID) && (lteInfo->getSourceId() >= ENB_MIN_ID && lteInfo->getSourceId() <= UE_MAX_ID)){
        return;
    }
    //

    if (lteInfo->getUseNR() && getNodeTypeById(pdcp_->getNodeId()) != ENODEB && getNodeTypeById(pdcp_->getNodeId()) != GNODEB)
        lteInfo->setSourceId(pdcp_->getNrNodeId());
    else
        lteInfo->setSourceId(pdcp_->getNodeId());

    if (lteInfo->getMulticastGroupId() > 0)   // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
        lteInfo->setDestId(pdcp_->getNodeId());
    else
        lteInfo->setDestId(pdcp_->getDestId(lteInfo));

}

NRTxPdcpEntity::~NRTxPdcpEntity()
{
}
