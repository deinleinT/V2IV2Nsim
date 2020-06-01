/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "world/radio/LteChannelControl.h"


class NRChannelControl : public LteChannelControl
{
  protected:

    /** Calculate interference distance*/
    virtual double calcInterfDist();

    /** Reads init parameters and calculates a maximal interference distance*/
    virtual void initialize();

  public:
    NRChannelControl();
    virtual ~NRChannelControl();

    /** Called from ChannelAccess, to transmit a frame to all the radios in range, on the frame's channel */
    virtual void sendToChannel(RadioRef srcRadio, AirFrame *airFrame);
};

