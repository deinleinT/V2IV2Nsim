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

#ifndef _LTE_LTERLCUM_H_
#define _LTE_LTERLCUM_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/rlc/packet/LteRlcSdu_m.h"
#include "stack/rlc/um/entity/UmTxEntity.h"
#include "stack/rlc/um/entity/UmRxEntity.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/layer/LteMacBase.h"
#include "nr/stack/sdap/utils/QosHandler.h"

class UmTxEntity;
class UmRxEntity;

/**
 * @class LteRlcUm
 * @brief UM Module
 *
 * This is the UM Module of RLC.
 * It implements the unacknowledged mode (UM):
 *
 * - Unacknowledged mode (UM):
 *   This mode is used for data traffic. Packets arriving on
 *   this port have been already assigned a CID.
 *   UM implements fragmentation and reassembly of packets.
 *   To perform this task there is a TxEntity module for
 *   every CID = <NODE_ID,LCID>. RLC PDUs are created by the
 *   sender and reassembly is performed at the receiver by
 *   simply returning him the original packet.
 *   Traffic on this port is then forwarded on ports
 *
 *   UM mode attaches an header to the packet. The size
 *   of this header is fixed to 2 bytes.
 *
 */
class LteRlcUm : public cSimpleModule
{
  public:
    LteRlcUm()
    {
    }
    virtual ~LteRlcUm()
    {
    }

    virtual void recordUETotalRlcThroughputUl(double length){
    	this->totalRcvdBytesUl += length;
    	double tp = totalRcvdBytesUl / (NOW - getSimulation()->getWarmupPeriod());
    	ueTotalRlcThroughputUl.record(tp);
    }

	virtual void recordUETotalRlcThroughputDl(double length) {
		this->totalRcvdBytesDl += length;
		double tp = totalRcvdBytesDl / (NOW - getSimulation()->getWarmupPeriod());
		ueTotalRlcThroughputDl.record(tp);
	}

	virtual void recordConnectedCellUes(double number){
		cellConnectedUes.record(number);
	}


    /**
     * sendDefragmented() is invoked by the RXBuffer as a direct method
     * call and used to forward fragments to upper layers. This is needed
     * since the RXBuffer himself has no output gates
     *
     * @param pkt packet to forward
     */
    virtual void sendDefragmented(cPacket *pkt);

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId);

    /**
     * sendToLowerLayer() is invoked by the TXEntity as a direct method
     * call and used to forward fragments to lower layers. This is needed
     * since the TXBuffer himself has no output gates
     *
     * @param pkt packet to forward
     */
    virtual void sendToLowerLayer(cPacket *pkt);

    virtual void resumeDownstreamInPackets(MacNodeId peerId) {}

    virtual bool isEmptyingTxBuffer(MacNodeId peerId) { return false; }


    /**
     * Initialize watches
     */
    virtual void initialize();

    virtual void finish()
    {
    }

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(cMessage *msg);

  protected:
    QosHandler * qosHandler;

    cGate* up_[2];
    cGate* down_[2];

    // statistics
    simsignal_t receivedPacketFromUpperLayer;
    simsignal_t receivedPacketFromLowerLayer;
    simsignal_t sentPacketToUpperLayer;
    simsignal_t sentPacketToLowerLayer;

    /**
     * getTxBuffer() is used by the sender to gather the TXBuffer
     * for that CID. If TXBuffer was already present, a reference
     * is returned, otherwise a new TXBuffer is created,
     * added to the tx_buffers map and a reference is returned aswell.
     *
     * @param lteInfo flow-related info
     * @return pointer to the TXBuffer for the CID of the flow
     *
     */
    virtual UmTxEntity* getTxBuffer(FlowControlInfo* lteInfo);

    /**
     * getRxBuffer() is used by the receiver to gather the RXBuffer
     * for that CID. If RXBuffer was already present, a reference
     * is returned, otherwise a new RXBuffer is created,
     * added to the rx_buffers map and a reference is returned aswell.
     *
     * @param lteInfo flow-related info
     * @return pointer to the RXBuffer for that CID
     *
     */
    virtual UmRxEntity* getRxBuffer(FlowControlInfo* lteInfo);

    //added for individual Throughput
    cOutVector ueTotalRlcThroughputDl;
    cOutVector ueTotalRlcThroughputUl;

    //time when the first packet for throughput calculation was sent
    simtime_t ueTotalRlcThroughputDlStartTime;
    simtime_t ueTotalRlcThroughputUlStartTime;

    //for checking the vector is already initialized
    bool ueTotalRlcThroughputDlInit;
    bool ueTotalRlcThroughputUlInit;

    double totalRcvdBytesUl;
    double totalRcvdBytesDl;

    cOutVector totalCellRlcThroughputUl;
	static double totalCellRcvdBytesUl;
	static double totalCellRcvdBytesDl;
	cOutVector totalCellRlcThroughputDl;

	double numberOfConnectedUes;
	cOutVector cellConnectedUes;



    /**
     * handler for traffic coming
     * from the upper layer (PDCP)
     *
     * handleUpperMessage() performs the following tasks:
     * - Adds the RLC-UM header to the packet, containing
     *   the CID, the Traffic Type and the Sequence Number
     *   of the packet (extracted from the IP Datagram)
     * - Search (or add) the proper TXBuffer, depending
     *   on the packet CID
     * - Calls the TXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleUpperMessage(cPacket *pkt);

    /**
     * UM Mode
     *
     * handler for traffic coming from
     * lower layer (DTCH, MTCH, MCCH).
     *
     * handleLowerMessage() performs the following task:
     *
     * - Search (or add) the proper RXBuffer, depending
     *   on the packet CID
     * - Calls the RXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleLowerMessage(cPacket *pkt);

    /*
     * Data structures
     */

    /**
     * The entities map associate each CID with
     * a TX/RX Entity , identified by its ID
     */
    typedef std::map<MacCid, UmTxEntity*> UmTxEntities;
    typedef std::map<MacCid, UmRxEntity*> UmRxEntities;
    UmTxEntities txEntities_;
    UmRxEntities rxEntities_;

  public:

	virtual void exchangeEntities(MacNodeId nodeId, LteRlcUm * newMasterRlcUmRealistic, MacNodeId newMasterId, MacNodeId oldMasterId); //nodeB

	virtual void modifyEntitiesUe(MacNodeId nodeId, LteRlcUm * newMasterRlcUmRealistic, MacNodeId newMasterId, MacNodeId oldMasterId); //UE

};

#endif
