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


#include "stack/mac/layer/LteMacBase.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "common/LteControlInfo.h"
#include "corenetwork/binder/LteBinder.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "assert.h"

#include "../../../corenetwork/lteCellInfo/LteCellInfo.h"

LteMacBase::LteMacBase()
{
    mbuf_.clear();
    macBuffers_.clear();
}

LteMacBase::~LteMacBase()
{
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); mit++)
        delete mit->second;
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); vit++)
        delete vit->second;
    mbuf_.clear();
    macBuffers_.clear();

    HarqTxBuffers::iterator htit;
    HarqRxBuffers::iterator hrit;
    for (htit = harqTxBuffers_.begin(); htit != harqTxBuffers_.end(); ++htit)
        delete htit->second;
    for (hrit = harqRxBuffers_.begin(); hrit != harqRxBuffers_.end(); ++hrit)
        delete hrit->second;
    harqTxBuffers_.clear();
    harqRxBuffers_.clear();
}

void LteMacBase::sendUpperPackets(cPacket* pkt)
{
    //std::cout << "LteMacBase::sendUpperPackets start at " << simTime().dbl() << std::endl;

    //EV << "LteMacBase : Sending packet " << pkt->getName() << " on port MAC_to_RLC\n";
    // Send message
    send(pkt,up_[OUT]);
    emit(sentPacketToUpperLayer, pkt);

    //std::cout << "LteMacBase::sendUpperPackets end at " << simTime().dbl() << std::endl;
}

void LteMacBase::sendLowerPackets(cPacket* pkt)
{
    //std::cout << "LteMacBase::sendLowerPackets start at " << simTime().dbl() << std::endl;

    //EV << "LteMacBase : Sending packet " << pkt->getName() << " on port MAC_to_PHY\n";
    // Send message
    updateUserTxParam(pkt);
    send(pkt,down_[OUT]);
    emit(sentPacketToLowerLayer, pkt);

    //std::cout << "LteMacBase::sendLowerPackets end at " << simTime().dbl() << std::endl;
}

/*
 * Upper layer handler
 */
void LteMacBase::fromRlc(cPacket *pkt)
{
    //std::cout << "LteMacBase::fromRlc start at " << simTime().dbl() << std::endl;

    handleUpperMessage(pkt);

    //std::cout << "LteMacBase::fromRlc end at " << simTime().dbl() << std::endl;
}

/*
 * Lower layer handler
 */
void LteMacBase::fromPhy(cPacket *pkt)
{
    // TODO: harq test (comment fromPhy: it has only to pass pdus to proper rx buffer and
    // to manage H-ARQ feedback)

    //std::cout << "LteMacBase::fromPhy start at " << simTime().dbl() << std::endl;

    UserControlInfo *userInfo = check_and_cast<UserControlInfo *>(pkt->getControlInfo());
    MacNodeId src = userInfo->getSourceId();
    //MacNodeId dest = userInfo->getDestId();

    if (userInfo->getFrameType() == HARQPKT)
    {
        // H-ARQ feedback, send it to TX buffer of source
        HarqTxBuffers::iterator htit = harqTxBuffers_.find(src);
        //EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
        if (htit == harqTxBuffers_.end())
        {
            // if a feedback arrives, a tx buffer must exists (unless it is an handover scenario
            // where the harq buffer was deleted but a feedback was in transit)
            // this case must be taken care of

            if (binder_->hasUeHandoverTriggered(nodeId_) || binder_->hasUeHandoverTriggered(src))
                return;

//            delete userInfo;
            delete pkt;
            return;
            //throw cRuntimeError("Mac::fromPhy(): Received feedback for an unexisting H-ARQ tx buffer");
        }
		LteHarqFeedback *hfbpkt = check_and_cast<LteHarqFeedback *>(pkt);
		htit->second->receiveHarqFeedback(hfbpkt);
    }
    else if (userInfo->getFrameType() == FEEDBACKPKT)
    {
        //Feedback pkt
        //EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received feedback pkt" << endl;
        macHandleFeedbackPkt(pkt);
    }
    else if (userInfo->getFrameType()==GRANTPKT)
    {
        //Scheduling Grant
        //EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received Scheduling Grant pkt" << endl;
        macHandleGrant(pkt);
    }
    else if(userInfo->getFrameType() == DATAPKT)
    {
        // data packet: insert in proper rx buffer
        //EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received DATA packet" << endl;

        LteMacPdu *pdu = check_and_cast<LteMacPdu *>(pkt);
        Codeword cw = userInfo->getCw();
        HarqRxBuffers::iterator hrit = harqRxBuffers_.find(src);

        if (hrit != harqRxBuffers_.end())
        {
            hrit->second->insertPdu(cw,pdu);
        }
        else
        {
            LteHarqBufferRx *hrb;
            if (userInfo->getDirection() == DL || userInfo->getDirection() == UL)
                hrb = new LteHarqBufferRx(harqProcesses_, this,src);
            else // D2D
                hrb = new LteHarqBufferRxD2D(harqProcesses_, this,src, (userInfo->getDirection() == D2D_MULTI) );

            hrb->insertPdu(cw,pdu);
            harqRxBuffers_[src] = hrb;
        }
    }
    else if (userInfo->getFrameType() == RACPKT)
    {
        //EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received RAC packet" << endl;
        macHandleRac(pkt);
    }
    else
    {
        throw cRuntimeError("Unknown packet type %d", (int)userInfo->getFrameType());
    }

    //std::cout << "LteMacBase::fromPhy end at " << simTime().dbl() << std::endl;
}

bool LteMacBase::bufferizePacket(cPacket* pkt)
{
    //std::cout << "LteMacBase::bufferizePacket start at " << simTime().dbl() << std::endl;

    pkt->setTimestamp();        // Add timestamp with current time to packet

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // build the virtual packet corresponding to this incoming packet
    PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);
        take(queue);
        LteMacBuffer* vqueue = new LteMacBuffer();

        queue->pushBack(pkt);
        vqueue->pushBack(vpkt);
        mbuf_[cid] = queue;
        macBuffers_[cid] = vqueue;

        // make a copy of lte control info and store it to traffic descriptors map
        FlowControlInfo toStore(*lteInfo);
        connDesc_[cid] = toStore;
        // register connection to lcg map.
        LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

        lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

        //EV << "LteMacBuffers : Using new buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;
        LteMacBuffer* vqueue = macBuffers_.find(cid)->second;
        if (!queue->pushBack(pkt))
        {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,sample);
            }
            else if (lteInfo->getDirection()==UL)
            {
                emit(macBufferOverflowUl_,sample);
            }
            else // D2D
            {
                emit(macBufferOverflowD2D_,sample);
            }

            //EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }
        vqueue->pushBack(vpkt);

        //EV << "LteMacBuffers : Using old buffer on node: " << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " << queue->getQueueSize() - queue->getByteLength() << "\n";
    }
        /// After bufferization buffers must be synchronized
    assert(mbuf_[cid]->getQueueLength() == macBuffers_[cid]->getQueueLength());

    //std::cout << "LteMacBase::bufferizePacket end at " << simTime().dbl() << std::endl;

    return true;
}

void LteMacBase::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent("deleteQueues");

    //std::cout << "LteMacBase::deleteQueues start at " << simTime().dbl() << std::endl;

    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); )
    {
        if (MacCidToNodeId(mit->first) == nodeId)
        {
            while (!mit->second->isEmpty())
            {
                cPacket* pkt = mit->second->popFront();
                delete pkt;
            }
            delete mit->second;        // Delete Queue
            mbuf_.erase(mit++);        // Delete Elem
        }
        else
        {
            ++mit;
        }
    }
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); )
    {
        if (MacCidToNodeId(vit->first) == nodeId)
        {
			if (vit->second != nullptr) {
				while (!vit->second->isEmpty())
					vit->second->popFront();
			}
			delete vit->second;        // Delete Queue
			macBuffers_.erase(vit++);        // Delete Elem
        }
        else
        {
            ++vit;
        }
    }

    // delete H-ARQ buffers
    HarqTxBuffers::iterator hit;
    for (hit = harqTxBuffers_.begin(); hit != harqTxBuffers_.end(); )
    {
        if (hit->first == nodeId)
        {
            delete hit->second; // Delete Queue
            harqTxBuffers_.erase(hit++); // Delete Elem
        }
        else
        {
            ++hit;
        }
    }
    HarqRxBuffers::iterator hit2;
    for (hit2 = harqRxBuffers_.begin(); hit2 != harqRxBuffers_.end();)
    {
        if (hit2->first == nodeId)
        {
            delete hit2->second; // Delete Queue
            harqRxBuffers_.erase(hit2++); // Delete Elem
        }
        else
        {
            ++hit2;
        }
    }

    std::multimap<LteTrafficClass, CidBufferPair> tmp;
    for(auto & var : lcgMap_){
        if(MacCidToNodeId(var.second.first) != nodeId){
            tmp.insert(std::make_pair(var.first,var.second));
        }
    }

    //only with real handover
    lcgMap_.clear();
    lcgMap_ = tmp;

    std::set<MacCid> cids;
    for(auto & var : connDesc_){
        if(MacCidToNodeId(var.first) == nodeId){
            cids.insert(var.first);
        	//connDesc_.erase(var.first);
        }
    }
    for(auto & var : cids){
    	connDesc_.erase(var);
    }

    cids.clear();
    for (auto & var : connDescIn_) {
        if (MacCidToNodeId(var.first) == nodeId) {
            cids.insert(var.first);
//        	connDescIn_.erase(cid);
        }
    }
	for (auto & var : cids) {
		connDescIn_.erase(var);
	}

    //std::cout << "LteMacBase::deleteQueues end at " << simTime().dbl() << std::endl;
}


/*
 * Main functions
 */
void LteMacBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        /* Gates initialization */
        up_[IN] = gate("RLC_to_MAC");
        up_[OUT] = gate("MAC_to_RLC");
        down_[IN] = gate("PHY_to_MAC");
        down_[OUT] = gate("MAC_to_PHY");

        /* Create buffers */
        queueSize_ = par("queueSize");

        /* Get reference to binder */
        binder_ = getBinder();

        /* Set The MAC MIB */

        muMimo_ = par("muMimo");

        harqProcesses_ = par("harqProcesses");

        /* Start TTI tick */
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);        // TTI TICK after other messages
//        scheduleAt(NOW + TTI, ttiTick_);
        scheduleAt(NOW + getBinder()->getTTI(), ttiTick_);
        totalOverflowedBytes_ = 0;
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

        rtxSignalised = false;

		if (getSimulation()->getSystemModule()->hasPar("useQosModel")) {
			if (getSimulation()->getSystemModule()->par("useQosModel").boolValue()) {
				//deactivate if qosModel scheduling is activated
				rtxSignalisedFlagEnabled = false;
			}else{
				rtxSignalisedFlagEnabled = par("rtxSignalisedFlagEnabled").boolValue();
			}
		}
    }
}

void LteMacBase::handleMessage(cMessage* msg)
{
    //std::cout << "LteMacBase::handleMessage start at " << simTime().dbl() << std::endl;

    if (msg->isSelfMessage())
    {
        handleSelfMessage();
        scheduleAt(NOW + getBinder()->getTTI(), ttiTick_);
        //std::cout << "LteMacBase::handleMessage end at " << simTime().dbl() << std::endl;
        return;
    }

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    //EV << "LteMacBase : Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();

    if (incoming == down_[IN])
    {
        // message from PHY_to_MAC gate (from lower layer)
        emit(receivedPacketFromLowerLayer, pkt);
        fromPhy(pkt);
    }
    else
    {
        // message from RLC_to_MAC gate (from upper layer)
        emit(receivedPacketFromUpperLayer, pkt);
        fromRlc(pkt);
    }

    //std::cout << "LteMacBase::handleMessage end at " << simTime().dbl() << std::endl;
//    return;
}

void LteMacBase::finish()
{
    EV_DEBUG << "LteMacBase - finishing.";
}

void LteMacBase::deleteModule(){
    EV_DEBUG << "LteMacBase - module deleted.";
    cancelAndDelete(ttiTick_);
    cSimpleModule::deleteModule();
}

