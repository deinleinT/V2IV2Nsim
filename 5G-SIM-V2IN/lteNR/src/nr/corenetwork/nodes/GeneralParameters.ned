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

//used for parameters, that can be set in omnetpp.ini file
//
//
package nr.corenetwork.nodes;

network GeneralParameters{
    parameters:
        bool v2vMulticastFlag = default(false);
        double v2vMulticastDistance = default(300.0);
        bool v2vCooperativeLaneMerge = default(false);
        string initialTxMode = default("OL_SPATIAL_MULTIPLEXING"); 
        bool remoteDrivingUL = default(false); //use case remote driving in UL activated
        bool remoteDrivingDL = default(false); //not implemented so far
        int remoteCarFactor = default(10); // controls the remote car penetration rate (e.g., set to 10 --> each 10th car is a remote vehicle)
        bool remoteCarJustOne = default(false); // if true, just one car is a remote vehicle --> the car identity can set via the parameter remoteCarFactor, e.g., this flag is true and remoteCarFactor = 10 --> the car with MacNodeId 1035 (or car[9]) is a remoteVehicle
        
        bool nrHarq = default(false); //if true, the NR HarqProcess-procedure is activeted (shorter intervals for scheduling grant and HARQ ACK/NACK)
        int numberHarqProcesses = default(8); //Number of LTE HarqProcesses
        int numberHarqProcessesNR = default(16); //Number of NR HarqProcesses
        int harqFBEvaluationIntervalLTE = default(3); //time of ms in LTE, until ACK/NACK is evaluated
        int harqFBEvaluationIntervalNR = default(1);// time of ms in NR, until ACK/NACK is evaluated
        int raRespWinStartNR = default(1); //time for scheduling grant
        bool fairRacScheduleInUL = default(false); //used in NRSchedulerGnbUl::racschedule (by default set to false)
        bool newTxbeforeRtx = default(true); //used in NRSchedulerGnbUl and NRMacUe (if true, a new transmission is scheduled before a retransmission, by default set to true)
}