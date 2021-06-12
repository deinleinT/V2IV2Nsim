/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "corenetwork/gtp/GtpUser.h"
#include "nr/common/binder/NRBinder.h"

class GtpUserNR : public GtpUser
{

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);

    // receive and IP Datagram from the traffic filter, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    virtual void handleFromTrafficFlowFilter(inet::Packet * datagram);

    // receive a GTP-U packet from UDP, reads the TEID and decides whether performing label switching or removal
    virtual void handleFromUdp(inet::Packet * pkt);
};
