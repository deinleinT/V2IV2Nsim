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

//used for parameters, which can be set in omnetpp.ini file
//
//
package nr.corenetwork.nodes;

network GeneralParameters{
    parameters:
        bool v2vMulticastFlag = default(false);
        double v2vMulticastDistance = default(300.0);
        
        bool v2vCooperativeLaneMerge = default(false);
		//TRANSMIT_DIVERSITY        OL_SPATIAL_MULTIPLEXING,MULTI_USER,SINGLE_ANTENNA_PORT0
        string initialTxMode = default("SINGLE_ANTENNA_PORT0"); 
        bool remoteDrivingUL = default(false); //use case remote driving in UL activated
        bool remoteDrivingDL = default(false); //use case remote driving in UL activated
        int remoteCarFactor = default(10); // controls the remote car penetration rate (e.g., set to 10 --> each 10th car is a remote vehicle)
        bool remoteCarJustOne = default(false); // if true, just one car is a remote vehicle --> the car identity can set via the parameter remoteCarFactor, e.g., this flag is true and remoteCarFactor = 10 --> the car with MacNodeId 1035 (or car[9]) is a remoteVehicle
        bool remoteCarByColour = default(false);// if true, only red cars are remote vehicles
        
        int mtu @unit("B") = default(40000B);
        
        bool nrHarq = default(true); //if true, the NR HarqProcess-procedure is activeted (shorter intervals for scheduling grant and HARQ ACK/NACK)
        int numberHarqProcesses = default(8); //Number of LTE HarqProcesses
        int numberHarqProcessesNR = default(16); //Number of NR HarqProcesses
        int harqFBEvaluationIntervalLTE = default(3); //time of ms in LTE, until ACK/NACK is evaluated
        int harqFBEvaluationIntervalNR = default(1);// time of ms in NR, until ACK/NACK is evaluated
        int raRespWinStartNR = default(1); //time for scheduling grant
        bool fairRacScheduleInUL = default(false); //used in NRSchedulerGnbUl::racschedule (by default set to false)
        bool newTxbeforeRtx = default(true); //used in NRSchedulerGnbUl and NRMacUe (if true, a new transmission is scheduled before a retransmission, by default set to true)
        
        //2, 4, 6 or 8 codeblockgroups are allowed
        bool useCodeBlockGroups = default(true);
        int numberOfCodeBlockGroups = default(8);
        
        //
        bool useSINRThreshold = default(true); //checks if the SINR of a UE is lower than SINRThreshold in UL
        string thresholdDirection = default("DL");
        int SINRThreshold = default(5); // if useSINRThreshold true, the SINR in UL is used to check, whether a UE is in coverage or not (during Handover), if the SINR to all Base stations is lower than this value, the UE is not connected to any base station
        double checkConnectionInterval @unit(s) = default(0.1s);
        **.dasRssiThreshold = SINRThreshold; 
        
        //
        //if true, the VideoStream Confic in the ini file is modified --> every 10th vehicle sends or receives a videostream with 5Mbps (packet size 15625 byte, packet intervall 25ms)
        //one half of all other vehicles send or receive a voip stream with 80kbps (packet size 100byte), packet interval 10ms
        //the other half send or receive random streams (packet size uniform(50byte,1500byte), packet interval uniform(10ms, 500ms)
        bool realisticApproach = default(true); 
        
        //Records the vehicle position in downlink and uplink when a packet loss at application layer was detected
        bool recordPositionAndPacketLoss = default(false);
        
}
