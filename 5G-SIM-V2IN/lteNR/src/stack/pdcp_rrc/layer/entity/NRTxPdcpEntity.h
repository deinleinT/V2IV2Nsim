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

#ifndef _NRTXPDCPENTITY_H_
#define _NRTXPDCPENTITY_H_

#include "stack/pdcp_rrc/layer/entity/LteTxPdcpEntity.h"
#include "stack/dualConnectivityManager/DualConnectivityManager.h"

class LtePdcpRrcBase;

/**
 * @class NRPdcpEntity
 * @brief Entity for New Radio PDCP Layer
 *
 * This is the PDCP entity of LTE/NR Stack.
 *
 * PDCP entity performs the following tasks:
 * - mantain numbering of one logical connection
 *
 */
class NRTxPdcpEntity : public LteTxPdcpEntity
{
  protected:

    // deliver the PDCP PDU to the lower layer or to the X2
    virtual void deliverPdcpPdu(Packet* pkt);

    virtual void setIds(FlowControlInfo* lteInfo);

  public:

    NRTxPdcpEntity();
    virtual ~NRTxPdcpEntity();

    virtual void initialize();
};

#endif
