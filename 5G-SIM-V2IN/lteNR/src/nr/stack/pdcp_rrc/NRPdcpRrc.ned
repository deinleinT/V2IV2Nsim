
//
// Part of 5G-Sim-V2I/N
//

package nr.stack.pdcp_rrc;

import lte.stack.pdcp_rrc.*;

moduleinterface NRPdcpRrc extends LtePdcpRrc{
    parameters:
        @display("i=block/fork");

    gates:
        inout upperLayerRRC; //from RRC Module
        inout lowerLayerRRC; //from RLC Module

}

simple NRPdcpRrcBase extends LtePdcpRrcBase like NRPdcpRrc{
    parameters:
        @display("i=block/fork");

        //#
        //# Statistic recording: end2end delay and throughput at the mac layer
        //#
        @signal[pdcpdrop0];
        @statistic[pdcpdrop0](title="Drop at PDCP layer 0"; unit=""; source="pdcpdrop0"; record=mean);
        @signal[pdcpdrop1];
        @statistic[pdcpdrop1](title="Drop at PDCP layer 1"; unit=""; source="pdcpdrop1"; record=mean);
        @signal[pdcpdrop2];
        @statistic[pdcpdrop2](title="Drop at PDCP layer 2"; unit=""; source="pdcpdrop2"; record=mean);
        @signal[pdcpdrop3];
        @statistic[pdcpdrop3](title="Drop at PDCP layer 3"; unit=""; source="pdcpdrop3"; record=mean);
        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);

}

simple NRPdcpRrcUe extends NRPdcpRrcBase {
        @class("NRPdcpRrcUe");

}

simple NRPdcpRrcGnb extends NRPdcpRrcBase {
    @class("NRPdcpRrcGnb");
}
