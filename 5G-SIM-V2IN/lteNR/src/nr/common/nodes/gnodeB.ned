//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU), Computer Science 7 - Computer Networks and Communication Systems
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

package lteNR.nr.common.nodes;

import inet.linklayer.ppp.PppInterface;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import inet.transportlayer.sctp.Sctp;
import inet.common.MessageDispatcher;
import lteNR.x2.LteX2App;
import lteNR.stack.ICellularNic;
import lteNR.nr.common.cellInfo.NRCellInfo;
import lteNR.nr.corenetwork.gtp.GtpUserNR;
import lteNR.corenetwork.gtp.GtpUserX2;
import lteNR.nr.corenetwork.trafficFlowFilter.TrafficFlowFilterNR;

// 
// gnodeB Module --> own definition for 5G-Sim-V2IN to be able to modify the modules and parameters
//
module gnodeB
{
    parameters:
        @networkNode;
        @labels(node,ethernet-node,wireless-node);
        @display("i=device/antennatower;bgb=1805.475,1228.5375");
        @figure[applicationLayer](type=rectangle; pos=250,6; size=1000,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=1000,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");
        @figure[networkLayer](type=rectangle; pos=250,306; size=1000,130; fillColor=#00ff00; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[networkLayer.title](type=text; pos=1245,311; anchor=ne; text="network layer");
        @figure[linkLayer](type=rectangle; pos=250,456; size=1000,130; fillColor=#0000ff; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[linkLayer.title](type=text; pos=1245,461; anchor=ne; text="link layer");
        @figure[submodules];

        //# Node specs
        string nodeType = default("GNODEB");        // DO NOT CHANGE
        string gateway = default("upf");
        int masterId = default(0);  // ID of the master eNodeB. Used in case of Dual Connectivity 
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(40000mw);
        string nicType = default("NRnicGNB");

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default("^.ipv4.routingTable");
        *.forwarding = true;
        *.multicastForwarding = false;

        //# Apps
        int numApps = default(0);     // no of apps. 
        int numX2Apps = default(0);   // no of X2 apps. Specify the app types in INI file with x2App[0..1].typename="X2AppClient" syntax

        //# Transport layer
        bool hasUdp = true;
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = true;

        //# MEC support	
        string meHost = default("");		// symbolic name of the connected MEHost in the Network

    gates:
        inout ppp @loose;         // connection to the Core Network
        input radioIn @directIn;  // connection to the radio interface
        inout x2[];               // connection to the X2 interface

        //# MEC support
        inout pppMEHost @loose;		// connection to the GtpEndpoint module inside MEHost

    submodules:
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=117.65,658.3875;is=s");
        }
        mobility: StationaryMobility {
            parameters:
                @display("p=117.65,414.0375;is=s");
        }
        cellInfo: NRCellInfo {
            parameters:
                @display("p=117.65,533.95;is=s");
        }

        //#
        //# cellularNic modules
        //#
        cellularNic: <nicType> like ICellularNic {
            parameters:
                @display("p=371.05002,1033.9625");
                nodeType = nodeType;
        }

        pppIf: PppInterface {
            parameters:
                @display("p=837.125,1033.9625");
        }

        x2ppp[sizeof(x2)]: PppInterface {
            parameters:
                @display("p=1038.4875,1033.9625");
        }

        //#
        //# Mec interface	
        //#
        pppMEHostIf: PppInterface {
            @display("p=640.28754,1033.9625");
        }

        //#
        //# Network layer module
        //#
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=493.225,676.4875;q=queue");
        }

        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=660.65,676.4875;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== Udp ============== //
        app[numApps]: <> like IApp {
            parameters:
                @display("p=950.25,126.700005,row,140");
        }
        udp: Udp {
            parameters:
                @display("p=694.5875,395.9375");
        }
        tcp: Tcp if hasTcp {
            parameters:
                @display("p=1212.7001,395.9375");
        }

	    // =============== X2AP ============== //
        x2App[numX2Apps]: LteX2App {
            parameters:
                @display("p=527.16254,126.700005,row");
        }
        sctp: Sctp {
            parameters:
                @display("p=384.625,395.9375");
        }

	    // message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=776.03754,291.86252;b=1000,5,,,,1");
        }
        // message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=776.03754,561.10004;b=1000,5,,,,1");
        }
        // message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=746.625,914.05;b=1000,5,,,,1");
        }

        //# 
        //# Modules for connecting to the core network and X2
        //# 
        trafficFlowFilter: TrafficFlowFilterNR {
            parameters:
                @display("p=1377.8625,676.4875");
                ownerType = nodeType;
        }
        gtpUser: GtpUserNR {
            parameters:
                @display("p=1156.1376,676.4875");
                ipOutInterface = "wlan";       // FIXME
        }

        gtpUserX2: GtpUserX2 {
            parameters:
                @display("p=986.45,676.4875");
        }

    connections allowunconnected:

        //#
        //# stack to PHY and network layer
        //#        
        cellularNic.radioIn <-- radioIn;

        //#
        //# Ppp interface to network layer connections
        //#    
        pppIf.phys <--> ppp;
        pppIf.upperLayerOut --> nl.in++;
        pppIf.upperLayerIn <-- nl.out++;

        //#
        //# X2 interface to network layer connections
        //#    
        for i=0..sizeof(x2)-1 {
            x2ppp[i].phys <--> x2[i];
            x2ppp[i].upperLayerOut --> tn.in++;
            x2ppp[i].upperLayerIn <-- tn.out++;
        }

        //#
        //# Connections to the core network
        //#  
        nl.in++ <-- gtpUser.pppGate;
        trafficFlowFilter.gtpUserGateOut --> gtpUser.trafficFlowFilterGate;
        trafficFlowFilter.internetFilterGateIn <-- nl.out++;
        gtpUser.socketOut --> at.in++;
        gtpUser.socketIn <-- at.out++;

	    //# GTPUserX2 connections
        gtpUserX2.lteStackOut --> cellularNic.x2$i++;
        gtpUserX2.lteStackIn <-- cellularNic.x2$o++;
        gtpUserX2.socketOut --> at.in++;
        gtpUserX2.socketIn <-- at.out++;

        //# X2 apps connections        
        for i=0..numX2Apps-1 {
            x2App[i].sctpOut[0] --> at.in++;
            x2App[i].sctpIn[0] <-- at.out++;
            x2App[i].sctpOut[1] --> at.in++;
            x2App[i].sctpIn[1] <-- at.out++;
            x2App[i].x2ManagerIn <-- cellularNic.x2$o++;
            x2App[i].x2ManagerOut --> cellularNic.x2$i++;
        }

		//# 
		//# Apps to transport layer connections
		//#
        for i=0..numApps-1 {
            app[i].socketOut --> at.in++;
            app[i].socketIn <-- at.out++;
        }

        at.out++ --> udp.appIn if hasUdp;
        at.in++ <-- udp.appOut if hasUdp;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;

        at.out++ --> sctp.appIn if hasSctp;
        at.in++ <-- sctp.appOut if hasSctp;

        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        sctp.ipOut --> tn.in++ if hasSctp;
        tn.out++ --> sctp.ipIn if hasSctp;

        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;

        at.out++ --> tn.in++;
        at.in++ <-- tn.out++;

        cellularNic.upperLayerIn <-- nl.out++;
        cellularNic.upperLayerOut --> nl.in++;

        //#
        //# MEC support	
        //#
        pppMEHostIf.upperLayerIn <-- nl.out++;
        pppMEHostIf.upperLayerOut --> nl.in++;
        pppMEHost <--> pppMEHostIf.phys;
}

