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
// This file is part of 5G-Sim-V2I/N
//


package nr.stack;

import nr.stack.phy.*;
import lte.stack.ILteNic;

import nr.stack.phy.feedback.NRDlFeedbackGenerator;

import lte.x2.LteX2Manager;
import lte.stack.compManager.LteCompManager;
import lte.stack.handoverManager.LteHandoverManager;
import lte.stack.mac.LteMac;
import lte.stack.d2dModeSelection.D2DModeSelection;

//NR:
import nr.corenetwork.nrip.IP2NR;
import nr.stack.pdcp_rrc.NRPdcpRrc;
import nr.stack.rlc.NRRlcUm;
import nr.stack.mac.NRMacUe;
import nr.stack.mac.NRMacGnb;
import nr.stack.rlc.NRRlc;
import nr.stack.sdap.layer.INRsdap;
import nr.stack.rrc.Rrc;
import nr.stack.sdap.utils.*;
import nr.stack.phy.ChannelModel.NRRealisticChannelModel;

moduleinterface INRNic extends ILteNic
{
}

module NRNicBase like INRNic
{
    parameters:
        @display("i=block/ifcard;bgb=288.66,562.604;bgl=3");
        string interfaceTableModule;
        bool isBS = default(false);
        bool dynamicNLOS = default(false); 
        bool veinsObstacleShadowing = default(false);
        string routingTableModule;
        string NRPdcpRrcType;
        string LteMacType;
        //        string NRMacType;
        string NRPhyType;
        bool d2dCapable;            // inherit the value from the parent module
        string nodeType;

        string qosHandlerType;
        string NRsdapType;

    gates:
        //#
        //# Gates connecting UE/eNB and LTE Stack
        //#
        //# Control Ports
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()
        inout x2[] @loose;     // optional x2 manager

        //input radioInRRC @loose;

    submodules:

        qosHandler: <qosHandlerType> like IQosHandler {

            @display("p=227.532,45.28");
        }

        rrc: Rrc {
            @display("p=216.212,169.8");
        }

        // bridge between lte stack and network layer
        //ip2lte: IP2lte {
        ip2lte: IP2NR {
            nodeType = nodeType;
            interfaceTableModule = interfaceTableModule;
            routingTableModule = routingTableModule;
            @display("p=150,90");
        }

        sdap: <NRsdapType> like INRsdap {
            @display("p=150,170");
        }

        // PDCP-RRC Layer
        //pdcpRrc: <LtePdcpRrcType> like LtePdcpRrc {
        pdcpRrc: <NRPdcpRrcType> like NRPdcpRrc {
            @display("p=150,250");
        }

        // RLC Layer
        //rlc: LteRlc {
        rlc: NRRlc {
            @display("p=150,330");
            d2dCapable = d2dCapable;
            LteRlcUmType = default("NRRlcUm");
        }
        // MAC Layer
        //mac: <LteMacType> like LteMac {
        mac: <LteMacType> like LteMac {
            interfaceTableModule = interfaceTableModule;
            @display("p=150,410");
        }

        // LtePhy submodule
        phy: <NRPhyType> like NRPhy {
            @display("p=150,490");
            @class("NRPhyType");
        }
        
        channelModel: NRRealisticChannelModel {
            @display("p=44.8,389.75998");
            isNodeB = isBS;
        }

    connections allowunconnected:

        ip2lte.stackLte <--> sdap.upperLayer;
        sdap.lowerLayer <--> pdcpRrc.DataPort;

        //RRC Handling
        //RRC -- PDCP
        rrc.gateRRC <--> pdcpRrc.upperLayerRRC;

        //PDCP -- RLC
        pdcpRrc.lowerLayerRRC <--> rlc.upperLayer;

        //RLC -- MAC
        rlc.lowerLayer <--> mac.upperLayer;

        //MAC -- PHY
        mac.lowerLayer <--> phy.upperRRC;

        //#
        //# Internal LTE Stack Connections 
        //# 

        EUTRAN_RRC_Sap <--> pdcpRrc.EUTRAN_RRC_Sap;

        //# PDCP <-> RLC
        pdcpRrc.UM_Sap <--> rlc.UM_Sap;
        pdcpRrc.AM_Sap <--> rlc.AM_Sap;
        pdcpRrc.TM_Sap <--> rlc.TM_Sap;

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
        ip2lte.upperLayerOut --> upperLayerOut;
        upperLayerIn --> ip2lte.upperLayerIn;

}

module NRNicUe extends NRNicBase
{
    parameters:
        NRPdcpRrcType = "NRPdcpRrcUe";
        LteMacType = default("NRMacUe");
        NRPhyType = "NRPhyUe";
        rrc.nodeType = "UE";
        qosHandlerType = "QosHandlerUE";
        NRsdapType = "NRsdapUE";
        

    submodules:
        dlFbGen: NRDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }
        
}

module NRNicGnb extends NRNicBase
{
    parameters:
        NRPdcpRrcType = "NRPdcpRrcGnb";
        LteMacType = default("NRMacGnb");
        NRPhyType = "NRPhyGnb";
        bool compEnabled = default(false);
        isBS = true;
        string LteCompManagerType = default("LteCompManagerBase");
        rrc.nodeType = "GNODEB";
        qosHandlerType = "QosHandlerGNB";
        NRsdapType = "NRsdapGNB";

    submodules:
        //#
        //# X2 manager
        //#       
        x2Manager: LteX2Manager {
            @display("p=60,227,row");
        }

        //#
        //# Modules exploiting X2 interface
        //#
        compManager: <LteCompManagerType> like LteCompManager if compEnabled {
            @display("p=60,315,row");
        }
        handoverManager: LteHandoverManager {
            @display("p=60,142,row");
        }

    connections:
        //# connections between X2 Manager and its users
        compManager.x2ManagerIn <-- x2Manager.dataPort$o++ if compEnabled;
        compManager.x2ManagerOut --> x2Manager.dataPort$i++ if compEnabled;
        handoverManager.x2ManagerIn <-- x2Manager.dataPort$o++;
        handoverManager.x2ManagerOut --> x2Manager.dataPort$i++;

        //# connections between X2 Manager and GTP (for handover)
        x2$i[0] --> x2Manager.x2Gtp$i;
        x2$o[0] <-- x2Manager.x2Gtp$o;

        //# connections between X2 Manager and the outer world
        for i=1..sizeof(x2)-1 {
            x2$i[i] --> x2Manager.x2$i++;
            x2$o[i] <-- x2Manager.x2$o++;
        }

}



