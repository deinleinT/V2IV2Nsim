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

package lteNR.nr.stack.phy;

import lteNR.stack.phy.NRPhyUe;
import lteNR.stack.phy.LtePhyEnb;

simple NRPhyUE extends NRPhyUe
{
    parameters:
        double handoverDelta @unit(s) = default(0.001s);
        double dasRssiThreshold = default(0.00001);
        
        @class("NRPhyUE");

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
        
}

simple NRPhyGnb extends LtePhyEnb
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
        
}

