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

import lteNR.nodes.Ue;
import lteNR.stack.NRNicUe;

// 
// User Equipment Module
//
module NRUe extends Ue
{
    parameters:
        nicType = default("NRNicUe");
        
        int nrMasterId = default(0);
        int nrMacNodeId = default(0); // TODO: this is not a real parameter
        int nrMacCellId = default(0); // TODO: this is not a real parameter
    
    gates:
        input nrRadioIn @directIn;     // connection to master    

    submodules:
        //# UeStatsCollector - for MEC
//        NRueCollector: NRueStatsCollector if hasRNISupport {
//            @display("p=73.687996,445.75198;is=s");
//        }

    connections allowunconnected:

        cellularNic.nrRadioIn <-- nrRadioIn;
}
