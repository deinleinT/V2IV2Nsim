//***************************************************************************
// * file:        ChannelControl.ned
// *
// * author:      Andras Varga, Steffen Sroka, Daniel Willkomm
// *
// * copyright:   (C) 2004 Telecommunication Networks Group (TKN) at
// *              Technische Universitaet Berlin, Germany.
// *
// *              This program is free software; you can redistribute it
// *              and/or modify it under the terms of the GNU General Public
// *              License as published by the Free Software Foundation; either
// *              version 2 of the License, or (at your option) any later
// *              version.
// *              For further information see file COPYING
// *              in the top level directory
// ***************************************************************************
// * part of:     framework implementation developed by tkn
// **************************************************************************/


package lte.world.radio;

//
// ~ChannelControl has exactly one instance in every network model that
// contains mobile or wireless nodes. This module gets informed about the
// location and movement of nodes, and determines which nodes are within
// communication or interference distance. This info is then used by the
// radio interfaces of nodes at transmissions.
// Must be named as "channelControl" inside the network.
//
// This ~ChannelControl is a different implementation from the one in
// Mobility Framework 1.0a5: here we use sendDirect(), while the MF version
// used normal send() and dynamic connections.
//
// @author Andras Varga (based on MF's ChannelControl by Steffen Sroka and Daniel Willkomm)
// @see ~IMobility
//
simple ChannelControl
{
    parameters:
        bool coreDebug = default(false); // debug switch for core framework
        double pMax @unit("mW") = default(100000mW); // maximum sending power used for this network (in mW) --> 100000mW ~ 50dBm
        double sat @unit("dBm") = default(-80dBm); // signal attenuation threshold (in dBm)
        double alpha = default(2); // path loss coefficient
        //double carrierFrequency = default(2.4); // base carrier frequency of all the channels (in Hz)
//        double carrierFrequency = default(2.4); // base carrier frequency of all the channels (in Hz)
        int numChannels = default(1); // number of radio channels (frequencies)
        string propagationModel @enum("FreeSpaceModel","TwoRayGroundModel","RiceModel","RayleighModel","NakagamiModel","LogNormalShadowingModel") = default("FreeSpaceModel");
        @display("i=misc/sun");
        @labels(node);
}