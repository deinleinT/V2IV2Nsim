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

#ifndef _NRRXPDCPENTITY_H_
#define _NRRXPDCPENTITY_H_

#include "stack/pdcp_rrc/layer/entity/LteRxPdcpEntity.h"
#include "stack/dualConnectivityManager/DualConnectivityManager.h"
#include "common/timer/TTimer.h"

class LtePdcpRrcBase;

/*
 * Descriptor of the entity status
 * Taken from 3GPP TS 38.323
 */
struct PdcpRxWindowDesc
{
  public:

    //! Size of the reception window
    unsigned int windowSize_;
    //! Sno of the next SDU expected to be received (RX_NEXT)
    unsigned int rxNext_;
    //! Sno of the first SDU not delivered to upper layer (RX_DELIV)
    unsigned int rxDeliv_;
    //! Sno of the SDU following the SDU that triggered t_reordering (RX_REORD)
    unsigned int rxReord_;

    void clear(unsigned int i=0)
    {
        rxNext_ = i;
        rxDeliv_ = i;
        rxReord_ = i;
    }

    PdcpRxWindowDesc()
    {
        windowSize_ = 16;  // TODO make it configurable
        clear();
    }

};

enum PdcpRxTimerType
{
    REORDERING_T = 0
};

/**
 * @class NRPdcpEntity
 * @brief Entity for New Radio PDCP Layer
 *
 * This is the PDCP entity of LTE/NR Stack.
 *
 */
class NRRxPdcpEntity : public LteRxPdcpEntity
{
  protected:

    // if true, deliver packets to upper layer without reordering
    // NOTE: reordering can apply for Split Bearers only
    bool outOfOrderDelivery_;

    // The SDU enqueue buffer.
    cArray sduBuffer_;

    // For each SDU a received status variable is kept
    std::vector<bool> received_;

    // State variables
    PdcpRxWindowDesc rxWindowDesc_;

    // Timer to manage reordering of the PDUs
    TTimer t_reordering_;

    // Timeout for above timer
    double timeout_;

    // handler for PDCP SDU
    virtual void handlePdcpSdu(Packet* pdcpSdu);

  public:

    NRRxPdcpEntity();
    virtual ~NRRxPdcpEntity();

    virtual void initialize();

    virtual void handleMessage(cMessage *msg);
};

#endif
