//
// Part of 5G-Sim-V2I/N
//

package nr.epc;

simple TrafficFlowFilterNR
{
    parameters:
        @display("i=block/filter");
        @class("TrafficFlowFilterNR");

        string ownerType;
        bool fastForwarding = default(true);
    gates:
        input internetFilterGateIn;
        output gtpUserGateOut;    
}

