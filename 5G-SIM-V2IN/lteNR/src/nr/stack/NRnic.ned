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
// This file is part of 5G-Sim-V2I/N
//


package lteNR.nr.stack;

import lteNR.stack.*;
import inet.linklayer.contract.IWirelessInterface;
import inet.networklayer.common.InterfaceEntry;
import lteNR.stack.phy.LtePhy;
import lteNR.stack.mac.LteMac;
import lteNR.stack.pdcp_rrc.LtePdcpRrc;
import lteNR.stack.phy.feedback.LteDlFeedbackGenerator;
import lteNR.stack.rlc.LteRlc;
import lteNR.stack.ip2nic.INetworkLayer2CellularNic;
import lteNR.stack.ip2nic.IP2Nic;
import lteNR.x2.LteX2Manager;
import lteNR.stack.compManager.LteCompManager;
import lteNR.stack.d2dModeSelection.D2DModeSelection;
import lteNR.stack.handoverManager.LteHandoverManager;
import lteNR.stack.dualConnectivityManager.DualConnectivityManager;
import lteNR.stack.phy.ChannelModel.LteChannelModelInterface;

//5G-Sim-V2IN modules
import lteNR.nr.stack.nrip.IP2NR;
import lteNR.nr.stack.pdcp_rrc.NRPdcpRrc;
import lteNR.nr.stack.rlc.NRRlcUm;
import lteNR.nr.stack.mac.NRMacUE;
import lteNR.nr.stack.mac.NRMacGNB;
import lteNR.nr.stack.rlc.NRRlc;
import lteNR.nr.stack.sdap.layer.INRsdap;
import lteNR.nr.stack.sdap.utils.IQosHandler;
import lteNR.nr.stack.phy.ChannelModel.NRRealisticChannelModel;
import lteNR.nr.stack.phy.feedback.NRDlFeedbackGenerator;

//covers LteNicBase
module NRnicBase extends InterfaceEntry like ICellularNic
{
    parameters:

        @class(::inet::InterfaceEntry);
        @display("i=block/ifcard;bgb=616,478;bgl=3");
        string interfaceTableModule;
        string routingTableModule;
        string nodeType;
        double processingDelayIn @unit(s) = default(0s);
        double processingDelayOut @unit(s) = default(0s);
        bool d2dCapable;
        bool dualConnectivityEnabled = default(false);
        int numCarriers = default(1);
        string address = default("auto");

        string qosHandlerType;
        string NRsdapType;
        string NRPdcpRrcType;
        string NRMacType;
        string NRPhyType;
        string NRChannelModelType = default("NRRealisticChannelModel");

        bool isBS = default(false);

    gates:
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()
        input nrRadioIn @loose;// for NR support
        inout x2[] @loose;     // optional x2 manager

    submodules:

        qosHandler: <qosHandlerType> like IQosHandler {
            @display("p=62,65");
        }

        // bridge between radio nic and network layer
        ip2nic: <default("IP2NR")> like INetworkLayer2CellularNic {
            nodeType = nodeType;
            interfaceTableModule = interfaceTableModule;
            routingTableModule = routingTableModule;
            @display("p=420,65");
        }

        sdap: <NRsdapType> like INRsdap {
            @display("p=420,130");
        }

        // PDCP-RRC Layer
        pdcpRrc: <NRPdcpRrcType> like NRPdcpRrc {
            @display("p=420,199");
        }

        // RLC Layer
        rlc: NRRlc {
            @display("p=420,266");
            d2dCapable = d2dCapable;
            LteRlcUmType = default("NRRlcUm");
        }

        // MAC Layer
        mac: <NRMacType> like LteMac {
            interfaceTableModule = interfaceTableModule;
            @display("p=420,339");
        }

        // PHY Layer
        phy: <NRPhyType> like LtePhy {
            @display("p=420,416");
            @class("NRPhyType");
        }

        // NR Channel Model 
        nrChannelModel[numCarriers]: <NRChannelModelType> like LteChannelModelInterface {
            @display("p=220,65");
        }


    connections allowunconnected:
        ip2nic.stackNic <--> sdap.upperLayer;
        sdap.lowerLayer <--> pdcpRrc.DataPort;

        EUTRAN_RRC_Sap <--> pdcpRrc.EUTRAN_RRC_Sap;

        //# PDCP <-> RLC
        pdcpRrc.UM_Sap++ <--> rlc.UM_Sap;
        pdcpRrc.AM_Sap++ <--> rlc.AM_Sap;
        pdcpRrc.TM_Sap++ <--> rlc.TM_Sap;

        //# RLC <-> MAC
        rlc.RLC_to_MAC --> mac.RLC_to_MAC;
        rlc.MAC_to_RLC <-- mac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        mac.MAC_to_PHY --> phy.upperGateIn;
        mac.PHY_to_MAC <-- phy.upperGateOut;

        //# external: lower connection
        radioIn --> phy.radioIn;
        ip2nic.upperLayerOut --> {  delay = processingDelayOut; } --> upperLayerOut;
        upperLayerIn --> {  delay = processingDelayIn; } --> ip2nic.upperLayerIn;
}



module NRnicUE extends NRnicBase
{
    parameters:
        NRPhyType = default("NRPhyUE");
        NRMacType = default("NRMacUE");
        NRPdcpRrcType = default("NRPdcpRrcUE");
        NRsdapType = default("NRsdapUE");
        qosHandlerType = default("QosHandlerUE");

    submodules:
        dlFbGen: NRDlFeedbackGenerator {
            @display("p=62,130;is=s");
        }

}

module NRnicGNB extends NRnicBase
{
    parameters:
        NRPhyType = default("NRPhyGnb");
        NRMacType = default("NRMacGNB");
        NRPdcpRrcType = default("NRPdcpRrcGnb");
        d2dCapable = default(false);
        bool compEnabled = default(false);
        isBS = true;
        string LteCompManagerType = default("LteCompManagerProportional");

        qosHandlerType = default("QosHandlerGNB");
        NRsdapType = default("NRsdapGNB");

        nrChannelModel[*].isNodeB = true;

        @display("bgb=633.92,477.704");
    submodules:

        x2Manager: LteX2Manager {
            @display("p=143.764,294.32,row");
        }

        compManager: <LteCompManagerType> like LteCompManager if compEnabled {
            @display("p=143.764,408.652,row");
        }

        handoverManager: LteHandoverManager {
            @display("p=143.764,173.196,row");
        }

        dualConnectivityManager: DualConnectivityManager if dualConnectivityEnabled {
            @display("p=276.208,294.32,row");
        }

    connections:
        compManager.x2ManagerIn <-- x2Manager.dataPort$o++ if compEnabled;
        compManager.x2ManagerOut --> x2Manager.dataPort$i++ if compEnabled;
        handoverManager.x2ManagerIn <-- x2Manager.dataPort$o++;
        handoverManager.x2ManagerOut --> x2Manager.dataPort$i++;
        dualConnectivityManager.x2ManagerIn <-- x2Manager.dataPort$o++ if dualConnectivityEnabled;
        dualConnectivityManager.x2ManagerOut --> x2Manager.dataPort$i++ if dualConnectivityEnabled;

        if ( sizeof(x2) > 0 ) {
            //# connections between X2 Manager and GTP (for handover)
            x2$i[0] --> x2Manager.x2Gtp$i;
            x2$o[0] <-- x2Manager.x2Gtp$o;
        }
        //# connections between X2 Manager and the outer world
        for i=1..sizeof(x2)-1 {
            x2$i[i] --> x2Manager.x2$i++;
            x2$o[i] <-- x2Manager.x2$o++;
        }
}



