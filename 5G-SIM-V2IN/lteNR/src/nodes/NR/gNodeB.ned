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


package lteNR.nodes.NR;

import lteNR.nodes.eNodeB;

// 
// gNodeB Module
//
module gNodeB extends eNodeB
{
    parameters:
        @networkNode();
        @display("i=device/antennatower;bgb=1260,600");

        gateway = default("upf");
        nodeType = "GNODEB";        // DO NOT CHANGE
        nicType = default("NRNicEnb");
}
