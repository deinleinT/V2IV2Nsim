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

		v2xDelayBudget = par("v2xDelayBudget").doubleValue();
		voipDelayBudget = par("voipDelayBudget").doubleValue();
		videoDelayBudget = par("videoDelayBudget").doubleValue();
		dataDelayBudget = par("dataDelayBudget").doubleValue();

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

		delayVoipVariationOut = 0;
		delayV2XVariationOut = 0;
		delayVideoVariationOut = 0;
		delayDataVariationOut = 0;

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

		//packets that arrived NOT in the delayBudget
		recPacketsVideoOutBudget = 0;
		recPacketsV2XOutBudget = 0;
		recPacketsVoipOutBudget = 0;
		recPacketsDataOutBudget = 0;

		WATCH(recPacketsVideoOutBudget);
		WATCH(recPacketsV2XOutBudget);
		WATCH(recPacketsVoipOutBudget);
		WATCH(recPacketsDataOutBudget);

		//
		delayDataTransferFinished.setName("delayDataTransferFinished");

		messages = par("messages").intValue();

		delayBudget10ms = par("delayBudget10ms").doubleValue();
		delayBudget20ms = par("delayBudget20ms").doubleValue();
		delayBudget50ms = par("delayBudget50ms").doubleValue();
		delayBudget100ms = par("delayBudget100ms").doubleValue();
		delayBudget200ms = par("delayBudget200ms").doubleValue();
		delayBudget500ms = par("delayBudget500ms").doubleValue();
		delayBudget1s = par("delayBudget1s").doubleValue();

		//Data
		reliabilityData10msVecUL.setName("reliabilityData10msVecUL");
		reliabilityData20msVecUL.setName("reliabilityData20msVecUL");
		reliabilityData50msVecUL.setName("reliabilityData50msVecUL");
		reliabilityData100msVecUL.setName("reliabilityData100msVecUL");
		reliabilityData200msVecUL.setName("reliabilityData200msVecUL");
		reliabilityData500msVecUL.setName("reliabilityData500msVecUL");
		reliabilityData1sVecUL.setName("reliabilityData1sVecUL");

		reliabilityData10msVecDL.setName("reliabilityData10msVecDL");
		reliabilityData20msVecDL.setName("reliabilityData20msVecDL");
		reliabilityData50msVecDL.setName("reliabilityData50msVecDL");
		reliabilityData100msVecDL.setName("reliabilityData100msVecDL");
		reliabilityData200msVecDL.setName("reliabilityData200msVecDL");
		reliabilityData500msVecDL.setName("reliabilityData500msVecDL");
		reliabilityData1sVecDL.setName("reliabilityData1sVecDL");
		//

		//V2X
		reliabilityV2X10msVecUL.setName("reliabilityV2X10msVecUL");
		reliabilityV2X20msVecUL.setName("reliabilityV2X20msVecUL");
		reliabilityV2X50msVecUL.setName("reliabilityV2X50msVecUL");
		reliabilityV2X100msVecUL.setName("reliabilityV2X100msVecUL");
		reliabilityV2X200msVecUL.setName("reliabilityV2X200msVecUL");
		reliabilityV2X500msVecUL.setName("reliabilityV2X500msVecUL");
		reliabilityV2X1sVecUL.setName("reliabilityV2X1sVecUL");

		reliabilityV2X10msVecDL.setName("reliabilityV2X10msVecDL");
		reliabilityV2X20msVecDL.setName("reliabilityV2X20msVecDL");
		reliabilityV2X50msVecDL.setName("reliabilityV2X50msVecDL");
		reliabilityV2X100msVecDL.setName("reliabilityV2X100msVecDL");
		reliabilityV2X200msVecDL.setName("reliabilityV2X200msVecDL");
		reliabilityV2X500msVecDL.setName("reliabilityV2X500msVecDL");
		reliabilityV2X1sVecDL.setName("reliabilityV2X1sVecDL");
		//

		//Voip
		reliabilityVoip10msVecDL.setName("reliabilityVoip10msVecDL");
		reliabilityVoip20msVecDL.setName("reliabilityVoip20msVecDL");
		reliabilityVoip50msVecDL.setName("reliabilityVoip50msVecDL");
		reliabilityVoip100msVecDL.setName("reliabilityVoip100msVecDL");
		reliabilityVoip200msVecDL.setName("reliabilityVoip200msVecDL");
		reliabilityVoip500msVecDL.setName("reliabilityVoip500msVecDL");
		reliabilityVoip1sVecDL.setName("reliabilityVoip1sVecDL");

		reliabilityVoip10msVecUL.setName("reliabilityVoip10msVecUL");
		reliabilityVoip20msVecUL.setName("reliabilityVoip20msVecUL");
		reliabilityVoip50msVecUL.setName("reliabilityVoip50msVecUL");
		reliabilityVoip100msVecUL.setName("reliabilityVoip100msVecUL");
		reliabilityVoip200msVecUL.setName("reliabilityVoip200msVecUL");
		reliabilityVoip500msVecUL.setName("reliabilityVoip500msVecUL");
		reliabilityVoip1sVecUL.setName("reliabilityVoip1sVecUL");
		//

		//Video
		reliabilityVideo10msVecDL.setName("reliabilityVideo10msVecDL");
		reliabilityVideo20msVecDL.setName("reliabilityVideo20msVecDL");
		reliabilityVideo50msVecDL.setName("reliabilityVideo50msVecDL");
		reliabilityVideo100msVecDL.setName("reliabilityVideo100msVecDL");
		reliabilityVideo200msVecDL.setName("reliabilityVideo200msVecDL");
		reliabilityVideo500msVecDL.setName("reliabilityVideo500msVecDL");
		reliabilityVideo1sVecDL.setName("reliabilityVideo1sVecDL");

		reliabilityVideo10msVecUL.setName("reliabilityVideo10msVecUL");
		reliabilityVideo20msVecUL.setName("reliabilityVideo20msVecUL");
		reliabilityVideo50msVecUL.setName("reliabilityVideo50msVecUL");
		reliabilityVideo100msVecUL.setName("reliabilityVideo100msVecUL");
		reliabilityVideo200msVecUL.setName("reliabilityVideo200msVecUL");
		reliabilityVideo500msVecUL.setName("reliabilityVideo500msVecUL");
		reliabilityVideo1sVecUL.setName("reliabilityVideo1sVecUL");
		//

		considerDatasizeAndMessages = par("considerDatasizeAndMessages").boolValue();
		//

		lastSentStatusUpdateSN = 0;

		selfMsg = new cMessage("sendTimer");
		take(selfMsg);
		//for DL and UL, set in OmnetINI
		startTime = NOW + uniform(0, par("startTime").doubleValue());
		stopTime = SIMTIME_MAX;
		if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
			throw cRuntimeError("Invalid startTime/stopTime parameters");
	}
}

//saving statistics
void TrafficGenerator::finish() {
	recordScalar("packets sent", numberSentPackets);
	recordScalar("packets received", numberReceivedPackets);

	if (recPacketsVideoOutBudget > 0 && recPacketsVideo > 0) {
		double reliabilityVideo = 1.0 - double(recPacketsVideoOutBudget / recPacketsVideo);
		recordScalar("reliabilityVideo", reliabilityVideo);
	}

	if (recPacketsV2XOutBudget > 0 && recPacketsV2X > 0) {
		double reliabilityV2X = 1.0 - double(recPacketsV2XOutBudget / recPacketsV2X);
		recordScalar("reliabilityV2X", reliabilityV2X);
	}

	if (recPacketsVoipOutBudget > 0 && recPacketsVoip > 0) {
		double reliabilityVoip = 1.0 - double(recPacketsVoipOutBudget / recPacketsVoip);
		recordScalar("reliabilityVoip", reliabilityVoip);
	}

	if (recPacketsDataOutBudget > 0 && recPacketsData > 0) {
		double reliabilityData = 1.0 - double(recPacketsDataOutBudget / recPacketsData);
		recordScalar("reliabilityData", reliabilityData);
	}

	if (nodeType == "car") {

		for (auto & var : connectionsServToUE) {
			//record all values for the four applications

			double recPacketsDataOutBudget10ms = var.second.statReport.recPacketsDataOutBudget10ms;
			double recPacketsDataOutBudget20ms = var.second.statReport.recPacketsDataOutBudget20ms;
			double recPacketsDataOutBudget50ms = var.second.statReport.recPacketsDataOutBudget50ms;
			double recPacketsDataOutBudget100ms = var.second.statReport.recPacketsDataOutBudget100ms;
			double recPacketsDataOutBudget200ms = var.second.statReport.recPacketsDataOutBudget200ms;
			double recPacketsDataOutBudget500ms = var.second.statReport.recPacketsDataOutBudget500ms;
			double recPacketsDataOutBudget1s = var.second.statReport.recPacketsDataOutBudget1s;
			double recPacketsData = var.second.statReport.recPacketsData;

			if (recPacketsData <= 0) {
				//std::cout << "TrafficGenerator finish recording Data --> no packets arrived!" << std::endl;
			} else {

				double reliabilityData10ms = 1.0 - double(recPacketsDataOutBudget10ms / recPacketsData);
				recordScalar("reliabilityData10msDL", reliabilityData10ms);
				reliabilityData10msVecDL.record(reliabilityData10ms);

				double reliabilityData20ms = 1.0 - double(recPacketsDataOutBudget20ms / recPacketsData);
				recordScalar("reliabilityData20msDL", reliabilityData20ms);
				reliabilityData20msVecDL.record(reliabilityData20ms);

				double reliabilityData50ms = 1.0 - double(recPacketsDataOutBudget50ms / recPacketsData);
				recordScalar("reliabilityData50msDL", reliabilityData50ms);
				reliabilityData50msVecDL.record(reliabilityData50ms);

				double reliabilityData100ms = 1.0 - double(recPacketsDataOutBudget100ms / recPacketsData);
				recordScalar("reliabilityData100msDL", reliabilityData100ms);
				reliabilityData100msVecDL.record(reliabilityData100ms);

				double reliabilityData200ms = 1.0 - double(recPacketsDataOutBudget200ms / recPacketsData);
				recordScalar("reliabilityData200msDL", reliabilityData200ms);
				reliabilityData200msVecDL.record(reliabilityData200ms);

				double reliabilityData500ms = 1.0 - double(recPacketsDataOutBudget500ms / recPacketsData);
				recordScalar("reliabilityData500msDL", reliabilityData500ms);
				reliabilityData500msVecDL.record(reliabilityData500ms);

				double reliabilityData1s = 1.0 - double(recPacketsDataOutBudget1s / recPacketsData);
				recordScalar("reliabilityData1sDL", reliabilityData1s);
				reliabilityData1sVecDL.record(reliabilityData1s);
			}
			//

			//V2X
			double recPacketsV2XOutBudget10ms = var.second.statReport.recPacketsV2XOutBudget10ms;
			double recPacketsV2XOutBudget20ms = var.second.statReport.recPacketsV2XOutBudget20ms;
			double recPacketsV2XOutBudget50ms = var.second.statReport.recPacketsV2XOutBudget50ms;
			double recPacketsV2XOutBudget100ms = var.second.statReport.recPacketsV2XOutBudget100ms;
			double recPacketsV2XOutBudget200ms = var.second.statReport.recPacketsV2XOutBudget200ms;
			double recPacketsV2XOutBudget500ms = var.second.statReport.recPacketsV2XOutBudget500ms;
			double recPacketsV2XOutBudget1s = var.second.statReport.recPacketsV2XOutBudget1s;
			double recPacketsV2X = var.second.statReport.recPacketsV2X;

			if (recPacketsV2X <= 0) {
//				std::cout << "TrafficGenerator finish recording V2X --> no packets arrived!" << std::endl;
			} else {

				double reliabilityV2X10ms = 1.0 - double(recPacketsV2XOutBudget10ms / recPacketsV2X);
				recordScalar("reliabilityV2X10msDL", reliabilityV2X10ms);
				reliabilityV2X10msVecDL.record(reliabilityV2X10ms);

				double reliabilityV2X20ms = 1.0 - double(recPacketsV2XOutBudget20ms / recPacketsV2X);
				recordScalar("reliabilityV2X20msDL", reliabilityV2X20ms);
				reliabilityV2X20msVecDL.record(reliabilityV2X20ms);

				double reliabilityV2X50ms = 1.0 - double(recPacketsV2XOutBudget50ms / recPacketsV2X);
				recordScalar("reliabilityV2X50msDL", reliabilityV2X50ms);
				reliabilityV2X50msVecDL.record(reliabilityV2X50ms);

				double reliabilityV2X100ms = 1.0 - double(recPacketsV2XOutBudget100ms / recPacketsV2X);
				recordScalar("reliabilityV2X100msDL", reliabilityV2X100ms);
				reliabilityV2X100msVecDL.record(reliabilityV2X100ms);

				double reliabilityV2X200ms = 1.0 - double(recPacketsV2XOutBudget200ms / recPacketsV2X);
				recordScalar("reliabilityV2X200msDL", reliabilityV2X200ms);
				reliabilityV2X200msVecDL.record(reliabilityV2X200ms);

				double reliabilityV2X500ms = 1.0 - double(recPacketsV2XOutBudget500ms / recPacketsV2X);
				recordScalar("reliabilityV2X500msDL", reliabilityV2X500ms);
				reliabilityV2X500msVecDL.record(reliabilityV2X500ms);

				double reliabilityV2X1s = 1.0 - double(recPacketsV2XOutBudget1s / recPacketsV2X);
				recordScalar("reliabilityV2X1sDL", reliabilityV2X1s);
				reliabilityV2X1sVecDL.record(reliabilityV2X1s);
			}
			//

			//Voip
			double recPacketsVoipOutBudget10ms = var.second.statReport.recPacketsVoipOutBudget10ms;
			double recPacketsVoipOutBudget20ms = var.second.statReport.recPacketsVoipOutBudget20ms;
			double recPacketsVoipOutBudget50ms = var.second.statReport.recPacketsVoipOutBudget50ms;
			double recPacketsVoipOutBudget100ms = var.second.statReport.recPacketsVoipOutBudget100ms;
			double recPacketsVoipOutBudget200ms = var.second.statReport.recPacketsVoipOutBudget200ms;
			double recPacketsVoipOutBudget500ms = var.second.statReport.recPacketsVoipOutBudget500ms;
			double recPacketsVoipOutBudget1s = var.second.statReport.recPacketsVoipOutBudget1s;
			double recPacketsVoip = var.second.statReport.recPacketsVoip;

			if (recPacketsVoip <= 0) {
//				std::cout << "TrafficGenerator finish recording VoIP --> no packets arrived!" << std::endl;
			} else {

				double reliabilityVoip10ms = 1.0 - double(recPacketsVoipOutBudget10ms / recPacketsVoip);
				recordScalar("reliabilityVoip10msDL", reliabilityVoip10ms);
				reliabilityVoip10msVecDL.record(reliabilityVoip10ms);

				double reliabilityVoip20ms = 1.0 - double(recPacketsVoipOutBudget20ms / recPacketsVoip);
				recordScalar("reliabilityVoip20msDL", reliabilityVoip20ms);
				reliabilityVoip20msVecDL.record(reliabilityVoip20ms);

				double reliabilityVoip50ms = 1.0 - double(recPacketsVoipOutBudget50ms / recPacketsVoip);
				recordScalar("reliabilityVoip50msDL", reliabilityVoip50ms);
				reliabilityVoip50msVecDL.record(reliabilityVoip50ms);

				double reliabilityVoip100ms = 1.0 - double(recPacketsVoipOutBudget100ms / recPacketsVoip);
				recordScalar("reliabilityVoip100msDL", reliabilityVoip100ms);
				reliabilityVoip100msVecDL.record(reliabilityVoip100ms);

				double reliabilityVoip200ms = 1.0 - double(recPacketsVoipOutBudget200ms / recPacketsVoip);
				recordScalar("reliabilityVoip200msDL", reliabilityVoip200ms);
				reliabilityVoip200msVecDL.record(reliabilityVoip200ms);

				double reliabilityVoip500ms = 1.0 - double(recPacketsVoipOutBudget500ms / recPacketsVoip);
				recordScalar("reliabilityVoip500msDL", reliabilityVoip500ms);
				reliabilityVoip500msVecDL.record(reliabilityVoip500ms);

				double reliabilityVoip1s = 1.0 - double(recPacketsVoipOutBudget1s / recPacketsVoip);
				recordScalar("reliabilityVoip1sDL", reliabilityVoip1s);
				reliabilityVoip1sVecDL.record(reliabilityVoip1s);
			}
			//

			//Video
			double recPacketsVideoOutBudget10ms = var.second.statReport.recPacketsVideoOutBudget10ms;
			double recPacketsVideoOutBudget20ms = var.second.statReport.recPacketsVideoOutBudget20ms;
			double recPacketsVideoOutBudget50ms = var.second.statReport.recPacketsVideoOutBudget50ms;
			double recPacketsVideoOutBudget100ms = var.second.statReport.recPacketsVideoOutBudget100ms;
			double recPacketsVideoOutBudget200ms = var.second.statReport.recPacketsVideoOutBudget200ms;
			double recPacketsVideoOutBudget500ms = var.second.statReport.recPacketsVideoOutBudget500ms;
			double recPacketsVideoOutBudget1s = var.second.statReport.recPacketsVideoOutBudget1s;
			double recPacketsVideo = var.second.statReport.recPacketsVideo;

			if (recPacketsVideo <= 0) {
//				std::cout << "TrafficGenerator finish recording Video --> no packets arrived!" << std::endl;
			} else {

				double reliabilityVideo10ms = 1.0 - double(recPacketsVideoOutBudget10ms / recPacketsVideo);
				recordScalar("reliabilityVideo10msDL", reliabilityVideo10ms);
				reliabilityVideo10msVecDL.record(reliabilityVideo10ms);

				double reliabilityVideo20ms = 1.0 - double(recPacketsVideoOutBudget20ms / recPacketsVideo);
				recordScalar("reliabilityVideo20msDL", reliabilityVideo20ms);
				reliabilityVideo20msVecDL.record(reliabilityVideo20ms);

				double reliabilityVideo50ms = 1.0 - double(recPacketsVideoOutBudget50ms / recPacketsVideo);
				recordScalar("reliabilityVideo50msDL", reliabilityVideo50ms);
				reliabilityVideo50msVecDL.record(reliabilityVideo50ms);

				double reliabilityVideo100ms = 1.0 - double(recPacketsVideoOutBudget100ms / recPacketsVideo);
				recordScalar("reliabilityVideo100msDL", reliabilityVideo100ms);
				reliabilityVideo100msVecDL.record(reliabilityVideo100ms);

				double reliabilityVideo200ms = 1.0 - double(recPacketsVideoOutBudget200ms / recPacketsVideo);
				recordScalar("reliabilityVideo200msDL", reliabilityVideo200ms);
				reliabilityVideo200msVecDL.record(reliabilityVideo200ms);

				double reliabilityVideo500ms = 1.0 - double(recPacketsVideoOutBudget500ms / recPacketsVideo);
				recordScalar("reliabilityVideo500msDL", reliabilityVideo500ms);
				reliabilityVideo500msVecDL.record(reliabilityVideo500ms);

				double reliabilityVideo1s = 1.0 - double(recPacketsVideoOutBudget1s / recPacketsVideo);
				recordScalar("reliabilityVideo1sDL", reliabilityVideo1s);
				reliabilityVideo1sVecDL.record(reliabilityVideo1s);
			}
			//
		}

	}

	//on server side
	if (nodeType == "server") {

		for (auto & var : connectionsUEtoServ) {
			//record all different reliabilities

			double recPacketsDataOutBudget10ms = var.second.statReport.recPacketsDataOutBudget10ms;
			double recPacketsDataOutBudget20ms = var.second.statReport.recPacketsDataOutBudget20ms;
			double recPacketsDataOutBudget50ms = var.second.statReport.recPacketsDataOutBudget50ms;
			double recPacketsDataOutBudget100ms = var.second.statReport.recPacketsDataOutBudget100ms;
			double recPacketsDataOutBudget200ms = var.second.statReport.recPacketsDataOutBudget200ms;
			double recPacketsDataOutBudget500ms = var.second.statReport.recPacketsDataOutBudget500ms;
			double recPacketsDataOutBudget1s = var.second.statReport.recPacketsDataOutBudget1s;
			double recPacketsData = var.second.statReport.recPacketsData;

			if (recPacketsData <= 0) {
//				std::cout << "TrafficGenerator Server finish recording Data --> no packets arrived!" << std::endl;
			} else {

				double reliabilityData10ms = 1.0 - double(recPacketsDataOutBudget10ms / recPacketsData);
				recordScalar("reliabilityData10msUL", reliabilityData10ms);
				reliabilityData10msVecUL.record(reliabilityData10ms);

				double reliabilityData20ms = 1.0 - double(recPacketsDataOutBudget20ms / recPacketsData);
				recordScalar("reliabilityData20msUL", reliabilityData20ms);
				reliabilityData20msVecUL.record(reliabilityData20ms);

				double reliabilityData50ms = 1.0 - double(recPacketsDataOutBudget50ms / recPacketsData);
				recordScalar("reliabilityData50msUL", reliabilityData50ms);
				reliabilityData50msVecUL.record(reliabilityData50ms);

				double reliabilityData100ms = 1.0 - double(recPacketsDataOutBudget100ms / recPacketsData);
				recordScalar("reliabilityData100msUL", reliabilityData100ms);
				reliabilityData100msVecUL.record(reliabilityData100ms);

				double reliabilityData200ms = 1.0 - double(recPacketsDataOutBudget200ms / recPacketsData);
				recordScalar("reliabilityData200msUL", reliabilityData200ms);
				reliabilityData200msVecUL.record(reliabilityData200ms);

				double reliabilityData500ms = 1.0 - double(recPacketsDataOutBudget500ms / recPacketsData);
				recordScalar("reliabilityData500msUL", reliabilityData500ms);
				reliabilityData500msVecUL.record(reliabilityData500ms);

				double reliabilityData1s = 1.0 - double(recPacketsDataOutBudget1s / recPacketsData);
				recordScalar("reliabilityData1sUL", reliabilityData1s);
				reliabilityData1sVecUL.record(reliabilityData1s);
			}

			//V2X
			double recPacketsV2XOutBudget10ms = var.second.statReport.recPacketsV2XOutBudget10ms;
			double recPacketsV2XOutBudget20ms = var.second.statReport.recPacketsV2XOutBudget20ms;
			double recPacketsV2XOutBudget50ms = var.second.statReport.recPacketsV2XOutBudget50ms;
			double recPacketsV2XOutBudget100ms = var.second.statReport.recPacketsV2XOutBudget100ms;
			double recPacketsV2XOutBudget200ms = var.second.statReport.recPacketsV2XOutBudget200ms;
			double recPacketsV2XOutBudget500ms = var.second.statReport.recPacketsV2XOutBudget500ms;
			double recPacketsV2XOutBudget1s = var.second.statReport.recPacketsV2XOutBudget1s;
			double recPacketsV2X = var.second.statReport.recPacketsV2X;

			if (recPacketsV2X <= 0) {
//				std::cout << "TrafficGenerator Server finish recording V2X --> no packets arrived!" << std::endl;
			} else {

				double reliabilityV2X10ms = 1.0 - double(recPacketsV2XOutBudget10ms / recPacketsV2X);
				recordScalar("reliabilityV2X10msUL", reliabilityV2X10ms);
				reliabilityV2X10msVecUL.record(reliabilityV2X10ms);

				double reliabilityV2X20ms = 1.0 - double(recPacketsV2XOutBudget20ms / recPacketsV2X);
				recordScalar("reliabilityV2X20msUL", reliabilityV2X20ms);
				reliabilityV2X20msVecUL.record(reliabilityV2X20ms);

				double reliabilityV2X50ms = 1.0 - double(recPacketsV2XOutBudget50ms / recPacketsV2X);
				recordScalar("reliabilityV2X50msUL", reliabilityV2X50ms);
				reliabilityV2X50msVecUL.record(reliabilityV2X50ms);

				double reliabilityV2X100ms = 1.0 - double(recPacketsV2XOutBudget100ms / recPacketsV2X);
				recordScalar("reliabilityV2X100msUL", reliabilityV2X100ms);
				reliabilityV2X100msVecUL.record(reliabilityV2X100ms);

				double reliabilityV2X200ms = 1.0 - double(recPacketsV2XOutBudget200ms / recPacketsV2X);
				recordScalar("reliabilityV2X200msUL", reliabilityV2X200ms);
				reliabilityV2X200msVecUL.record(reliabilityV2X200ms);

				double reliabilityV2X500ms = 1.0 - double(recPacketsV2XOutBudget500ms / recPacketsV2X);
				recordScalar("reliabilityV2X500msUL", reliabilityV2X500ms);
				reliabilityV2X500msVecUL.record(reliabilityV2X500ms);

				double reliabilityV2X1s = 1.0 - double(recPacketsV2XOutBudget1s / recPacketsV2X);
				recordScalar("reliabilityV2X1sUL", reliabilityV2X1s);
				reliabilityV2X1sVecUL.record(reliabilityV2X1s);
			}
			//

			//Voip
			double recPacketsVoipOutBudget10ms = var.second.statReport.recPacketsVoipOutBudget10ms;
			double recPacketsVoipOutBudget20ms = var.second.statReport.recPacketsVoipOutBudget20ms;
			double recPacketsVoipOutBudget50ms = var.second.statReport.recPacketsVoipOutBudget50ms;
			double recPacketsVoipOutBudget100ms = var.second.statReport.recPacketsVoipOutBudget100ms;
			double recPacketsVoipOutBudget200ms = var.second.statReport.recPacketsVoipOutBudget200ms;
			double recPacketsVoipOutBudget500ms = var.second.statReport.recPacketsVoipOutBudget500ms;
			double recPacketsVoipOutBudget1s = var.second.statReport.recPacketsVoipOutBudget1s;
			double recPacketsVoip = var.second.statReport.recPacketsVoip;

			if (recPacketsVoip <= 0) {
//				std::cout << "TrafficGenerator Server finish recording VoIP --> no packets arrived!" << std::endl;
			} else {

				double reliabilityVoip10ms = 1.0 - double(recPacketsVoipOutBudget10ms / recPacketsVoip);
				recordScalar("reliabilityVoip10msUL", reliabilityVoip10ms);
				reliabilityVoip10msVecUL.record(reliabilityVoip10ms);

				double reliabilityVoip20ms = 1.0 - double(recPacketsVoipOutBudget20ms / recPacketsVoip);
				recordScalar("reliabilityVoip20msUL", reliabilityVoip20ms);
				reliabilityVoip20msVecUL.record(reliabilityVoip20ms);

				double reliabilityVoip50ms = 1.0 - double(recPacketsVoipOutBudget50ms / recPacketsVoip);
				recordScalar("reliabilityVoip50msUL", reliabilityVoip50ms);
				reliabilityVoip50msVecUL.record(reliabilityVoip50ms);

				double reliabilityVoip100ms = 1.0 - double(recPacketsVoipOutBudget100ms / recPacketsVoip);
				recordScalar("reliabilityVoip100msUL", reliabilityVoip100ms);
				reliabilityVoip100msVecUL.record(reliabilityVoip100ms);

				double reliabilityVoip200ms = 1.0 - double(recPacketsVoipOutBudget200ms / recPacketsVoip);
				recordScalar("reliabilityVoip200msUL", reliabilityVoip200ms);
				reliabilityVoip200msVecUL.record(reliabilityVoip200ms);

				double reliabilityVoip500ms = 1.0 - double(recPacketsVoipOutBudget500ms / recPacketsVoip);
				recordScalar("reliabilityVoip500msUL", reliabilityVoip500ms);
				reliabilityVoip500msVecUL.record(reliabilityVoip500ms);

				double reliabilityVoip1s = 1.0 - double(recPacketsVoipOutBudget1s / recPacketsVoip);
				recordScalar("reliabilityVoip1sUL", reliabilityVoip1s);
				reliabilityVoip1sVecUL.record(reliabilityVoip1s);
			}
			//

			//Video
			double recPacketsVideoOutBudget10ms = var.second.statReport.recPacketsVideoOutBudget10ms;
			double recPacketsVideoOutBudget20ms = var.second.statReport.recPacketsVideoOutBudget20ms;
			double recPacketsVideoOutBudget50ms = var.second.statReport.recPacketsVideoOutBudget50ms;
			double recPacketsVideoOutBudget100ms = var.second.statReport.recPacketsVideoOutBudget100ms;
			double recPacketsVideoOutBudget200ms = var.second.statReport.recPacketsVideoOutBudget200ms;
			double recPacketsVideoOutBudget500ms = var.second.statReport.recPacketsVideoOutBudget500ms;
			double recPacketsVideoOutBudget1s = var.second.statReport.recPacketsVideoOutBudget1s;
			double recPacketsVideo = var.second.statReport.recPacketsVideo;

			if (recPacketsVideo <= 0) {
//				std::cout << "TrafficGenerator Server finish recording Video --> no packets arrived!" << std::endl;
			} else {

				double reliabilityVideo10ms = 1.0 - double(recPacketsVideoOutBudget10ms / recPacketsVideo);
				recordScalar("reliabilityVideo10msUL", reliabilityVideo10ms);
				reliabilityVideo10msVecUL.record(reliabilityVideo10ms);

				double reliabilityVideo20ms = 1.0 - double(recPacketsVideoOutBudget20ms / recPacketsVideo);
				recordScalar("reliabilityVideo20msUL", reliabilityVideo20ms);
				reliabilityVideo20msVecUL.record(reliabilityVideo20ms);

				double reliabilityVideo50ms = 1.0 - double(recPacketsVideoOutBudget50ms / recPacketsVideo);
				recordScalar("reliabilityVideo50msUL", reliabilityVideo50ms);
				reliabilityVideo50msVecUL.record(reliabilityVideo50ms);

				double reliabilityVideo100ms = 1.0 - double(recPacketsVideoOutBudget100ms / recPacketsVideo);
				recordScalar("reliabilityVideo100msUL", reliabilityVideo100ms);
				reliabilityVideo100msVecUL.record(reliabilityVideo100ms);

				double reliabilityVideo200ms = 1.0 - double(recPacketsVideoOutBudget200ms / recPacketsVideo);
				recordScalar("reliabilityVideo200msUL", reliabilityVideo200ms);
				reliabilityVideo200msVecUL.record(reliabilityVideo200ms);

				double reliabilityVideo500ms = 1.0 - double(recPacketsVideoOutBudget500ms / recPacketsVideo);
				recordScalar("reliabilityVideo500msUL", reliabilityVideo500ms);
				reliabilityVideo500msVecUL.record(reliabilityVideo500ms);

				double reliabilityVideo1s = 1.0 - double(recPacketsVideoOutBudget1s / recPacketsVideo);
				recordScalar("reliabilityVideo1sUL", reliabilityVideo1s);
				reliabilityVideo1sVecUL.record(reliabilityVideo1s);
			}
			//
		}
	}

	recordScalar("lostPacketsVideo", lostPacketsVideo);
	recordScalar("lostPacketsV2X", lostPacketsV2X);
	recordScalar("lostPacketsVoip", lostPacketsVoip);
	recordScalar("lostPacketsData", lostPacketsData);

	recordScalar("recPacketsVideo", recPacketsVideo);
	recordScalar("recPacketsV2X", recPacketsV2X);
	recordScalar("recPacketsVoip", recPacketsVoip);
	recordScalar("recPacketsData", recPacketsData);

	recordScalar("sentPacketsVideo", sentPacketsVideo);
	recordScalar("sentPacketsV2X", sentPacketsV2X);
	recordScalar("sentPacketsVoip", sentPacketsVoip);
	recordScalar("sentPacketsData", sentPacketsData);

	recordScalar("recPacketsVideoOutBudget", recPacketsVideoOutBudget);
	recordScalar("recPacketsV2XOutBudget", recPacketsV2XOutBudget);
	recordScalar("recPacketsVoipOutBudget", recPacketsVoipOutBudget);
	recordScalar("recPacketsDataOutBudget", recPacketsDataOutBudget);

	recordScalar("delayVoipVariationOut", delayVoipVariationOut);
	recordScalar("delayV2XVariationOut", delayV2XVariationOut);
	recordScalar("delayVideoVariationOut", delayVideoVariationOut);
	recordScalar("delayDataVariationOut", delayDataVariationOut);

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
 * calculates and records statistics from received packtes from cars
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

		V2XMessage * temp = check_and_cast<V2XMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		unsigned int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();

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
			if (NOW - pk->getCreationTime() > v2xDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget++;
				recPacketsV2XOutBudget++;
			}
			//
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
				// packetDelayVariation
				calcPVV2XUL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastV2X->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsV2X = lostPackets; //lost packets from this node
				lostPacketsV2X = lostPackets; //all over summary of lost packets
			}

			if (NOW - pk->getCreationTime() > v2xDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsV2XOutBudget++;
				recPacketsV2XOutBudget++;
			} else {
				connectionsUEtoServ[nodeId].statReport.recPacketsV2X++;
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

		VideoMessage * temp = check_and_cast<VideoMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//
			recPacketsVideo++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsUEtoServ[nodeId].statReport.recPacketsVideo++;

			//Reliability
			if (NOW - pk->getCreationTime() > videoDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget++;
				recPacketsVideoOutBudget++;
			}
			//
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
				calcPVVideoUL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsVideo = lostPackets; //lost packets from this node
				lostPacketsVideo = lostPackets; //all over summary of lost packets

			}
			//Reliability
			if (NOW - pk->getCreationTime() > videoDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVideoOutBudget++;
				recPacketsVideoOutBudget++;
			} else {
				connectionsUEtoServ[nodeId].statReport.recPacketsVideo++;
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

		VoIPMessage * temp = check_and_cast<VoIPMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//
			recPacketsVoip++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsUEtoServ[nodeId].statReport.recPacketsVoip++;

			//Reliability
			if (NOW - pk->getCreationTime() > voipDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget++;
				recPacketsVoipOutBudget++;
			}
			//
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
				calcPVVoipUL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastVoIP->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsVoip = lostPackets; //lost packets from this node
				lostPacketsVoip = lostPackets; //all over summary of lost packets

			}
			//Reliability
			if (NOW - pk->getCreationTime() > voipDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsVoipOutBudget++;
				recPacketsVoipOutBudget++;
			} else {
				connectionsUEtoServ[nodeId].statReport.recPacketsVoip++;
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

		DataMessage * temp = check_and_cast<DataMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();
		temp->setArrivalTime(NOW);
		int messageLen = temp->getByteLength();

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {

			recPacketsData++;

			connectionsUEtoServ[nodeId].statReport.recPacketsData++;

			//Reliability
			if (NOW - pk->getCreationTime() > dataDelayBudget) {
				//out of Budget
				connectionsUEtoServ[nodeId].statReport.recPacketsDataOutBudget++;
				recPacketsDataOutBudget++;
			}
			//
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
							TrafficGeneratorCarUL * tmp1 = check_and_cast<TrafficGeneratorCarUL*>(getSimulation()->getModuleByPath(name)->getSubmodule("udpApp", 0));
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
				calcPVDataUL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsUEtoServ[nodeId].statReport.lastData->getSequenceNumber() + 1);
				connectionsUEtoServ[nodeId].statReport.lostPacketsData = lostPackets; //lost packets from this node
				lostPacketsData = lostPackets; //all over summary of lost packets

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

void TrafficGeneratorCarUL::sendPacket() {

	unsigned short nodeId = getNRBinder()->getMacNodeId(localAddress_.toIPv4());
	numberSentPackets++;
	//from UDPBasicApp
	numSent++;
	//

	if (strcmp(packetName, "V2X") == 0) {

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
			connectionsUEtoServ[nodeId].statReport.sentPacketsV2X++;

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

			tmp.statReport.sentPacketsV2X++;
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

		//send the request at the same tim
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

		if (connectionsUEtoServ.find(nodeId) != connectionsUEtoServ.end()) {
			//already one entry
			//find the corresponding destination

			payload->setSequenceNumber(connectionsUEtoServ[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
			delete connectionsUEtoServ[nodeId].statReport.lastVideo;
			connectionsUEtoServ[nodeId].statReport.lastVideo = payload->dup();
			connectionsUEtoServ[nodeId].statReport.sentPacketsVideo++;

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

			tmp.statReport.sentPacketsVideo++;
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
			connectionsUEtoServ[nodeId].statReport.sentPacketsVoip++;

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

			tmp.statReport.sentPacketsVoip++;
			tmp.statReport.lastVoIP = payload->dup();

			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		socket.sendTo(payload, destAddress_, destPort);

	} else if (strcmp(packetName, "Data") == 0) {

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
			connectionsUEtoServ[nodeId].statReport.sentPacketsData++;

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

			tmp.statReport.sentPacketsData++;
			tmp.statReport.lastData = payload->dup();

			tmp.messages = messages;

			connectionsUEtoServ[nodeId] = tmp;
		}

		socket.sendTo(payload, destAddress_, destPort);
	} else
		throw cRuntimeError("Unknown Application Type");

}

void TrafficGeneratorCarUL::processSend() {

	sendPacket();

	if (!sendVideoPacket)
		return;

	if (!sendDataPacket)
		return;

	double sendInterval = par("sendInterval").doubleValue();
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
		const char * carName = getParentModule()->getFullName();

		//for V2X Broadcast
		bool flag = getSystemModule()->par("v2vMulticastFlag").boolValue();
		if (flag)
			return;
		//

		TrafficGeneratorServerDL * tmp0 = check_and_cast<TrafficGeneratorServerDL*>(getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", 0));
		listener0 = new Listener(tmp0);
		carNameSignal = registerSignal("carName");
		subscribe(carNameSignal, listener0);

		if (getAncestorPar("numUdpApps").intValue() > 1) {

			unsigned short tmpGate = 0;
			unsigned short tmpGateTwo = 0;
			unsigned short tmpGateThree = 0;
			if (getAncestorPar("oneServer").boolValue())
				tmpGate = 1;
			TrafficGeneratorServerDL * tmp1 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGate));

			if (getAncestorPar("oneServer").boolValue())
				tmpGateTwo = 2;
			TrafficGeneratorServerDL * tmp2 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGateTwo));

			if (getAncestorPar("oneServer").boolValue())
				tmpGateThree = 3;
			TrafficGeneratorServerDL * tmp3 = check_and_cast<TrafficGeneratorServerDL*>(
					getSimulation()->getModuleByPath(par("destAddresses").stdstringValue().c_str())->getSubmodule("udpApp", tmpGateThree));

			listener1 = new Listener(tmp1);
			listener2 = new Listener(tmp2);
			listener3 = new Listener(tmp3);
			subscribe(carNameSignal, listener1);
			subscribe(carNameSignal, listener2);
			subscribe(carNameSignal, listener3);
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

		V2XMessage * temp = check_and_cast<V2XMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		unsigned int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();

		cModule *mod = getSimulation()->getModuleByPath(name);
		if (!mod) {
			delete temp;
			return;
		}

		const char * myName = getParentModule()->getFullName();



		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
			//
			recPacketsV2X++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsServToUE[nodeId].statReport.recPacketsV2X++;

			//Reliability
			if (NOW - pk->getCreationTime() > v2xDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget++;
				recPacketsV2XOutBudget++;
			}
			//
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
				calcPVV2XDL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastV2X->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsV2X = lostPackets; //lost packets from this node
				lostPacketsV2X = lostPackets; //all over summary of lost packets

			}
			//
			if (NOW - pk->getCreationTime() > v2xDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsV2XOutBudget++;
				recPacketsV2XOutBudget++;
			} else {
				connectionsServToUE[nodeId].statReport.recPacketsV2X++;
			}

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

		VideoMessage * temp = check_and_cast<VideoMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
			//
			recPacketsVideo++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsServToUE[nodeId].statReport.recPacketsVideo++;

			//Reliability
			if (NOW - pk->getCreationTime() > videoDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget++;
				recPacketsVideoOutBudget++;
			}
			//
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
				calcPVVideoDL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsVideo = lostPackets; //lost packets from this node
				lostPacketsVideo = lostPackets; //all over summary of lost packets

			}

			if (NOW - pk->getCreationTime() > videoDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVideoOutBudget++;
				recPacketsVideoOutBudget++;
			} else {
				connectionsServToUE[nodeId].statReport.recPacketsVideo++;
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

		VoIPMessage * temp = check_and_cast<VoIPMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();
		temp->setArrivalTime(NOW);

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
			//
			recPacketsVoip++;
			//
			//already one entry
			//find the corresponding destination and check the next sequenceNumber

			connectionsServToUE[nodeId].statReport.recPacketsVoip++;

			//Reliability
			if (NOW - pk->getCreationTime() > voipDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget++;
				recPacketsVoipOutBudget++;
			}
			//
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
				// packetDelayVariation
				calcPVVoipDL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastVoIP->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsVoip = lostPackets; //lost packets from this node
				lostPacketsVoip = lostPackets; //all over summary of lost packets

			}

			if (NOW - pk->getCreationTime() > voipDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsVoipOutBudget++;
				recPacketsVoipOutBudget++;
			} else {
				connectionsServToUE[nodeId].statReport.recPacketsVoip++;
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

		DataMessage * temp = check_and_cast<DataMessage*>(pk->dup());
		int nodeId = temp->getSenderNodeId();
		int number = temp->getSequenceNumber();
		const char * name = temp->getSenderName();
		unsigned int messageLen = temp->getByteLength();
		temp->setArrivalTime(NOW);

		if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {

			//
			recPacketsData++;
			//
			connectionsServToUE[nodeId].statReport.recPacketsData++;

			//Reliability
			if (NOW - pk->getCreationTime() > dataDelayBudget) {
				//out of Budget
				connectionsServToUE[nodeId].statReport.recPacketsDataOutBudget++;
				recPacketsDataOutBudget++;
			}
			//
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
							TrafficGeneratorServerDL * tmp1 = check_and_cast<TrafficGeneratorServerDL*>(getSimulation()->getModuleByPath(name)->getSubmodule("udpApp", 0));
							const char* ueName = this->getParentModule()->getFullName();
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
				calcPVDataDL(nodeId, temp);

			} else {
				//some packet lost on the way
				unsigned int lostPackets = number - (connectionsServToUE[nodeId].statReport.lastData->getSequenceNumber() + 1);
				connectionsServToUE[nodeId].statReport.lostPacketsData = lostPackets; //lost packets from this node
				lostPacketsData = lostPackets; //all over summary of lost packets

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
			sendPacket();
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
void TrafficGeneratorServerDL::sendPacket() {

	if (names.size() == 0) {
		scheduleAt(NOW + par("sendInterval").doubleValue(), selfMsg);
		return;
	}

	simtime_t nextSelfMsg = NOW + par("sendInterval").doubleValue() + uniform(0, par("resendingDelay").doubleValue());

	for (auto & var : names) {

		if (carsSendingTimes[var] == NOW) {

			std::string carName = var;
			carsSendingTimes[var] = nextSelfMsg;

			cModule *mod = getSimulation()->getModuleByPath(carName.c_str());
			if (!mod) {
				names.erase(carName);
				carsSendingTimes.erase(carName);
				continue;
			}

			int omnetId = mod->getId();
			int nodeId = getNRBinder()->getMacNodeIdFromOmnetId(omnetId);

			numberSentPackets++;
			//from UDPBasicApp
			numSent++;
			//

			if (strcmp(packetName, "V2X") == 0) {
				sentPacketsV2X++;
				V2XMessage *payload = new V2XMessage(packetName);
				payload->setByteLength(messageLength);
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
					connectionsServToUE[nodeId].statReport.sentPacketsV2X++;

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

					tmp.statReport.sentPacketsV2X++;
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

				if (connectionsServToUE.find(nodeId) != connectionsServToUE.end()) {
					//already one entry
					//find the corresponding destination

					payload->setSequenceNumber(connectionsServToUE[nodeId].statReport.lastVideo->getSequenceNumber() + 1);
					delete connectionsServToUE[nodeId].statReport.lastVideo;
					connectionsServToUE[nodeId].statReport.lastVideo = payload->dup();
					connectionsServToUE[nodeId].statReport.sentPacketsVideo++;

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

					tmp.statReport.sentPacketsVideo++;
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
					connectionsServToUE[nodeId].statReport.sentPacketsVoip++;

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

					tmp.statReport.sentPacketsVoip++;
					tmp.statReport.lastVoIP = payload->dup();

					tmp.messages = messages;

					connectionsServToUE[nodeId] = tmp;
				}

				socket.sendTo(payload, L3AddressResolver().resolve(carName.c_str()), destPort);
			} else if (strcmp(packetName, "Data") == 0) {
				sentPacketsData++;

				DataMessage *payload = new DataMessage(packetName);
				payload->setByteLength(messageLength);

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
					connectionsServToUE[nodeId].statReport.sentPacketsData++;

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

					tmp.statReport.sentPacketsData++;
					tmp.statReport.lastData = payload->dup();

					tmp.messages = messages;

					connectionsServToUE[nodeId] = tmp;
				}

				socket.sendTo(payload, L3AddressResolver().resolve(carName.c_str()), destPort);
			}

		}
	}

	selfMsg->setKind(START);
	for (auto & var : names) {
		if (carsSendingTimes[var] <= nextSelfMsg) {
			nextSelfMsg = carsSendingTimes[var];
		}
	}
	scheduleAt(nextSelfMsg, selfMsg);
}

bool TrafficGeneratorServerDL::handleNodeStart(IDoneCallback *doneCallback) {

	socket.setOutputGate(gate("udpOut"));
	localAddress_ = L3AddressResolver().resolve(getParentModule()->getFullName());

	socket.bind(localAddress_, localPort);

	return TrafficGenerator::handleNodeStart(doneCallback);
}
