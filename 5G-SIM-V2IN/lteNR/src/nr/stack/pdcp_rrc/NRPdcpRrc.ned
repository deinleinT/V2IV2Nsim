//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nürnberg (FAU), 
// Computer Science 7 - Computer Networks and Communication Systems
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

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
