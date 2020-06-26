//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU), 
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

package nr.stack.phy;

import lte.stack.phy.LtePhy;


moduleinterface NRPhy extends LtePhy
{
    gates:
        inout upperRRC;
}

simple NRPhyBase like NRPhy
{
    parameters:
        @display("i=block/wrxtx");
        bool usePropagationDelay = default(true);                     // Should transmission delay be simulated?
        //xml channelModel;
        double ueTxPower = default(24);
        double eNodeBTxPower = default(44);
        double relayTxPower = default(20);
        double microTxPower = default(30);
        string txDirection = default("OMNI");
        double txAngle = default(0);
//        bool exchangeBuffersOnHandover = default(false);

        bool dynamicCellAssociation = default(true);

        // switch for handover messages handling on UEs
        bool enableHandover = default(true);
        double handoverLatency @unit(s) = default(0.05s);
        double handoverDelta @unit(s) = default(0.05s);
        
        bool enableMulticastD2DRangeCheck = default(false);
        double multicastD2DRange @unit(m) = default(1000m);

    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect
        inout upperRRC;
}

simple NRPhyUe extends NRPhyBase
{
    parameters:
        @class("NRPhyUe");

        @signal[servingCell];
        @statistic[servingCell](title="ID of the serving eNodeB for the UE"; unit=""; source="servingCell"; record=vector);

        //# CQI statistics
        @signal[averageCqiDl];
        @statistic[averageCqiDl](title="Average Cqi reported in DL"; unit="cqi"; source="averageCqiDl"; record=mean,vector);
        
        @signal[averageCqiUl];
        @statistic[averageCqiUl](title="Average Cqi reported in UL"; unit="cqi"; source="averageCqiUl"; record=mean,vector);

        @signal[averageTxPower];
        @statistic[averageTxPower](title="TxPower UE"; unit=""; source="averageTxPower"; record=vector);

        @signal[attenuation];
        @statistic[attenuation](title="attenuation UE"; unit=""; source="attenuation"; record=vector);

        @signal[snir];
        @statistic[snir](title="snir UE"; unit=""; source="snir"; record=vector);

        @signal[d3d];
        @statistic[d3d](title="d3d UE BS"; unit=""; source="d3d"; record=vector);

        @signal[d2d];
        @statistic[d2d](title="d2d UE BS"; unit=""; source="d2d"; record=vector);

        @signal[totalPer];
        @statistic[totalPer](title="Total Error Probability"; unit=""; source="totalPer"; record=vector);

        @signal[bler];
        @statistic[bler](title="Bler"; unit=""; source="bler"; record=vector);

        @signal[speed];
        @statistic[speed](title="Speed"; unit=""; source="speed"; record=vector);
        
        //channelModel.isNodeB = false;
        
}

simple NRPhyGnb extends NRPhyBase
{
    parameters:
        @class("NRPhyGnb");
        //xml feedbackComputation;

        @signal[averageTxPower];
        @statistic[averageTxPower](title="TxPower GNB"; unit=""; source="averageTxPower"; record=vector);

        @signal[attenuation];
        @statistic[attenuation](title="attenuation GNB"; unit=""; source="attenuation"; record=vector);

        @signal[snir];
        @statistic[snir](title="snir GNB"; unit=""; source="snir"; record=vector);

        @signal[d3d];
        @statistic[d3d](title="d3d UE BS"; unit=""; source="d3d"; record=vector);

        @signal[d2d];
        @statistic[d2d](title="d2d UE BS"; unit=""; source="d2d"; record=vector);

        @signal[totalPer];
        @statistic[totalPer](title="Total Error Probability"; unit=""; source="totalPer"; record=vector);

        @signal[bler];
        @statistic[bler](title="Bler"; unit=""; source="bler"; record=vector);

        @signal[speed];
        @statistic[speed](title="Speed"; unit=""; source="speed"; record=vector);
        
        double targetBler = default(0.1);
        double lambdaMinTh = default(0.02);
        double lambdaMaxTh = default(0.2);
        double lambdaRatioTh = default(20);
        bool cqiFlag = default(false);
        
}

