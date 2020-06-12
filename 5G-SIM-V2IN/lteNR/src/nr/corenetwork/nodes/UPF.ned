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

package nr.corenetwork.nodes;


import inet.linklayer.ppp.PPPInterface;
import inet.node.inet.NodeBase;
import inet.transportlayer.udp.UDP;

import nr.epc.gtp.GtpUserNR;
import nr.epc.TrafficFlowFilterNR;
import nr.stack.sdap.utils.IQosHandler;

module UPF extends NodeBase
{
    parameters:
        string nodeType = default("UPF");
        string qosHandlerType = default("QosHandlerUPF");
        @display("bgb=920,462;i=device/mainframe");
    gates:
        inout filterGate @labels(PPPFrame-conn);

    submodules:
        trafficFlowFilter: TrafficFlowFilterNR {
            ownerType = nodeType;
            @display("p=813,206");
        }
        pppInterface: PPPInterface {
            @display("p=727,386");
        }
        udp: UDP {
            @display("p=329,206");
        }
        gtp_user: GtpUserNR {
            @display("p=591,206");
        }

        qosHandler: <qosHandlerType> like IQosHandler {
            @display("p=468.93,99.33");
        }
    connections:
        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;

        pppInterface.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppInterface.upperLayerIn <-- gtp_user.pppGate;
        pppInterface.phys <--> filterGate;

        udp.appOut++ --> gtp_user.udpIn;
        gtp_user.udpOut --> udp.appIn++;
        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
}
