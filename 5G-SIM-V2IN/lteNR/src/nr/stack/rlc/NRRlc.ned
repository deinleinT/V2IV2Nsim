//
// Part of 5G-Sim-V2I/N
//


package nr.stack.rlc;

import lte.stack.rlc.*;

// 
// Compound module for the RLC layer of LTE Stack.
//
module NRRlc extends LteRlc
{
    parameters:
        @display("i=block/transport");
        string NRRlcUmType = default("NRRlcUm");
        LteRlcUmType = default("NRRlcUm");     // One of: "LteRlcUm", "LteRlcUmRealistic", "LteRlcUmRealisticD2D"      

}


// 
// UM Module
//
simple NRRlcUm extends LteRlcUm like ILteRlcUm
{
    parameters:
        @class("NRRlcUm");
        @display("i=block/wheelbarrow");
        
        
}
