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
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.IPv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IUDPApp;
import inet.applications.contract.ITCPApp;
import inet.applications.contract.ISCTPApp;
import inet.transportlayer.udp.UDP;
import inet.transportlayer.tcp.TCP;
import inet.transportlayer.sctp.SCTP;
import lte.x2.LteX2App;
import lte.epc.gtp.GtpUserX2;

import nr.corenetwork.cellInfo.NRCellInfo;
import nr.epc.gtp.GtpUserNR;
import nr.epc.TrafficFlowFilterNR;
import nr.stack.*;

// 
// gNodeB Module
//
module gNodeB
{
    parameters:
        @networkNode();
        @display("i=device/antennatower;bgb=800,500;bgl=2");

        //# Node specs
        string nodeType = "ENODEB";        // DO NOT CHANGE
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(40000mw);
        bool d2dCapable = default(false);
        string nicType = d2dCapable ? "NRNicGnbD2D" : "NRNicGnb";

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        string routingTableType = default("IPv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");

        //# Num apps
        int numTcpApps = default(0);  // no of TCP apps. Specify the app types in INI file with tcpApp[0..1].typename="TCPEchoApp" syntax
        int numUdpApps = default(0);  // no of UDP apps. Specify the app types in INI file with udpApp[0..1].typename="UDPVideoStreamCli" syntax
        int numX2Apps = default(0);   // no of X2 apps. Specify the app types in INI file with x2App[0..1].typename="X2AppClient" syntax

    gates:
        inout ppp;                // connection to the Core Network
        input radioIn @directIn;  // connection to the radio interface
        inout x2[];               // connection to the X2 interface

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }

        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=50,125;is=s");
        }
        mobility: StationaryMobility {
            @display("p=50,175;is=s");
        }

        cellInfo: NRCellInfo {
            @display("p=50,225;is=s");
        }

        //#
        //# NRNic modules
        //#
        lteNic: <nicType> like INRNic {
            nodeType = nodeType;
            //d2dCapable = d2dCapable;
            @display("p=250,407");
        }

        pppIf: PPPInterface {
            @display("p=501,363");
        }

        x2ppp[sizeof(x2)]: PPPInterface {
            @display("p=683,363");
        }

        //#
        //# Network layer module
        //#
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=501,269;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        udpApp[numUdpApps]: <> like IUDPApp {
            @display("p=555,48,row");
        }
        udp: UDP {
            @display("p=501,148");
        }

        // =============== TCP ============== //
        tcpApp[numTcpApps]: <> like ITCPApp {
            @display("p=683,48,row");
        }
        tcp: TCP if numTcpApps>0 {
            @display("p=683,148");
        }

	    // =============== X2AP ============== //
        x2App[numX2Apps]: LteX2App {
            @display("p=155,48,row");
        }
        sctp: SCTP {
            @display("p=275,148");
        }

        //# 
        //# Simplified EPC
        //# 
        trafficFlowFilter: TrafficFlowFilterNR {
            ownerType = nodeType;
            @display("p=330,48");
        }
        gtpUser: GtpUserNR {
            @display("p=434,48");
        }

        gtpUserX2: GtpUserX2 {
            @display("p=410,363");
        }

    connections allowunconnected:

        //#
        //# NR stack to PHY and network layer
        //#     

        lteNic.radioIn <-- radioIn;
        lteNic.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        lteNic.upperLayerIn <-- gtpUser.pppGate;
        trafficFlowFilter.gtpUserGateOut --> gtpUser.trafficFlowFilterGate;
        gtpUser.udpOut --> udp.appIn++;
        gtpUser.udpIn <-- udp.appOut++;


        //#
        //# PPP interface to network layer connections
        //#    
        pppIf.phys <--> ppp;
        pppIf.upperLayerOut --> networkLayer.ifIn++;
        pppIf.upperLayerIn <-- networkLayer.ifOut++;


        gtpUserX2.lteStackOut --> lteNic.x2$i++;
        gtpUserX2.lteStackIn <-- lteNic.x2$o++;
        gtpUserX2.udpOut --> udp.appIn++;
        gtpUserX2.udpIn <-- udp.appOut++;

        //#
        //# X2 interface to network layer connections
        //#    
        for i=0..sizeof(x2)-1 {
            x2ppp[i].phys <--> x2[i];
            x2ppp[i].upperLayerOut --> networkLayer.ifIn++;
            x2ppp[i].upperLayerIn <-- networkLayer.ifOut++;
        }

		//# 
		//# Apps to transport layer connections
		//#
        for i=0..numTcpApps-1 {
            tcpApp[i].tcpOut --> tcp.appIn++;
            tcpApp[i].tcpIn <-- tcp.appOut++;
        }
        for i=0..numUdpApps-1 {
            udpApp[i].udpOut --> udp.appIn++;
            udpApp[i].udpIn <-- udp.appOut++;
        }
        for i=0..numX2Apps-1 {
            x2App[i].sctpOut[0] --> sctp.from_appl++;
            x2App[i].sctpIn[0] <-- sctp.to_appl++;

            x2App[i].sctpOut[1] --> sctp.from_appl++;
            x2App[i].sctpIn[1] <-- sctp.to_appl++;
            x2App[i].x2ManagerIn <-- lteNic.x2$o++;
            x2App[i].x2ManagerOut --> lteNic.x2$i++;
        }

        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;
        tcp.ipOut --> networkLayer.transportIn++ if numTcpApps>0;
        tcp.ipIn <-- networkLayer.transportOut++ if numTcpApps>0;
        sctp.to_ip --> networkLayer.transportIn++ if numX2Apps>0;
        sctp.from_ip <-- networkLayer.transportOut++ if numX2Apps>0;


}
