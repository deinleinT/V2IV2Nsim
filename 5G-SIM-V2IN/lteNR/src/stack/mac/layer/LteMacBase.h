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

#ifndef _LTE_LTEMACBASE_H_
#define _LTE_LTEMACBASE_H_

#include "common/LteCommon.h"

class LteHarqBufferTx;
class LteHarqBufferRx;
class LteBinder;
class FlowControlInfo;
class LteMacBuffer;

/**
 * Map associating a nodeId with the corresponding TX H-ARQ buffer.
 * Used in eNB, where there is more than one TX H-ARQ buffer.
 */
typedef std::map<MacNodeId, LteHarqBufferTx *> HarqTxBuffers;

/**
 * Map associating a nodeId with the corresponding RX H-ARQ buffer.
 * Used in eNB, where there is more than one RX H-ARQ buffer.
 */
typedef std::map<MacNodeId, LteHarqBufferRx *> HarqRxBuffers;

/*
 * MultiMap associating a LCG group with all connection belonging to it and
 * corresponding virtual buffer pointer
 */
typedef std::pair<MacCid, LteMacBuffer*> CidBufferPair;
typedef std::pair<LteTrafficClass, CidBufferPair> LcgPair;
typedef std::multimap<LteTrafficClass, CidBufferPair> LcgMap;

/**
 * @class LteMacBase
 * @brief MAC Layer
 *
 * This is the MAC layer of LTE Stack:
 * it performs buffering/sending packets.
 *
 * On each TTI, the handleSelfMessage() is called
 * to perform scheduling and other tasks
 */
class LteMacBase : public cSimpleModule
{
    friend class LteHarqBufferTx;
    friend class LteHarqBufferRx;
    friend class LteHarqBufferTxD2D;
    friend class LteHarqBufferRxD2D;

public:
    virtual bool & getRtxSignalised(){
    	return rtxSignalised;
    }

    virtual bool isRtxSignalisedEnabled(){
    	return rtxSignalisedFlagEnabled;
    }

	virtual void deleteOnHandoverRtxSignalised(MacNodeId node){
		rtxSignalisedMap.erase(node);
	}
	virtual void setRtxSignalised(MacNodeId node, bool flag) {
		rtxSignalisedMap[node] = flag;
	}
	virtual bool getRtxSignalised(MacNodeId node){
		if (rtxSignalisedFlagEnabled) {
			std::map<MacNodeId, bool>::iterator it;
			it = rtxSignalisedMap.find(node);
			if (it != rtxSignalisedMap.end()) {
				return rtxSignalisedMap[node];
			} else {
				return false;
			}
		} else {
			return false;
		}
	}
  protected:
    bool rtxSignalised;
    bool rtxSignalisedFlagEnabled;

    std::map<MacNodeId,bool> rtxSignalisedMap;

    unsigned int totalOverflowedBytes_;
    simsignal_t macBufferOverflowDl_;
    simsignal_t macBufferOverflowUl_;
    simsignal_t macBufferOverflowD2D_;
    simsignal_t receivedPacketFromUpperLayer;
    simsignal_t receivedPacketFromLowerLayer;
    simsignal_t sentPacketToUpperLayer;
    simsignal_t sentPacketToLowerLayer;
    simsignal_t measuredItbs_;

    /*
     * Data Structures
     */
    LteBinder *binder_;

    /*
     * Gates
     */
    cGate* up_[2];     /// RLC <--> MAC
    cGate* down_[2];   /// MAC <--> PHY

    /*
     * MAC MIB Params
     */
    bool muMimo_;

    int harqProcesses_;

    /// TTI self message
    cMessage* ttiTick_;

    /// MacNodeId
    MacNodeId nodeId_;

    /// MacCellId
    MacCellId cellId_;

    /// Mac Buffers maximum queue size
    int queueSize_;

    /// Mac Sdu Real Buffers
    LteMacBuffers mbuf_;

    /// Mac Sdu Virtual Buffers
    LteMacBufferMap macBuffers_;

    /// List of pdus finalized for each user on each codeword
    MacPduList macPduList_;

    /// Harq Tx Buffers
    HarqTxBuffers harqTxBuffers_;
//    unsigned char lastHarqProcess_;
//    unsigned char lastAcid_;
//    bool retransmission_;

    /// Harq Rx Buffers
    HarqRxBuffers harqRxBuffers_;

    /* Connection Descriptors
     * Holds flow related infos
     */
    std::map<MacCid, FlowControlInfo> connDesc_;

    /* Incoming Connection Descriptors:
     * a connection is stored at the first MAC SDU delivered to the RLC
     */
    std::map<MacCid, FlowControlInfo> connDescIn_;

    /* LCG to CID and buffers map - used for supporting LCG - based scheduler operations
     * TODO : delete/update entries on hand-over
     */
    LcgMap lcgMap_;
    // Node Type;
    LteNodeType nodeType_;

    // record the last TTI that HARQ processes for a given UE have been aborted (useful for D2D switching)
    std::map<MacNodeId, simtime_t> resetHarq_;

  public:

    /**
     * Initializes MAC Buffers
     */
    LteMacBase();

    /**
     * Deletes MAC Buffers
     */
    virtual ~LteMacBase();

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId);

    //* public utility function - drops ownership of an object
    void dropObj(cOwnedObject* obj)
    {
        Enter_Method_Silent("dropObj");

        //std::cout << "LteMacBase::dropObj at " << simTime().dbl() << std::endl;

        drop(obj);
    }

    //* public utility function - takes ownership of an object
    void takeObj(cOwnedObject* obj)
    {
        Enter_Method_Silent("takeObj");

        //std::cout << "LteMacBase::takeObj at " << simTime().dbl() << std::endl;

        take(obj);
    }

    /*
     * Getters
     */

    MacNodeId getMacNodeId()
    {
        Enter_Method_Silent("getMacNodeId");

        //std::cout << "LteMacBase::getMacNodeId at " << simTime().dbl() << std::endl;

        return nodeId_;
    }
    MacCellId getMacCellId()
    {
        Enter_Method_Silent("getMacCellId");

        //std::cout << "LteMacBase::getMacCellId at " << simTime().dbl() << std::endl;

        return cellId_;
    }

    // Returns the virtual buffers
    LteMacBufferMap* getMacBuffers()
    {
        Enter_Method_Silent("getMacBuffers");

        //std::cout << "LteMacBase::getMacBuffers at " << simTime().dbl() << std::endl;

        return &macBuffers_;
    }

    // Returns Traffic Class to cid mapping
    LcgMap& getLcgMap()
    {
        Enter_Method_Silent("getLcgMap");

        //std::cout << "LteMacBase::getLcgMap at " << simTime().dbl() << std::endl;

        return lcgMap_;
    }

    // Returns connection descriptors
    std::map<MacCid, FlowControlInfo>& getConnDesc()
    {
        Enter_Method_Silent("getConnDesc");

        //std::cout << "LteMacBase::getConnDesc at " << simTime().dbl() << std::endl;

        return connDesc_;
    }

    // Returns connection descriptors
    std::map<MacCid, FlowControlInfo>& getConnDescIn() {
        Enter_Method_Silent
        ("getConnDescIn");

        //std::cout << "LteMacBase::getConnDesc at " << simTime().dbl() << std::endl;

        return connDescIn_;
    }

    // Returns the harq tx buffers
    HarqTxBuffers* getHarqTxBuffers()
    {
        Enter_Method_Silent("getHarqTxBuffers");

        //std::cout << "LteMacBase::getHarqTxBuffers at " << simTime().dbl() << std::endl;

        return &harqTxBuffers_;
    }

    // Returns the harq tx buffers
    HarqRxBuffers* getHarqRxBuffers()
    {
        Enter_Method_Silent("getHarqRxBuffers");

        //std::cout << "LteMacBase::getHarqRxBuffers at " << simTime().dbl() << std::endl;

        return &harqRxBuffers_;
    }

    // Returns number of Harq Processes
    unsigned int harqProcesses() const
    {
        Enter_Method_Silent("harqProcesses");

        //std::cout << "LteMacBase::harqProcesses at " << simTime().dbl() << std::endl;

        return harqProcesses_;
    }

    // Returns the MU-MIMO enabled flag
    bool muMimo() const
    {
        Enter_Method_Silent("muMimo");

        //std::cout << "LteMacBase::muMimo at " << simTime().dbl() << std::endl;

        return muMimo_;
    }

    LteNodeType getNodeType()
    {
        Enter_Method_Silent("getNodeType");

        //std::cout << "LteMacBase::getNodeType at " << simTime().dbl() << std::endl;

        return nodeType_;
    }

    void emitItbs( unsigned int iTbs )
    {
        Enter_Method_Silent("emitItbs");

        //std::cout << "LteMacBase::emitItbs at " << simTime().dbl() << std::endl;

        emit( measuredItbs_ , iTbs );
    }

    virtual bool isD2DCapable()
    {
        Enter_Method_Silent("isD2DCapable");

        //std::cout << "LteMacBase::isD2DCapable at " << simTime().dbl() << std::endl;

        return false;
    }

    // check whether HARQ processes have been aborted during this TTI
    bool isHarqReset(MacNodeId srcId)
    {
        Enter_Method_Silent("isHarqReset");

        //std::cout << "LteMacBase::isHarqReset at " << simTime().dbl() << std::endl;

        if (resetHarq_.find(srcId) != resetHarq_.end())
        {
            if (resetHarq_[srcId] == NOW)
                return true;
        }
        return false;
    }

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }

    /**
     * Grabs NED parameters, initializes gates
     * and the TTI self message
     */
    virtual void initialize(int stage);

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(cMessage *msg);


    /**
     * Statistics recording
     */
    virtual void finish();

    /**
     * Deleting the module
     *
     * Method is overridden in order to cancel the periodic TTI self-message,
     * afterwards the deleteModule method of cSimpleModule is called.
     */
    virtual void deleteModule();

    /**
     * Main loop of the Mac level, calls the scheduler
     * and every other function every TTI : must be reimplemented
     * by derivate classes
     */
    virtual void handleSelfMessage() = 0;

    /**
     * sendLowerPackets() is used
     * to send packets to lower layer
     *
     * @param pkt Packet to send
     */
    virtual void sendLowerPackets(cPacket* pkt);

    /**
     * sendUpperPackets() is used
     * to send packets to upper layer
     *
     * @param pkt Packet to send
     */
    virtual void sendUpperPackets(cPacket* pkt);

    /*
     * Functions to be redefined by derivated classes
     */

    virtual void macPduMake(MacCid cid = 0) = 0;
    virtual void macPduUnmake(cPacket* pkt) = 0;

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(cPacket* pkt);

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(cPacket* pkt)
    {
        bufferizePacket(pkt);
    }

    /**
     * macHandleFeedbackPkt is called every time a feedback pkt arrives on MAC
     */
    virtual void macHandleFeedbackPkt(cPacket* pkt)
    {
    }

    /*
     * Receives and handles scheduling grants - implemented in LteMacUe
     */
    virtual void macHandleGrant(cPacket* pkt)
    {
    }

    /*
     * Receives and handles RAC requests (eNodeB implementation)  and responses (LteMacUe implementation)
     */
    virtual void macHandleRac(cPacket* pkt)
    {
    }

    /*
     * Update UserTxParam stored in every lteMacPdu when an rtx change this information
     */
    virtual void updateUserTxParam(cPacket* pkt)=0;

  protected:

    /// Upper Layer Handler
    virtual void fromRlc(cPacket *pkt);

    /// Lower Layer Handler
    virtual void fromPhy(cPacket *pkt);
};

#endif
