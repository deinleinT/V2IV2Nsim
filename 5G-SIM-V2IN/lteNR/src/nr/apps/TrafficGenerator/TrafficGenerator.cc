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

/*
 * Part of 5G-Sim-V2I/N
 *
 * UDP Application for generating constant packet flows
 *
 * see header file for comments
 *
 */

#include "nr/apps/TrafficGenerator/TrafficGenerator.h"

Define_Module(TrafficGenerator);
Define_Module(TrafficGeneratorServerUL);
Define_Module(TrafficGeneratorCarUL);
Define_Module(TrafficGeneratorServerDL);
Define_Module(TrafficGeneratorCarDL);

//used for use case cooperative lane merge
extern const std::string STATUS_UPDATE = "status-update";
extern const std::string REQUEST_TO_MERGE = "request-to-merge";
extern const std::string REQUEST_ACK = "request-ack";
extern const std::string SAFE_TO_MERGE_DENIAL = "safe-to-merge|denial";
//

TrafficGenerator::~TrafficGenerator() {

}

void TrafficGenerator::initialize(int stage) {
	ApplicationBase::initialize(stage);

	if (stage == INITSTAGE_LOCAL) {
		numberSentPackets = 0; //
		numberReceivedPackets = 0;
		WATCH(numberSentPackets);
		WATCH(numberReceivedPackets);

		nodeType = par("type").stdstringValue();
		localPort = par("localPort");
		destPort = par("destPort");
		packetName = par("packetName");
		autoReply = par("autoReply").boolValue();
		messageLength = par("messageLength").intValue();
		videoSize = par("videoSize").intValue();
		sendVideoPacket = true;
		dataSize = par("dataSize").intValue();
		sendDataPacket = true;

		delayVoip = registerSignal("delayVoip");
		delayVideo = registerSignal("delayVideo");
		delayV2X = registerSignal("delayV2X");
		delayData = registerSignal("delayData");

		delayVoipVariation = registerSignal("delayVoipVariation");
		delayVideoVariation = registerSignal("delayVideoVariation");
		delayV2XVariation = registerSignal("delayV2XVariation");
		delayDataVariation = registerSignal("delayDataVariation");
		delayVoipVariationReal = registerSignal("delayVoipVariationReal");
		delayVideoVariationReal = registerSignal("delayVideoVariationReal");
		delayV2XVariationReal = registerSignal("delayV2XVariationReal");
		delayDataVariationReal = registerSignal("delayDataVariationReal");

		v2vExchangeDelayReal = registerSignal("v2vExchangeDelayReal");

		lostPacketsV2X = 0;
		lostPacketsVideo = 0;
		lostPacketsVoip = 0;
		lostPacketsData = 0;

		WATCH(lostPacketsV2X);
		WATCH(lostPacketsVideo);
		WATCH(lostPacketsVoip);
		WATCH(lostPacketsData);

		//total received Packets
		recPacketsVideo = 0;
		recPacketsV2X = 0;
		recPacketsVoip = 0;
		recPacketsData = 0;

		WATCH(recPacketsVideo);
		WATCH(recPacketsV2X);
		WATCH(recPacketsVoip);
		WATCH(recPacketsData);

		//total sent Packets
		sentPacketsVideo = 0;
		sentPacketsV2X = 0;
		sentPacketsVoip = 0;
		sentPacketsData = 0;

		WATCH(sentPacketsVideo);
		WATCH(sentPacketsV2X);
		WATCH(sentPacketsVoip);
		WATCH(sentPacketsData);

		delayDataTransferFinished.setName("delayDataTransferFinished");

		messages = par("messages").intValue();

		delayBudget10ms = par("delayBudget10ms").doubleValue();
		delayBudget20ms = par("delayBudget20ms").doubleValue();
		delayBudget50ms = par("delayBudget50ms").doubleValue();
		delayBudget100ms = par("delayBudget100ms").doubleValue();
		delayBudget200ms = par("delayBudget200ms").doubleValue();
		delayBudget500ms = par("delayBudget500ms").doubleValue();
		delayBudget1s = par("delayBudget1s").doubleValue();

		considerDatasizeAndMessages = par("considerDatasizeAndMessages").boolValue();
		//

		lastSentStatusUpdateSN = 0;

		selfMsg = new cMessage("sendTimer");
		sendInterval = par("sendInterval").doubleValue();
		take(selfMsg);
		//for DL and UL, set in OmnetINI
		startTime = NOW + uniform(0, par("startTime").doubleValue());
		stopTime = SIMTIME_MAX;
		if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
			throw cRuntimeError("Invalid startTime/stopTime parameters");
	}
}

void TrafficGenerator::recordReliability() {

	ConnectionsMap tmpMap;

	if (nodeType == "car") {
		tmpMap = connectionsServToUE;
	} else {
		tmpMap = connectionsUEtoServ;
	}

	for (auto &var : tmpMap) {
		//record all different reliabilities

		double recPacketsData = var.second.statReport.recPacketsData;

		if (recPacketsData >= 0) {

			double recPacketsDataOutBudget10ms = var.second.statReport.recPacketsDataOutBudget10ms;
			double recPacketsDataOutBudget20ms = var.second.statReport.recPacketsDataOutBudget20ms;
			double recPacketsDataOutBudget50ms = var.second.statReport.recPacketsDataOutBudget50ms;
			double recPacketsDataOutBudget100ms = var.second.statReport.recPacketsDataOutBudget100ms;
			double recPacketsDataOutBudget200ms = var.second.statReport.recPacketsDataOutBudget200ms;
			double recPacketsDataOutBudget500ms = var.second.statReport.recPacketsDataOutBudget500ms;
			double recPacketsDataOutBudget1s = var.second.statReport.recPacketsDataOutBudget1s;
			double lostPacketsData = var.second.statReport.lostPacketsData;
			MacNodeId carNodeId = var.first;

			double reliabilityData10ms = 1.0 - double(recPacketsDataOutBudget10ms / recPacketsData);
			std::string nameString = "reliabilityData10msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData10ms);
			nameString = "TotalReliabilityData10msUL" + std::to_string(carNodeId);
			double TotalReliabilityData10ms = 1.0 - double((recPacketsDataOutBudget10ms + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData10ms);

			double reliabilityData20ms = 1.0 - double(recPacketsDataOutBudget20ms / recPacketsData);
			nameString = "reliabilityData20msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData20ms);
			nameString = "TotalReliabilityData20msUL" + std::to_string(carNodeId);
			double TotalReliabilityData20ms = 1.0 - double((recPacketsDataOutBudget20ms + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData20ms);

			double reliabilityData50ms = 1.0 - double(recPacketsDataOutBudget50ms / recPacketsData);
			nameString = "reliabilityData50msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData50ms);
			nameString = "TotalReliabilityData50msUL" + std::to_string(carNodeId);
			double TotalReliabilityData50ms = 1.0 - double((recPacketsDataOutBudget50ms + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData50ms);

			double reliabilityData100ms = 1.0 - double(recPacketsDataOutBudget100ms / recPacketsData);
			nameString = "reliabilityData100msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData100ms);
			nameString = "TotalReliabilityData100msUL" + std::to_string(carNodeId);
			double TotalReliabilityData100ms = 1.0 - double((recPacketsDataOutBudget100ms + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData100ms);

			double reliabilityData200ms = 1.0 - double(recPacketsDataOutBudget200ms / recPacketsData);
			nameString = "reliabilityData200msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData200ms);
			nameString = "TotalReliabilityData200msUL" + std::to_string(carNodeId);
			double TotalReliabilityData200ms = 1.0 - double((recPacketsDataOutBudget200ms + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData200ms);

			double reliabilityData500ms = 1.0 - double(recPacketsDataOutBudget500ms / recPacketsData);
			nameString = "reliabilityData500msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData500ms);
			nameString = "TotalReliabilityData500msUL" + std::to_string(carNodeId);
			double TotalReliabilityData500ms = 1.0 - double((recPacketsDataOutBudget500ms + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData500ms);

			double reliabilityData1s = 1.0 - double(recPacketsDataOutBudget1s / recPacketsData);
			nameString = "reliabilityData1sUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityData1s);
			nameString = "TotalReliabilityData1sUL" + std::to_string(carNodeId);
			double TotalReliabilityData1s = 1.0 - double((recPacketsDataOutBudget1s + lostPacketsData) / (lostPacketsData + recPacketsData));
			recordScalar(nameString.c_str(), TotalReliabilityData1s);

			nameString = "lostPacketsData" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), lostPacketsData);
			nameString = "recPacketsData" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), recPacketsData);
			recordScalar("numberCarsData", carsData.size());
		}

		//V2X
		double recPacketsV2X = var.second.statReport.recPacketsV2X;

		if (recPacketsV2X >= 0) {

			double recPacketsV2XOutBudget10ms = var.second.statReport.recPacketsV2XOutBudget10ms;
			double recPacketsV2XOutBudget20ms = var.second.statReport.recPacketsV2XOutBudget20ms;
			double recPacketsV2XOutBudget50ms = var.second.statReport.recPacketsV2XOutBudget50ms;
			double recPacketsV2XOutBudget100ms = var.second.statReport.recPacketsV2XOutBudget100ms;
			double recPacketsV2XOutBudget200ms = var.second.statReport.recPacketsV2XOutBudget200ms;
			double recPacketsV2XOutBudget500ms = var.second.statReport.recPacketsV2XOutBudget500ms;
			double recPacketsV2XOutBudget1s = var.second.statReport.recPacketsV2XOutBudget1s;
			double lostPacketsV2X = var.second.statReport.lostPacketsV2X;
			MacNodeId carNodeId = var.first;

			double reliabilityV2X10ms = 1.0 - double(recPacketsV2XOutBudget10ms / recPacketsV2X);
			std::string nameString = "reliabilityV2X10msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X10ms);
			nameString = "TotalReliabilityV2X10msUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X10ms = 1.0 - double((recPacketsV2XOutBudget10ms + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X10ms);

			double reliabilityV2X20ms = 1.0 - double(recPacketsV2XOutBudget20ms / recPacketsV2X);
			nameString = "reliabilityV2X20msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X20ms);
			nameString = "TotalReliabilityV2X20msUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X20ms = 1.0 - double((recPacketsV2XOutBudget20ms + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X20ms);

			double reliabilityV2X50ms = 1.0 - double(recPacketsV2XOutBudget50ms / recPacketsV2X);
			nameString = "reliabilityV2X50msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X50ms);
			nameString = "TotalReliabilityV2X50msUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X50ms = 1.0 - double((recPacketsV2XOutBudget50ms + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X50ms);

			double reliabilityV2X100ms = 1.0 - double(recPacketsV2XOutBudget100ms / recPacketsV2X);
			nameString = "reliabilityV2X100msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X100ms);
			nameString = "TotalReliabilityV2X100msUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X100ms = 1.0 - double((recPacketsV2XOutBudget100ms + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X100ms);

			double reliabilityV2X200ms = 1.0 - double(recPacketsV2XOutBudget200ms / recPacketsV2X);
			nameString = "reliabilityV2X200msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X200ms);
			nameString = "TotalReliabilityV2X200msUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X200ms = 1.0 - double((recPacketsV2XOutBudget200ms + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X200ms);

			double reliabilityV2X500ms = 1.0 - double(recPacketsV2XOutBudget500ms / recPacketsV2X);
			nameString = "reliabilityV2X500msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X500ms);
			nameString = "TotalReliabilityV2X500msUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X500ms = 1.0 - double((recPacketsV2XOutBudget500ms + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X500ms);

			double reliabilityV2X1s = 1.0 - double(recPacketsV2XOutBudget1s / recPacketsV2X);
			nameString = "reliabilityV2X1sUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityV2X1s);
			nameString = "TotalReliabilityV2X1sUL" + std::to_string(carNodeId);
			double TotalReliabilityV2X1s = 1.0 - double((recPacketsV2XOutBudget1s + lostPacketsV2X) / (lostPacketsV2X + recPacketsV2X));
			recordScalar(nameString.c_str(), TotalReliabilityV2X1s);

			nameString = "lostPacketsV2X" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), lostPacketsV2X);
			nameString = "recPacketsV2X" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), recPacketsV2X);
			recordScalar("numberCarsV2X", carsData.size());
		}

		//Voip
		double recPacketsVoip = var.second.statReport.recPacketsVoip;

		if (recPacketsVoip >= 0) {

			double recPacketsVoipOutBudget10ms = var.second.statReport.recPacketsVoipOutBudget10ms;
			double recPacketsVoipOutBudget20ms = var.second.statReport.recPacketsVoipOutBudget20ms;
			double recPacketsVoipOutBudget50ms = var.second.statReport.recPacketsVoipOutBudget50ms;
			double recPacketsVoipOutBudget100ms = var.second.statReport.recPacketsVoipOutBudget100ms;
			double recPacketsVoipOutBudget200ms = var.second.statReport.recPacketsVoipOutBudget200ms;
			double recPacketsVoipOutBudget500ms = var.second.statReport.recPacketsVoipOutBudget500ms;
			double recPacketsVoipOutBudget1s = var.second.statReport.recPacketsVoipOutBudget1s;
			double lostPacketsVoip = var.second.statReport.lostPacketsVoip;
			MacNodeId carNodeId = var.first;

			double reliabilityVoip10ms = 1.0 - double(recPacketsVoipOutBudget10ms / recPacketsVoip);
			std::string nameString = "reliabilityVoip10msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip10ms);
			nameString = "TotalReliabilityVoip10msUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip10ms = 1.0 - double((recPacketsVoipOutBudget10ms  + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip10ms);

			double reliabilityVoip20ms = 1.0 - double(recPacketsVoipOutBudget20ms / recPacketsVoip);
			nameString = "reliabilityVoip20msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip20ms);
			nameString = "TotalReliabilityVoip20msUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip20ms = 1.0 - double((recPacketsVoipOutBudget20ms + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip20ms);

			double reliabilityVoip50ms = 1.0 - double(recPacketsVoipOutBudget50ms / recPacketsVoip);
			nameString = "reliabilityVoip50msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip50ms);
			nameString = "TotalReliabilityVoip50msUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip50ms = 1.0 - double((recPacketsVoipOutBudget50ms + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip50ms);

			double reliabilityVoip100ms = 1.0 - double(recPacketsVoipOutBudget100ms / recPacketsVoip);
			nameString = "reliabilityVoip100msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip100ms);
			nameString = "TotalReliabilityVoip100msUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip100ms = 1.0 - double((recPacketsVoipOutBudget100ms + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip100ms);

			double reliabilityVoip200ms = 1.0 - double(recPacketsVoipOutBudget200ms / recPacketsVoip);
			nameString = "reliabilityVoip200msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip200ms);
			nameString = "TotalReliabilityVoip200msUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip200ms = 1.0 - double((recPacketsVoipOutBudget200ms + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip200ms);

			double reliabilityVoip500ms = 1.0 - double(recPacketsVoipOutBudget500ms / recPacketsVoip);
			nameString = "reliabilityVoip500msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip500ms);
			nameString = "TotalReliabilityVoip500msUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip500ms = 1.0 - double((recPacketsVoipOutBudget500ms + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip500ms);

			double reliabilityVoip1s = 1.0 - double(recPacketsVoipOutBudget1s / recPacketsVoip);
			nameString = "reliabilityVoip1sUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVoip1s);
			nameString = "TotalReliabilityVoip1sUL" + std::to_string(carNodeId);
			double TotalReliabilityVoip1s = 1.0 - double((recPacketsVoipOutBudget1s + lostPacketsVoip) / (lostPacketsVoip + recPacketsVoip));
			recordScalar(nameString.c_str(), TotalReliabilityVoip1s);

			nameString = "lostPacketsVoip" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), lostPacketsVoip);
			nameString = "recPacketsVoip" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), recPacketsVoip);

		}
		//

		//Video
		double recPacketsVideo = var.second.statReport.recPacketsVideo;

		if (recPacketsVideo >= 0) {

			double recPacketsVideoOutBudget10ms = var.second.statReport.recPacketsVideoOutBudget10ms;
			double recPacketsVideoOutBudget20ms = var.second.statReport.recPacketsVideoOutBudget20ms;
			double recPacketsVideoOutBudget50ms = var.second.statReport.recPacketsVideoOutBudget50ms;
			double recPacketsVideoOutBudget100ms = var.second.statReport.recPacketsVideoOutBudget100ms;
			double recPacketsVideoOutBudget200ms = var.second.statReport.recPacketsVideoOutBudget200ms;
			double recPacketsVideoOutBudget500ms = var.second.statReport.recPacketsVideoOutBudget500ms;
			double recPacketsVideoOutBudget1s = var.second.statReport.recPacketsVideoOutBudget1s;
			double lostPacketsVideo = var.second.statReport.lostPacketsVideo;
			MacNodeId carNodeId = var.first;

			double reliabilityVideo10ms = 1.0 - double(recPacketsVideoOutBudget10ms / recPacketsVideo);
			std::string nameString = "reliabilityVideo10msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo10ms);
			nameString = "TotalReliabilityVideo10msUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo10ms = 1.0 - double((recPacketsVideoOutBudget10ms + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo10ms);

			double reliabilityVideo20ms = 1.0 - double(recPacketsVideoOutBudget20ms / recPacketsVideo);
			nameString = "reliabilityVideo20msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo20ms);
			nameString = "TotalReliabilityVideo20msUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo20ms = 1.0 - double((recPacketsVideoOutBudget20ms + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo20ms);

			double reliabilityVideo50ms = 1.0 - double(recPacketsVideoOutBudget50ms / recPacketsVideo);
			nameString = "reliabilityVideo50msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo50ms);
			nameString = "TotalReliabilityVideo50msUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo50ms = 1.0 - double((recPacketsVideoOutBudget50ms + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo50ms);

			double reliabilityVideo100ms = 1.0 - double(recPacketsVideoOutBudget100ms / recPacketsVideo);
			nameString = "reliabilityVideo100msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo100ms);
			nameString = "TotalReliabilityVideo100msUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo100ms = 1.0 - double((recPacketsVideoOutBudget100ms + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo100ms);

			double reliabilityVideo200ms = 1.0 - double(recPacketsVideoOutBudget200ms / recPacketsVideo);
			nameString = "reliabilityVideo200msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo200ms);
			nameString = "TotalReliabilityVideo200msUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo200ms = 1.0 - double((recPacketsVideoOutBudget200ms + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo200ms);

			double reliabilityVideo500ms = 1.0 - double(recPacketsVideoOutBudget500ms / recPacketsVideo);
			nameString = "reliabilityVideo500msUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo500ms);
			nameString = "TotalReliabilityVideo500msUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo500ms = 1.0 - double((recPacketsVideoOutBudget500ms + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo500ms);

			double reliabilityVideo1s = 1.0 - double(recPacketsVideoOutBudget1s / recPacketsVideo);
			nameString = "reliabilityVideo1sUL" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), reliabilityVideo1s);
			nameString = "TotalReliabilityVideo1sUL" + std::to_string(carNodeId);
			double TotalReliabilityVideo1s = 1.0 - double((recPacketsVideoOutBudget1s + lostPacketsVideo) / (lostPacketsVideo + recPacketsVideo));
			recordScalar(nameString.c_str(), TotalReliabilityVideo1s);

			nameString = "lostPacketsVideo" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), lostPacketsVideo);
			nameString = "recPacketsVideo" + std::to_string(carNodeId);
			recordScalar(nameString.c_str(), recPacketsVideo);
		}

	}
}

//saving statistics
void TrafficGenerator::finish() {

	recordReliability();

	ApplicationBase::finish();
}

void TrafficGenerator::processStop() {
	socket.close();
}

void TrafficGenerator::handleMessageWhenUp(cMessage *msg) {
	if (msg->isSelfMessage()) {
		switch (msg->getKind()) {
		case START:
			processStart();
			break;

		case SEND:
			processSend();
			break;

		case STOP:
			processStop();
			break;

		default:
			throw cRuntimeError("Invalid kind %d in self message", (int) selfMsg->getKind());
		}
	} else if (msg->getKind() == UDP_I_DATA) {
		// process incoming packet
		processPacket(PK(msg));
	} else if (msg->getKind() == UDP_I_ERROR) {
		EV_WARN << "Ignoring UDP error report\n";
		delete msg;
	} else {
		delete msg;
	}
}

bool TrafficGenerator::handleNodeStart(IDoneCallback *doneCallback) {
	simtime_t start = std::max(startTime, simTime());
	if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
		selfMsg->setKind(START);
		if (!selfMsg->isScheduled())
			scheduleAt(start, selfMsg);
	}
	return true;
}

bool TrafficGenerator::handleNodeShutdown(IDoneCallback *doneCallback) {
	if (selfMsg)
		cancelEvent(selfMsg);

	return true;
}

void TrafficGenerator::handleNodeCrash() {
	if (selfMsg)
		cancelEvent(selfMsg);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Server UL --> TrafficGeneratorServerUL is used on a Server in UPLINK for receiving packets from cars and recording statistics
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TrafficGeneratorServerUL::handleNodeStart(IDoneCallback *doneCallback) {
	//do nothing
	socket.setOutputGate(gate("udpOut"));
	localAddress_ = L3AddressResolver().resolve(getParentModule()->getFullName());

	//const char *localAddress = par("localAddress");
	socket.bind(localAddress_, localPort);

	return true;
}

void TrafficGeneratorServerUL::handleMessageWhenUp(cMessage *msg) {

	processPacketServer(PK(msg));

	delete msg;
}

/**
 * calculates and records statistics from received packets from cars
 *
 * @param pk The packet received from Car
 */
void TrafficGeneratorServerUL::processPacketServer(cPacket *pk) {

	numberReceivedPackets++;
	//from UDPBasicApp
	numReceived++;
	//

	if (strcmp(pk->getName(), "V2X") == 0 || strcmp(pk->getName(), "status-update") == 0 || strcmp(pk->getName(), "request-to-merge") == 0 || strcmp(pk->getName(), "request-ack") == 0
			|| strcmp(pk->getName(), "safe-to-merge|denial") == 0) {

		if (strcmp(pk->getName(), "request-to-merge") == 0 || strcmp(pk->getName(), "request-ack") == 0 || strcmp(pk->getName(), "safe-to-merge|denial") == 0) {

			return;
		}

		V2XMessage *temp = check_and_cast<V2XMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		unsigned int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		carsV2X.insert(name);

		cModule *mod = getSimulation()->getModuleByPath(name);
		if (!mod) {
			delete temp;
			return;
		}

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//
			recPacketsV2X++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsUEtoServ[nodeId].statReport.recPacketsV2X++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget1s++;
			}
			//

			if ((connectionsUEtoServ[nodeId].statReport.lastV2X->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariationReal --> consecutive packets
				calcPVV2XUL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastV2X->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsV2X += lostPackets; //lost packets from this node
				lostPacketsV2X = connectionsUEtoServ[nodeId].statReport.lostPacketsV2X; //all over summary of lost packets

				calcPVV2XUL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, UL, lostPackets);
			}

			delete connectionsUEtoServ[nodeId].statReport.lastV2X;
			connectionsUEtoServ[nodeId].statReport.lastV2X = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayV2X, delay);

		} else {
			Connection tmp;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastV2X = temp->dup();
			tmp.statReport.recPacketsV2X++;
			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		delete temp;

	} else if (strcmp(pk->getName(), "Video") == 0) {

		VideoMessage *temp = check_and_cast<VideoMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//
			recPacketsVideo++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsUEtoServ[nodeId].statReport.recPacketsVideo++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget1s++;
			}
			//

			if ((connectionsUEtoServ[nodeId].statReport.lastVideo->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariation
				calcPVVideoUL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsVideo += lostPackets; //lost packets from this node
				lostPacketsVideo = connectionsUEtoServ[nodeId].statReport.lostPacketsVideo; //all over summary of lost packets

				calcPVVideoUL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, UL, lostPackets);
			}

			delete connectionsUEtoServ[nodeId].statReport.lastVideo;
			connectionsUEtoServ[nodeId].statReport.lastVideo = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayVideo, delay);

		} else {
			Connection tmp;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastVideo = temp->dup();
			tmp.statReport.recPacketsVideo++;
			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		delete temp;

	} else if (strcmp(pk->getName(), "VoIP") == 0) {

		VoIPMessage *temp = check_and_cast<VoIPMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//
			recPacketsVoip++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsUEtoServ[nodeId].statReport.recPacketsVoip++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget1s++;
			}
			//

			if ((connectionsUEtoServ[nodeId].statReport.lastVoIP->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariation
				calcPVVoipUL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastVoIP->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsVoip += lostPackets; //lost packets from this node
				lostPacketsVoip = connectionsUEtoServ[nodeId].statReport.lostPacketsVoip; //all over summary of lost packets

				calcPVVoipUL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, UL, lostPackets);
			}

			delete connectionsUEtoServ[nodeId].statReport.lastVoIP;
			connectionsUEtoServ[nodeId].statReport.lastVoIP = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayVoip, delay);

		} else {
			Connection tmp;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastVoIP = temp->dup();
			tmp.statReport.recPacketsVoip++;
			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		delete temp;
	} else if (strcmp(pk->getName(), "Data") == 0) {

		DataMessage *temp = check_and_cast<DataMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		carsData.insert(name);
		temp->setArrivalTime(NOW);
		int messageLen = temp->getByteLength();

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {

			recPacketsData++;

			connectionsUEtoServ[nodeId].statReport.recPacketsData++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget1s++;
			}

			//
			if (considerDatasizeAndMessages) {
				//HD Map
				if (connectionsUEtoServ[nodeId].timeFirstDataPacketArrived == 0) { //at least one full map packet arrived
					connectionsUEtoServ[nodeId].timeFirstDataPacketArrived = NOW;
				}
				if (connectionsUEtoServ[nodeId].dataBytesLeft <= 0 && connectionsUEtoServ[nodeId].messages <= 0) {
					delete temp;
					return;
				} else {
					connectionsUEtoServ[nodeId].dataBytesLeft = connectionsUEtoServ[nodeId].dataBytesLeft - messageLen;

					if (connectionsUEtoServ[nodeId].dataBytesLeft <= 0) {
						--connectionsUEtoServ[nodeId].messages;

						if (connectionsUEtoServ[nodeId].messages <= 0) {
							TrafficGeneratorCarUL *tmp1 = check_and_cast<TrafficGeneratorCarUL*>(getSimulation()->getModuleByPath(name)->getSubmodule("udpApp", 0));
							tmp1->setDataPacketFlag(false);
							connectionsUEtoServ[nodeId].timeLastDataPacketArrived = NOW;

							simtime_t delay = connectionsUEtoServ[nodeId].timeLastDataPacketArrived - connectionsUEtoServ[nodeId].timeFirstDataPacketArrived;
							delayDataTransferFinished.record(delay);
							connectionsUEtoServ[nodeId].dataBytesLeft = 0;

							delete temp;
							return;
						}
						//now finished
						connectionsUEtoServ[nodeId].timeLastDataPacketArrived = NOW;

						simtime_t delay = connectionsUEtoServ[nodeId].timeLastDataPacketArrived - connectionsUEtoServ[nodeId].timeFirstDataPacketArrived;
						delayDataTransferFinished.record(delay);
						connectionsUEtoServ[nodeId].dataBytesLeft = dataSize;

						connectionsUEtoServ[nodeId].timeFirstDataPacketArrived = 0;
					}
				}
			}
			//

			//already one entry
			//find the corresponding destination and check the next sequenceNumber
			if ((connectionsUEtoServ[nodeId].statReport.lastData->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariation
				calcPVDataUL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastData->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsData += lostPackets; //lost packets from this node
				lostPacketsData = connectionsUEtoServ[nodeId].statReport.lostPacketsData; //all over summary of lost packets

				calcPVDataUL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, UL, lostPackets);
			}

			delete connectionsUEtoServ[nodeId].statReport.lastData;
			connectionsUEtoServ[nodeId].statReport.lastData = temp->dup();

			assert(temp->getTimestamp() == temp->getCreationTime());
			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayData, delay);

		} else {
			Connection tmp;
			tmp.timeFirstDataPacketArrived = NOW;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = dataSize;
			tmp.statReport.lastData = temp->dup();
			tmp.statReport.recPacketsData++;
			tmp.messages = messages;

			if (considerDatasizeAndMessages) {

				tmp.dataSize = dataSize;

				tmp.dataBytesLeft = tmp.dataSize - messageLen;

			}

			connectionsUEtoServ[nodeId] = tmp;
		}

		delete temp;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////Car UPLINK
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrafficGeneratorCarUL::processStart() {

	localAddress_ = L3AddressResolver().resolve(getParentModule()->getFullName());

	socket.setOutputGate(gate("udpOut"));

	socket.bind(localAddress_, localPort);
	setSocketOptions();

	destAddress_ = L3AddressResolver().resolve(par("destAddresses").stringValue());

	if (strcmp(destAddress_.str().c_str(), "") != 0) {
		selfMsg->setKind(SEND);
		processSend();
	} else {
		if (stopTime >= SIMTIME_ZERO) {
			selfMsg->setKind(STOP);
			scheduleAt(stopTime, selfMsg);
		}
	}

}

//send packet from vehicle to server
void TrafficGeneratorCarUL::sendPacket(long bytes) {

	unsigned short nodeId = getNRBinder()->getMacNodeId(localAddress_.toIPv4());

	//do not send a packet if unconnected
	if(getSimulation()->getSystemModule()->par("useSINRThreshold").boolValue()){
		//get the binder an the ueNotConnectedList
		if(getBinder()->isNotConnected(nodeId)){
			return;
		}
	}
	//

	numberSentPackets++;
	//from UDPBasicApp
	numSent++;
	//

	if (strcmp(packetName, "V2X") == 0) {

		if (getSimulation()->getSystemModule()->par("remoteDrivingUL")) {
			//check nodeId --> every 10th car is a remote car
			if (!isRemoteCar(nodeId, getSystemModule()->par("remoteCarFactor").intValue())) {
				sendVideoPacket = false;
				return;
			}
		}

		sentPacketsV2X++;

		V2XMessage *payload = new V2XMessage(packetName);
		payload->setByteLength(messageLength);
		payload->setSenderNodeId(nodeId);
		payload->setSenderName(getParentModule()->getFullName());
		payload->setTimestamp(NOW);
		payload->setSequenceNumber(0);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//already one entry
			//find the corresponding destination

			payload->setSequenceNumber(connectionsUEtoServ[nodeId].statReport.lastV2X->getSequenceNumber() + 1);
			delete connectionsUEtoServ[nodeId].statReport.lastV2X;
			connectionsUEtoServ[nodeId].statReport.lastV2X = payload->dup();

		} else {
			//new connection
			Connection tmp;
			tmp.sendIpAddress = localAddress_;
			tmp.sendName = std::string(getParentModule()->getFullName());
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(tmp.sendName.c_str())->getId();
			tmp.macNodeIdSender = nodeId;

			tmp.destIpAddress = destAddress_;
			tmp.destName = par("destAddresses").stdstringValue();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;

			tmp.statReport.lastV2X = payload->dup();

			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		//exchange delay
		if (getSystemModule()->par("v2vCooperativeLaneMerge")) {
			payload->setName(STATUS_UPDATE.c_str());
			payload->setStartMeasuringExchangeDelay(simTime());
			payload->setSchedulingPriority(0);
			lastSentStatusUpdateSN = payload->getSequenceNumber();
		}

		socket.sendTo(payload, destAddress_, destPort);

		//send the request at the same time
		if (getSystemModule()->par("v2vCooperativeLaneMerge")) {
			V2XMessage *payloadReq = new V2XMessage("request-to-merge");
			payloadReq->setByteLength(messageLength);
			payloadReq->setSenderNodeId(nodeId);
			payloadReq->setSenderName(getParentModule()->getFullName());
			payloadReq->setTimestamp(simTime());
			payloadReq->setStartMeasuringExchangeDelay(simTime());
			payloadReq->setSchedulingPriority(1);
			socket.sendTo(payloadReq, destAddress_, destPort);
		}

	} else if (strcmp(packetName, "Video") == 0) {

		sentPacketsVideo++;

		VideoMessage *payload = new VideoMessage(packetName);
		payload->setByteLength(messageLength);
		payload->setSenderNodeId(nodeId);
		payload->setSenderName(getParentModule()->getFullName());
		payload->setTimestamp(NOW);
		payload->setSequenceNumber(0);


		/*
		 * realisticApproach
		 * every 10th car: interval 25ms, 15625byte
		 * even node Id: VoipStream, 80kbps, 100byte, 10ms
		 * odd node Id: random traffic, 10ms 500ms, 50 1500byte
		 */
		if (getSimulation()->getSystemModule()->par("realisticApproach")) {

			if (nodeId % 10 == 0) {
				//VideoStream with 5Mbps
				//packet size 15625 byte, packet interval 25ms
				payload->setByteLength(15625);
				sendInterval = 0.025;
			} else if (nodeId % 2 == 0) {
				//VoipStream with 80kpbs
				//packet size 100byte, packet interval 10ms
				payload->setByteLength(100);
				sendInterval = 0.01;
			} else {
				//random streams (packet size uniform(50byte,1500byte), packet interval uniform(10ms, 500ms)
				unsigned int bytes = uniform(50, 1500);
				payload->setByteLength(bytes);
				sendInterval = uniform(0.01, 0.500);
			}
		}
		//


		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//already one entry
			//find the corresponding destination

			payload->setSequenceNumber(connectionsUEtoServ[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
			delete connectionsUEtoServ[nodeId].statReport.lastVideo;
			connectionsUEtoServ[nodeId].statReport.lastVideo = payload->dup();


		} else {
			//new connection
			Connection tmp;
			tmp.sendIpAddress = localAddress_;
			tmp.sendName = std::string(getParentModule()->getFullName());
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(tmp.sendName.c_str())->getId();
			tmp.macNodeIdSender = nodeId;

			tmp.destIpAddress = destAddress_;
			tmp.destName = par("destAddresses").stdstringValue();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;

			tmp.statReport.lastVideo = payload->dup();

			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		//emit(sentPkSignal, payload);
		socket.sendTo(payload, destAddress_, destPort);

	} else if (strcmp(packetName, "VoIP") == 0) {

		sentPacketsVoip++;

		VoIPMessage *payload = new VoIPMessage(packetName);
		payload->setByteLength(messageLength);
		payload->setSenderNodeId(nodeId);
		payload->setSenderName(getParentModule()->getFullName());
		payload->setTimestamp(NOW);
		payload->setSequenceNumber(0);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//already one entry
			//find the corresponding destination

			payload->setSequenceNumber(connectionsUEtoServ[nodeId].statReport.lastVoIP->getSequenceNumber() + 1);
			delete connectionsUEtoServ[nodeId].statReport.lastVoIP;
			connectionsUEtoServ[nodeId].statReport.lastVoIP = payload->dup();

		} else {
			//new connection
			Connection tmp;
			tmp.sendIpAddress = localAddress_;
			tmp.sendName = std::string(getParentModule()->getFullName());
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(tmp.sendName.c_str())->getId();
			tmp.macNodeIdSender = nodeId;

			tmp.destIpAddress = destAddress_;
			tmp.destName = par("destAddresses").stdstringValue();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;

			tmp.statReport.lastVoIP = payload->dup();

			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		socket.sendTo(payload, destAddress_, destPort);

	} else if (strcmp(packetName, "Data") == 0) {

		if (getSimulation()->getSystemModule()->par("remoteDrivingUL")) {
			//check nodeId --> every 10th car is a remote car
			if (isRemoteCar(nodeId, getSystemModule()->par("remoteCarFactor"))) {
				sendDataPacket = false;
				return;
			}
		}

		sentPacketsData++;

		DataMessage *payload = new DataMessage(packetName);
		payload->setByteLength(messageLength);
		payload->setSenderNodeId(nodeId);
		payload->setSenderName(getParentModule()->getFullName());
		payload->setTimestamp(NOW);
		payload->setSequenceNumber(0);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//already one entry
			//find the corresponding destination

			payload->setSequenceNumber(connectionsUEtoServ[nodeId].statReport.lastData->getSequenceNumber() + 1);
			delete connectionsUEtoServ[nodeId].statReport.lastData;
			connectionsUEtoServ[nodeId].statReport.lastData = payload->dup();

		} else {
			//new connection
			Connection tmp;
			tmp.sendIpAddress = localAddress_;
			tmp.sendName = std::string(getParentModule()->getFullName());
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(tmp.sendName.c_str())->getId();
			tmp.macNodeIdSender = nodeId;

			tmp.destIpAddress = destAddress_;
			tmp.destName = par("destAddresses").stdstringValue();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = videoSize;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = dataSize - messageLength;

			tmp.statReport.lastData = payload->dup();

			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		socket.sendTo(payload, destAddress_, destPort);
	} else
		throw cRuntimeError("Unknown Application Type");

}

void TrafficGeneratorCarUL::processSend() {

	sendPacket(0);

	if (!sendVideoPacket)
		return;

	if (!sendDataPacket)
		return;

	//double sendInterval = par("sendInterval").doubleValue();
	double resendingDelay = par("resendingDelay").doubleValue();
	double uniForm = uniform(0.000, resendingDelay);
	simtime_t duration = simTime() + sendInterval + uniForm;

	if (stopTime < SIMTIME_ZERO || duration < stopTime) {
		selfMsg->setKind(SEND);
		scheduleAt(duration, selfMsg);
	} else {
		selfMsg->setKind(STOP);
		scheduleAt(stopTime, selfMsg);
	}
}

bool TrafficGeneratorCarUL::handleNodeStart(IDoneCallback *doneCallback) {
	//do nothing
	return TrafficGenerator::handleNodeStart(doneCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////Car Downlink
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrafficGeneratorCarDL::initialize(int stage) {
	ApplicationBase::initialize(stage);

	if (stage == INITSTAGE_LOCAL) {
		TrafficGenerator::initialize(stage);

	} else if (stage == INITSTAGE_LAST) {
		const char *carName = getParentModule()->getFullName();

		//for V2X Broadcast
		bool flag = getSystemModule()->par("v2vMulticastFlag").boolValue();
		if (flag)
			return;
		//

		TrafficGeneratorServerDL *tmp0 = check_and_cast<TrafficGeneratorServerDL*>(getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", 0));
		listener0 = new Listener(tmp0);
		carNameSignal = registerSignal("carName");
		subscribe(carNameSignal, listener0);

		if (getSimulation()->getSystemModule()->par("remoteDrivingUL").boolValue() && getSimulation()->getSystemModule()->par("remoteDrivingDL").boolValue()
				&& getAncestorPar("numUdpApps").intValue() == 4) {
			unsigned short tmpGate = 0;
			if (getAncestorPar("oneServer").boolValue())
				tmpGate = 1;
			TrafficGeneratorServerDL *tmp1 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGate));

			listener1 = new Listener(tmp1);
			subscribe(carNameSignal, listener1);

		} else if (getAncestorPar("numUdpApps").intValue() == 4) {

			unsigned short tmpGate = 0;
			unsigned short tmpGateTwo = 0;
			unsigned short tmpGateThree = 0;
			if (getAncestorPar("oneServer").boolValue())
				tmpGate = 1;
			TrafficGeneratorServerDL *tmp1 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGate));

			if (getAncestorPar("oneServer").boolValue())
				tmpGateTwo = 2;
			TrafficGeneratorServerDL *tmp2 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGateTwo));

			if (getAncestorPar("oneServer").boolValue())
				tmpGateThree = 3;
			TrafficGeneratorServerDL *tmp3 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGateThree));

			listener1 = new Listener(tmp1);
			listener2 = new Listener(tmp2);
			listener3 = new Listener(tmp3);
			subscribe(carNameSignal, listener1);
			subscribe(carNameSignal, listener2);
			subscribe(carNameSignal, listener3);
		} else if (getAncestorPar("numUdpApps").intValue() == 2) {
			unsigned short tmpGate = 0;

			if (getAncestorPar("oneServer").boolValue())
				tmpGate = 1;
			TrafficGeneratorServerDL *tmp1 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGate));

			listener1 = new Listener(tmp1);
			subscribe(carNameSignal, listener1);
		}
		emit(carNameSignal, carName);

		if (getSystemModule()->par("v2vCooperativeLaneMerge").boolValue()) {
			localAddress_ = L3AddressResolver().resolve(getParentModule()->getFullName());

			socket.setOutputGate(gate("udpOut"));

			socket.bind(localAddress_, localPort);
			setSocketOptions();
		}
	}
}

void TrafficGeneratorCarDL::handleMessageWhenUp(cMessage *msg) {

	processPacket(PK(msg));

	delete msg;

}

/**
 * DOWNLINK Car receives a UDP Packet from a server, records statistics
 * @param pk
 */
void TrafficGeneratorCarDL::processPacket(cPacket *pk) {

	numberReceivedPackets++;
	//from UDPBasicApp
	numReceived++;
	//

	if (strcmp(pk->getName(), "V2X") == 0 || strcmp(pk->getName(), "status-update") == 0 || strcmp(pk->getName(), "request-to-merge") == 0 || strcmp(pk->getName(), "request-ack") == 0
			|| strcmp(pk->getName(), "safe-to-merge|denial") == 0) {

		V2XMessage *temp = check_and_cast<V2XMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		unsigned int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();

		cModule *mod = getSimulation()->getModuleByPath(name);
		if (!mod) {
			delete temp;
			return;
		}

		const char *myName = getParentModule()->getFullName();

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
			//
			recPacketsV2X++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsServToUE[nodeId].statReport.recPacketsV2X++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget1s++;
			}
			//

			if ((connectionsServToUE[nodeId].statReport.lastV2X->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariation
				calcPVV2XDL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastV2X->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsV2X += lostPackets; //lost packets from this node
				lostPacketsV2X = connectionsServToUE[nodeId].statReport.lostPacketsV2X; //all over summary of lost packets

				calcPVV2XDL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				recordVehiclePositionAndLostPackets(nodeId, DL, lostPackets);

			}
			//

			delete connectionsServToUE[nodeId].statReport.lastV2X;
			connectionsServToUE[nodeId].statReport.lastV2X = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayV2X, delay);

		} else {
			Connection tmp;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = std::string(myName);
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastV2X = temp->dup();
			tmp.statReport.recPacketsV2X++;
			tmp.messages = messages;

			connectionsServToUE[nodeId] = tmp;
		}

		//for exchangeDelay
		if (getSystemModule()->par("v2vCooperativeLaneMerge").boolValue()) {
			if (strcmp(temp->getName(), STATUS_UPDATE.c_str()) == 0) { //get a status update, Receiver
				//
				connectionsServToUE[nodeId].lastReceivedStatusUpdateTime = temp->getStartMeasuringExchangeDelay();
				connectionsServToUE[nodeId].lastReceivedStatusUpdateSN = temp->getSequenceNumber();
				connectionsServToUE[nodeId].ackReceived = false;
				//

			} else if (strcmp(temp->getName(), REQUEST_TO_MERGE.c_str()) == 0) { // get a Reqeuest and send two Messages back, Receiver
//				if(connectionsServToUE[nodeId].lastReceivedStatusUpdateSN == temp->getSequenceNumber()){

				if (connectionsServToUE[nodeId].lastReceivedStatusUpdateTime != 0 && connectionsServToUE[nodeId].lastReceivedStatusUpdateTime != -1) {

					//send ack
					unsigned short localNodeId = getNRBinder()->getMacNodeId(L3AddressResolver().resolve(myName).toIPv4());
					V2XMessage *payload = new V2XMessage(REQUEST_ACK.c_str());
					payload->setByteLength(temp->getByteLength());
					payload->setSenderNodeId(localNodeId);
					payload->setSenderName(myName);
					payload->setStartMeasuringExchangeDelay(temp->getStartMeasuringExchangeDelay());
					payload->setSequenceNumber(temp->getSequenceNumber());
					payload->setSchedulingPriority(0);
					socket.sendTo(payload, L3AddressResolver().resolve(name), localPort);

					V2XMessage *payloadSafe = new V2XMessage(SAFE_TO_MERGE_DENIAL.c_str());
					payloadSafe->setByteLength(temp->getByteLength());
					payloadSafe->setSenderNodeId(localNodeId);
					payloadSafe->setSenderName(myName);
					payloadSafe->setStartMeasuringExchangeDelay(temp->getStartMeasuringExchangeDelay());
					payloadSafe->setSequenceNumber(temp->getSequenceNumber());
					payloadSafe->setSchedulingPriority(1);
					socket.sendTo(payloadSafe, L3AddressResolver().resolve(name), localPort);

					//RESET
					connectionsServToUE[nodeId].lastReceivedStatusUpdateTime = -1;
					connectionsServToUE[nodeId].ackReceived = false;

				} else {
					//lost a status update --> delete temp, reset
					connectionsServToUE[nodeId].lastReceivedStatusUpdateTime = -1;
					connectionsServToUE[nodeId].ackReceived = false;
					delete temp;
					return;
				}

			} else if (strcmp(temp->getName(), REQUEST_ACK.c_str()) == 0) { //the Sender gehts an ACK,

				connectionsServToUE[nodeId].ackReceived = true;

			} else if (strcmp(temp->getName(), SAFE_TO_MERGE_DENIAL.c_str()) == 0) {
				//calc the delay
				if (connectionsServToUE[nodeId].ackReceived) {
					//calc delay
					simtime_t delay = NOW - temp->getStartMeasuringExchangeDelay();
					emit(v2vExchangeDelayReal, delay);
				}

				connectionsServToUE[nodeId].ackReceived = false;
				delete temp;
				return;
			}
		}
		//end exchange delay

		delete temp;

	} else if (strcmp(pk->getName(), "Video") == 0) {

		VideoMessage *temp = check_and_cast<VideoMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
			//
			recPacketsVideo++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsServToUE[nodeId].statReport.recPacketsVideo++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget1s++;
			}
			//

			if ((connectionsServToUE[nodeId].statReport.lastVideo->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariation
				calcPVVideoDL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsVideo += lostPackets; //lost packets from this node
				lostPacketsVideo = connectionsServToUE[nodeId].statReport.lostPacketsVideo; //all over summary of lost packets

				calcPVVideoDL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, DL, lostPackets);

			}

			delete connectionsServToUE[nodeId].statReport.lastVideo;
			connectionsServToUE[nodeId].statReport.lastVideo = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayVideo, delay);

		} else {
			Connection tmp;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastVideo = temp->dup();
			tmp.statReport.recPacketsVideo++;

			tmp.messages = messages;

			connectionsServToUE[nodeId] = tmp;
		}

		delete temp;

	} else if (strcmp(pk->getName(), "VoIP") == 0) {

		VoIPMessage *temp = check_and_cast<VoIPMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
			//
			recPacketsVoip++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsServToUE[nodeId].statReport.recPacketsVoip++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget1s++;
			}
			//

			if ((connectionsServToUE[nodeId].statReport.lastVoIP->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariationReal
				calcPVVoipDL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastVoIP->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsVoip += lostPackets; //lost packets from this node
				lostPacketsVoip = connectionsServToUE[nodeId].statReport.lostPacketsVoip; //all over summary of lost packets
				//packetDelayVariation
				calcPVVoipDL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, DL, lostPackets);
			}

			delete connectionsServToUE[nodeId].statReport.lastVoIP;
			connectionsServToUE[nodeId].statReport.lastVoIP = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayVoip, delay);

		} else {
			Connection tmp;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastVoIP = temp->dup();
			tmp.statReport.recPacketsVoip++;

			tmp.messages = messages;

			connectionsServToUE[nodeId] = tmp;
		}

		delete temp;

	} else if (strcmp(pk->getName(), "Data") == 0) {

		DataMessage *temp = check_and_cast<DataMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char *name = temp->getSenderName();
		unsigned int messageLen = temp->getByteLength();
		temp->setArrivalTime(NOW);

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {

			//
			recPacketsData++;
			//
			connectionsServToUE[nodeId].statReport.recPacketsData++;

			//Reliability
			if (NOW - pk->getCreationTime() > delayBudget10ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget10ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget20ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget20ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget50ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget50ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget100ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget100ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget200ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget200ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget500ms) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget500ms++;
			}
			if (NOW - pk->getCreationTime() > delayBudget1s) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget1s++;
			}
			//

			if (considerDatasizeAndMessages) {
				//******************
				//HD Map
				if (connectionsServToUE[nodeId].timeFirstDataPacketArrived == 0) { //at least one full map packet arrived
					connectionsServToUE[nodeId].timeFirstDataPacketArrived = NOW;
				}
				if (connectionsServToUE[nodeId].dataBytesLeft <= 0 && connectionsServToUE[nodeId].messages <= 0) {
					delete temp;
					return;
				} else {
					connectionsServToUE[nodeId].dataBytesLeft = connectionsServToUE[nodeId].dataBytesLeft - messageLen;

					if (connectionsServToUE[nodeId].dataBytesLeft <= 0) {
						--connectionsServToUE[nodeId].messages;

						if (connectionsServToUE[nodeId].messages <= 0) {
							TrafficGeneratorServerDL *tmp1 = check_and_cast<TrafficGeneratorServerDL*>(getSimulation()->getModuleByPath(name)->getSubmodule("udpApp", 0));
							const char *ueName = this->getParentModule()->getFullName();
							tmp1->deleteNameFromQueuedNames(ueName);

							connectionsServToUE[nodeId].timeLastDataPacketArrived = NOW;

							simtime_t delay = connectionsServToUE[nodeId].timeLastDataPacketArrived - connectionsServToUE[nodeId].timeFirstDataPacketArrived;
							delayDataTransferFinished.record(delay);
							connectionsServToUE[nodeId].dataBytesLeft = dataSize;

							delete temp;
							return;
						}
						//now finished
						connectionsServToUE[nodeId].timeLastDataPacketArrived = NOW;

						simtime_t delay = connectionsServToUE[nodeId].timeLastDataPacketArrived - connectionsServToUE[nodeId].timeFirstDataPacketArrived;
						delayDataTransferFinished.record(delay);
						connectionsServToUE[nodeId].dataBytesLeft = dataSize;

						connectionsServToUE[nodeId].timeFirstDataPacketArrived = 0;
					}
				}
			}

			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			if ((connectionsServToUE[nodeId].statReport.lastData->getSequenceNumber() + 1) == number) {
				//received the next packet
				// packetDelayVariation
				calcPVDataDL(nodeId, temp, true);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastData->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsData += lostPackets; //lost packets from this node
				lostPacketsData = connectionsServToUE[nodeId].statReport.lostPacketsData; //all over summary of lost packets

				calcPVDataDL(nodeId, temp, false);

				//save number of lost packets for direction and each car
				// check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
				recordVehiclePositionAndLostPackets(nodeId, DL, lostPackets);
			}

			delete connectionsServToUE[nodeId].statReport.lastData;
			connectionsServToUE[nodeId].statReport.lastData = temp->dup();

			simtime_t delay = NOW - temp->getCreationTime();
			emit(delayData, delay);

		} else {
			Connection tmp;
			tmp.timeFirstDataPacketArrived = NOW;
			tmp.sendIpAddress = L3AddressResolver().resolve(name);
			tmp.sendName = name;
			tmp.sendPort = localPort;
			tmp.omnetIdSender = getSimulation()->getModuleByPath(name)->getId();
			tmp.macNodeIdSender = nodeId;
			tmp.destIpAddress = localAddress_;
			tmp.destName = getParentModule()->getFullName();
			tmp.destPort = destPort;
			tmp.omnetIdDest = getSimulation()->getModuleByPath(tmp.destName.c_str())->getId();
			tmp.macNodeIdDest = getBinder()->getMacNodeIdFromOmnetId(tmp.omnetIdDest);
			tmp.videoBytesLeft = 0;
			tmp.videoSize = videoSize;
			tmp.dataSize = dataSize;
			tmp.dataBytesLeft = 0;
			tmp.statReport.lastData = temp->dup();
			tmp.statReport.recPacketsData++;

			tmp.messages = messages;

			if (considerDatasizeAndMessages) {

				tmp.dataSize = dataSize;

				tmp.dataBytesLeft = tmp.dataSize - messageLen;

			}

			connectionsServToUE[nodeId] = tmp;
		}

		delete temp;
	}

}

bool TrafficGeneratorCarDL::handleNodeStart(IDoneCallback *doneCallback) {
	//do nothing
	socket.setOutputGate(gate("udpOut"));
	localAddress_ = L3AddressResolver().resolve(getParentModule()->getFullName());

	//const char *localAddress = par("localAddress");
	socket.bind(localAddress_, localPort);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DOWNLINK SERVER --> Sender of UDP Packets
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrafficGeneratorServerDL::initialize(int stage) {
	ApplicationBase::initialize(stage);

	if (stage == INITSTAGE_LOCAL) {
		TrafficGenerator::initialize(stage);

	}
}

void TrafficGeneratorServerDL::handleMessageWhenUp(cMessage *msg) {
	if (msg->isSelfMessage()) {

		switch (msg->getKind()) {
		case START:
			sendPacket(0);
			break;

		default:
			throw cRuntimeError("Invalid kind %d in self message", (int) selfMsg->getKind());
		}
	} else {
//        throw cRuntimeError("Unrecognized message (%s)%s", msg->getClassName(),
//                msg->getName());
		delete msg;
	}
}

/**
 * iterate over all car names and sends packets after the corresponding packet interval expired
 */
void TrafficGeneratorServerDL::sendPacket(long bytes) {

	if (names.size() == 0) {
		scheduleAt(NOW + par("sendInterval").doubleValue(), selfMsg);
		return;
	}

	double sendInterval = par("sendInterval").doubleValue();
	simtime_t nextSelfMsgTime = NOW + sendInterval + uniform(0, par("resendingDelay").doubleValue());

	std::set<std::string>::const_iterator itr = names.begin();
	while (itr != names.end()) {

		std::string carName = *itr;
		if (carsSendingTimes[carName] == NOW) {

			cModule *mod = getSimulation()->getModuleByPath(carName.c_str());
			if (!mod) {
				itr = names.erase(itr);
				carsSendingTimes.erase(carName);
				carsSendingIntervalRemoteDrivingDL.erase(carName);
				carsByteLengthRemoteDrivingDL.erase(carName);
				continue;
			}

			//
			if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
				//use the same random time
				carsSendingTimes[carName] = carsSendingIntervalRemoteDrivingDL[carName] + NOW;
			} else {
				carsSendingTimes[carName] = nextSelfMsgTime;
			}
			//

			int omnetId = mod->getId();
			int nodeId = getNRBinder()->getMacNodeIdFromOmnetId(omnetId);

			//do not send a packet if unconnected
			if(getSimulation()->getSystemModule()->par("useSINRThreshold").boolValue()){
				//get the binder an the ueNotConnectedList
				if(getBinder()->isNotConnected(nodeId)){
					continue;
				}
			}
			//

			numberSentPackets++;
			//from UDPBasicApp
			numSent++;
			//

			if (strcmp(packetName, "V2X") == 0) {

				if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
					//check nodeId --> every ...th car is a remote car
					if (!isRemoteCar(nodeId, getSystemModule()->par("remoteCarFactor").intValue())) {
						//sendVideoPacket = false;
						itr = names.erase(itr);
						carsSendingTimes.erase(carName);
						carsByteLengthRemoteDrivingDL.erase(carName);
						carsSendingIntervalRemoteDrivingDL.erase(carName);
						continue;
					}
				}

				sentPacketsV2X++;
				V2XMessage *payload = new V2XMessage(packetName);

				//
				if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
					payload->setByteLength(carsByteLengthRemoteDrivingDL[carName]);
				} else {
					payload->setByteLength(messageLength);
				}
				//

				payload->setSenderNodeId(nodeId);
				payload->setSenderName(getParentModule()->getFullName());
				payload->setTimestamp(NOW);
				payload->setSequenceNumber(0);

				if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
					//already one entry
					//find the corresponding destination

					payload->setSequenceNumber(connectionsServToUE[nodeId].statReport.lastV2X->getSequenceNumber() + 1);
					delete connectionsServToUE[nodeId].statReport.lastV2X;
					connectionsServToUE[nodeId].statReport.lastV2X = payload->dup();

				} else {
					//new connection
					Connection tmp;
					tmp.sendIpAddress = localAddress_;
					tmp.sendName = getParentModule()->getFullName();
					tmp.destIpAddress = L3AddressResolver().resolve(carName.c_str());
					tmp.destName = carName;
					tmp.videoBytesLeft = 0;
					tmp.videoSize = videoSize;
					tmp.dataSize = dataSize;
					tmp.dataBytesLeft = 0;

					tmp.messages = messages;

					tmp.statReport.lastV2X = payload->dup();

					connectionsServToUE[nodeId] = tmp;
				}

				socket.sendTo(payload, L3AddressResolver().resolve(carName.c_str()), destPort);
			} else if (strcmp(packetName, "Video") == 0) {
				sentPacketsVideo++;

				VideoMessage *payload = new VideoMessage(packetName);
				payload->setByteLength(messageLength);
				payload->setSenderNodeId(nodeId);
				payload->setSenderName(getParentModule()->getFullName());
				payload->setTimestamp(NOW);
				payload->setSequenceNumber(0);

				//
				if(getSimulation()->getSystemModule()->par("realisticApproach")){

					//10th car
					if(nodeId % 10 == 0){
						//VideoStream with 5Mbps
						//packet size 15625 byte, packet intervall 25ms
						payload->setByteLength(15625);
						sendInterval = 0.025;
					}else if(nodeId % 2 == 0){
						//VoipStream with 80kpbs
						//packet size 100byte, packet interval 10ms
						payload->setByteLength(100);
						sendInterval = 0.01;
					}else{
						//random streams (packet size uniform(50byte,1500byte), packet interval uniform(10ms, 500ms)
						unsigned int bytes = uniform(50,1500);
						payload->setByteLength(bytes);
						sendInterval = uniform(0.01,0.500);
					}
					//

					nextSelfMsgTime = NOW + sendInterval + uniform(0, par("resendingDelay").doubleValue());
					carsSendingTimes[carName] = nextSelfMsgTime;
				}
				//

				if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
					//already one entry
					//find the corresponding destination

					payload->setSequenceNumber(connectionsServToUE[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
					delete connectionsServToUE[nodeId].statReport.lastVideo;
					connectionsServToUE[nodeId].statReport.lastVideo = payload->dup();

					connectionsServToUE[nodeId].videoBytesLeft = connectionsServToUE[nodeId].videoBytesLeft - messageLength;

				} else {
					//new connection
					Connection tmp;
					tmp.sendIpAddress = localAddress_;
					tmp.sendName = getParentModule()->getFullName();
					tmp.destIpAddress = L3AddressResolver().resolve(carName.c_str());
					tmp.destName = carName;
					tmp.videoBytesLeft = 0;
					tmp.videoSize = videoSize;
					tmp.dataSize = dataSize;
					tmp.dataBytesLeft = 0;

					tmp.statReport.lastVideo = payload->dup();

					tmp.messages = messages;

					connectionsServToUE[nodeId] = tmp;
				}

				socket.sendTo(payload, L3AddressResolver().resolve(carName.c_str()), destPort);

			} else if (strcmp(packetName, "VoIP") == 0) {
				sentPacketsVoip++;
				VoIPMessage *payload = new VoIPMessage(packetName);
				payload->setByteLength(messageLength);
				payload->setSenderNodeId(nodeId);
				payload->setSenderName(getParentModule()->getFullName());
				payload->setTimestamp(NOW);
				payload->setSequenceNumber(0);

				if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
					//already one entry
					//find the corresponding destination

					payload->setSequenceNumber(connectionsServToUE[nodeId].statReport.lastVoIP->getSequenceNumber() + 1);
					delete connectionsServToUE[nodeId].statReport.lastVoIP;
					connectionsServToUE[nodeId].statReport.lastVoIP = payload->dup();

				} else {
					//new connection
					Connection tmp;
					tmp.sendIpAddress = localAddress_;
					tmp.sendName = getParentModule()->getFullName();
					tmp.destIpAddress = L3AddressResolver().resolve(carName.c_str());
					tmp.destName = carName;
					tmp.videoBytesLeft = 0;
					tmp.videoSize = videoSize;
					tmp.dataSize = dataSize;
					tmp.dataBytesLeft = 0;

					tmp.statReport.lastVoIP = payload->dup();

					tmp.messages = messages;

					connectionsServToUE[nodeId] = tmp;
				}

				socket.sendTo(payload, L3AddressResolver().resolve(carName.c_str()), destPort);
			} else if (strcmp(packetName, "Data") == 0) {

				if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
					//check nodeId --> every 10th car is a remote car
					if (isRemoteCar(nodeId, getSystemModule()->par("remoteCarFactor").intValue())) {
						//sendVideoPacket = false;
						itr = names.erase(itr);
						carsSendingTimes.erase(carName);
						carsByteLengthRemoteDrivingDL.erase(carName);
						carsSendingIntervalRemoteDrivingDL.erase(carName);
						continue;
					}
				}

				sentPacketsData++;

				DataMessage *payload = new DataMessage(packetName);

				//
				if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
					payload->setByteLength(carsByteLengthRemoteDrivingDL[carName]);
				} else {
					payload->setByteLength(messageLength);
				}
				//

				payload->setSenderNodeId(nodeId);
				payload->setSenderName(getParentModule()->getFullName());
				payload->setTimestamp(NOW);
				payload->setSequenceNumber(0);

				if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
					//already one entry
					//find the corresponding destination

					payload->setSequenceNumber(connectionsServToUE[nodeId].statReport.lastData->getSequenceNumber() + 1);
					delete connectionsServToUE[nodeId].statReport.lastData;
					connectionsServToUE[nodeId].statReport.lastData = payload->dup();

				} else {
					//new connection
					Connection tmp;

					tmp.sendIpAddress = localAddress_;
					tmp.sendName = getParentModule()->getFullName();
					tmp.destIpAddress = L3AddressResolver().resolve(carName.c_str());
					tmp.destName = carName;
					tmp.videoBytesLeft = 0;
					tmp.videoSize = videoSize;
					tmp.dataSize = dataSize;
					tmp.dataBytesLeft = 0;

					tmp.statReport.lastData = payload->dup();

					tmp.messages = messages;

					connectionsServToUE[nodeId] = tmp;
				}

				socket.sendTo(payload, L3AddressResolver().resolve(carName.c_str()), destPort);
			}

		}

		++itr;
	}

	selfMsg->setKind(START);
	for (auto &var : names) {
		if (carsSendingTimes[var] <= nextSelfMsgTime) {
			nextSelfMsgTime = carsSendingTimes[var];
		}
	}
	scheduleAt(nextSelfMsgTime, selfMsg);
}

bool TrafficGeneratorServerDL::handleNodeStart(IDoneCallback *doneCallback) {

	socket.setOutputGate(gate("udpOut"));
	localAddress_ = L3AddressResolver().resolve(getParentModule()->getFullName());

	socket.bind(localAddress_, localPort);

	return TrafficGenerator::handleNodeStart(doneCallback);
}
