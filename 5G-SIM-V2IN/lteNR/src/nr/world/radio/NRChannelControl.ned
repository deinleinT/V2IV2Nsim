//
// Part of 5G-Sim-V2I/N
//

package nr.world.radio;

import lte.world.radio.LteChannelControl;

simple NRChannelControl extends LteChannelControl
{
    parameters:       
        @display("i=misc/sun");
        @labels(node);
        @class("NRChannelControl");
}