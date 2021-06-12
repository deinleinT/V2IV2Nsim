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

#ifndef _LTE_LTETXPDCPENTITY_H_
#define _LTE_LTETXPDCPENTITY_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

class LtePdcpRrcBase;
class LtePdcpPdu;

using namespace inet;

/**
 * @class LteTxPdcpEntity
 * @brief Entity for PDCP Layer
 *
 * This is the PDCP entity of LTE Stack.
 *
 * PDCP entity performs the following tasks:
 * - mantain numbering of one logical connection
 *
 */
class LteTxPdcpEntity : public cSimpleModule
{
  protected:
    // reference to the PDCP layer
    LtePdcpRrcBase* pdcp_;

    // next sequence number to be assigned
    unsigned int sno_;

    // deliver the PDCP PDU to the lower layer
    virtual void deliverPdcpPdu(Packet* pdcpPkt);

    virtual void setIds(FlowControlInfo* lteInfo);

  public:

    LteTxPdcpEntity();
    virtual ~LteTxPdcpEntity();

    virtual void initialize();

    // create a PDCP PDU from the IP datagram
    void handlePacketFromUpperLayer(Packet* pkt);
};

#endif
