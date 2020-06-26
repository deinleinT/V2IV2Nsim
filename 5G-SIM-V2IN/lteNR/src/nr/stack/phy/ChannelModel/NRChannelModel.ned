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

package nr.stack.phy.ChannelModel;

import lte.stack.phy.ChannelModel.LteRealisticChannelModel;


simple NRRealisticChannelModel extends LteRealisticChannelModel
{
    parameters:
    @class("NRRealisticChannelModel");

	string scenarioNR = default("RURAL_EMBB");
	string channelModelType = default("RMa_A");

    // Enable/disable shadowing        
    shadowing = default(false);
    // eNodeB height -->
    nodeb_height = default(35);
    // Building height -->
    building_height = default(5);
    street_wide = default(20);
    ue_height = default(1.5);
    // Carrier Frequency (GHz) -->
    carrierFrequency = default(4);
    // Antenna Gain of UE -->
    antennaGainUe = default(0);
    // Antenna Gain of eNodeB -->
    antennGainEnB = default(8);
    // Antenna Gain of Micro node -->
    antennGainMicro = default(5);
    // Thermal Noise for 10 MHz of Bandwidth -->
    thermalNoise = default(-104.5);
    // Ue noise figure -->
    ue_noise_figure = default(7);
    // eNodeB noise figure -->
    bs_noise_figure = default(5);
    // Cable Loss -->
    cable_loss = default(2);
   
    // Enable/disable fading -->  
    fading = default(false);
    // Fading type (JAKES or RAYGHLEY) -->  
    fading_type = default("JAKES");
    // If jakes fading this parameter specify the number of path (tap channel) -->  
    fading_paths = default(6);
    // if true, enables the inter-cell interference computation for DL connections from external cells -->  
    extCell_interference = default(false);
    // if true, enables the inter-cell interference computation for DL connections -->  
    downlink_interference = default(false);
    // if true, enables the interference computation for UL connections -->
    uplink_interference = default(false);
    
	tolerateMaxDistViolation = default(false);
	// If true enable the possibility to switch dinamically the LOS/NLOS pathloss computation -->
    dynamic_los = default(false);
    // If dynamic-los is false this parameter, if true, compute LOS pathloss otherwise compute NLOS pathloss -->
    fixed_los = default(false);
	// Target bler used to compute feedback -->
    targetBler = default(0.1);
    // HARQ reduction -->
    harqReduction = default(0.1);
    // Rank indicator tracefile -->
    lambdaMinTh = default(0.02);
    lambdaMaxTh = default(0.2);
    lambdaRatioTh = default(20);
    correlation_distance = default(50);
	
	dynamic_los = default(false);
	bool dynamicNlos = default(false);
	bool veinsObstacleShadowing = default(false);
	bool isNodeB;
	useTorus = default(false);
	
	//bool cqiFlag = default(false);

    // statistics
    @signal[rcvdSinr];
    @statistic[rcvdSinr](title="SINR measured at packet reception"; unit="dB"; source="rcvdSinr"; record=mean,vector);
}


