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
package lteNR.nr.common.nodes;

network GeneralParameters
{
    parameters:
        //for V2VCoopPerception
        bool v2vMulticastFlag = default(false);
        double v2vMulticastDistance = default(300.0);
        string initialTxMode = default("SINGLE_ANTENNA_PORT0"); 
        bool remoteDrivingUL = default(false); //use case remote driving in UL activated
        bool remoteDrivingDL = default(false); //use case remote driving in DL activated
        int remoteCarFactor = default(10); // controls the remote car penetration rate (e.g., set to 10 --> each 10th car is a remote vehicle)
        bool remoteCarJustOne = default(false); // if true, just one car is a remote vehicle --> the car identity can set via the parameter remoteCarFactor, e.g., this flag is true and remoteCarFactor = 10 --> the car with MacNodeId 1035 (or car[9]) is a remoteVehicle
        bool remoteCarByColour = default(false);// if true, only red cars are remote vehicles

        int mtu @unit("B") = default(40000B);

        bool nrHarq = default(true); //if true, the NR HarqProcess-procedure is activated (shorter intervals for scheduling grant and HARQ ACK/NACK)
        int numberHarqProcesses = default(8); //Number of LTE HarqProcesses
        int numberHarqProcessesNR = default(16); //Number of NR HarqProcesses
        int harqFBEvaluationIntervalLTE = default(3); //time of ms in LTE, until ACK/NACK is evaluated
        int harqFBEvaluationIntervalNR = default(1);// time of ms in NR, until ACK/NACK is evaluated
        int raRespWinStartNR = default(1); //time for scheduling grant

        //2, 4, 6 or 8 codeblockgroups are allowed
        bool useCodeBlockGroups = default(false);
        int numberOfCodeBlockGroups = default(8);

        //checks the connection
        bool useSINRThreshold = default(false); //checks if the SINR of a UE is lower than SINRThreshold in UL
        string thresholdDirection = default("DL");
        int SINRThreshold = default(-10); // if useSINRThreshold true, the SINR is used to check, whether a UE is in coverage or not, if the SINR to all Base stations is lower than this value, the UE is not connected to any base station
        double checkConnectionInterval @unit(s) = default(0.5s);

        //Application layer --> Traffic Generator
        //leads to a more realistic behavior: for each vehicle it is randomly evaluated whether a vehicle runs an application or not
        //thus, some vehicles will not transmit or receive data
        bool realisticApproachMultiApplication = default(false);
	//Heatmap
        //Records the vehicle position in downlink and uplink when a packet loss at application layer was detected
        bool recordPositionAndPacketLoss = default(false);
        //

        //if true, the NR QosModel is used for scheduling
        //first approach uses the priority level of the qos characteristics
        bool useQosModel = default(true);
        bool packetDropEnabled = default(true);
        bool combineQosWithRac = default(true);//should always set to true
        double lambdaPriority = default(1);
        double lambdaRemainDelayBudget = default(0);
        double lambdaCqi = default(0);
        double lambdaRtx = default(0);
        double lambdaByteSize = default(0);

        //the smallest priority value gets the highest priority and is scheduled first
        //priority --> use the priority values from the qos characteristics from 23.501
        //pdb --> use the pdb values from the qos characteristics
        //should be set to "default" by default
        string qosModelPriority = default("default");

        //if true, a uniform random processing delay is considered in downlink and uplink
        //in the modules NRsdapUE, NRsdapGNB, NRPdcpRrcUe, NRPdcpRrcGnb, TrafficFlowFilterNR, GtpUserNR
        //the delay is calculated by this formula: uniform(0, totalPacketSizeInBytes / 10e5)
        bool considerProcessingDelay = default(false);

        //if true, blerCurves from simu5G are used by default
        bool blerCurvesNR = default(true);
        //TU --> range from SINR -14.5 - 38
        //AWGN --> range from SINR -14.5 - 22.5
        string blerVersion = default("AWGN");

        //for testing
        bool printTBS = default(false);

        //extendedMCSTable
        bool useExtendedMcsTable = default(true);

        bool fairRacScheduleInUL = default(true); //used in NRSchedulerGnbUl::racschedule (by default set to true)
        bool newTxbeforeRtx = default(false); //used in NRSchedulerGnbUl and NRMacUe (if true, a new transmission is scheduled before a retransmission, by default set to false)

    	//heat map physical layer
    	//max SNR value in DL and UL
    	double maxDL = default(40);
    	double maxUL = default(20);
    	//min SNR value in DL and UL
    	double minDL = default(-10);
    	double minUL = default(-10);
    	
    	//if false, consider queuesize during scheduling
    	//if true, only the single packetsize is considered
    	//default false
    	bool useSinglePacketSizeDuringScheduling = default(false); 
    	
    	//simplified Flow Control
    	//binder --> ueId --> in DL check macBuffer on base station, in UL check macBuffer on UE
    	bool useSimplifiedFlowControl = default(true);
   	
    	//throughput measuring interval for vehicles
    	double throughputInterval = default(1);
}
