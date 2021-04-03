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

#pragma once

#include <omnetpp.h>
#include "nr/common/NRCommon.h"
#include "inet/applications/udpapp/UDPBasicApp.h"
#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "apps/voip/VoipPacket_m.h"
#include "nr/apps/TrafficGenerator/packet/V2XMessage_m.h"
#include "nr/apps/TrafficGenerator/packet/VideoMessage_m.h"
#include "nr/apps/TrafficGenerator/packet/VoIPMessage_m.h"
#include "nr/apps/TrafficGenerator/packet/DataMessage_m.h"
#include "veins/modules/mobility/traci/TraCIScenarioManager.h"
#include "nr/stack/phy/layer/NRPhyUe.h"

using namespace omnetpp;
using namespace inet;

/**
 * struct for recording the statistics for each
 * datastreams
 */
struct StatReport {

	StatReport() {
	}
	;
	~StatReport() {

	}

	V2XMessage *lastV2X = nullptr;
	VideoMessage *lastVideo = nullptr;
	DataMessage *lastData = nullptr;
	VoIPMessage *lastVoIP = nullptr;

	unsigned int lostPacketsVideo = 0;
	unsigned int lostPacketsV2X = 0;
	unsigned int lostPacketsVoip = 0;
	unsigned int lostPacketsData = 0;

	//total received Packets
	int recPacketsVideo = -1;
	int recPacketsV2X = -1;
	int recPacketsVoip = -1;
	int recPacketsData = -1;

	//packets that arrived NOT in the delayBudget
	unsigned int recPacketsVideoOutBudget = 0;
	unsigned int recPacketsV2XOutBudget = 0;
	unsigned int recPacketsVoipOutBudget = 0;
	unsigned int recPacketsDataOutBudget = 0;

	double reliabilityV2X = 0;
	double reliabilityVideo = 0;
	double reliabiltiyVoIP = 0;
	double reliabilityData = 0;

	double recPacketsDataOutBudget10ms = 0;
	double recPacketsDataOutBudget20ms = 0;
	double recPacketsDataOutBudget50ms = 0;
	double recPacketsDataOutBudget100ms = 0;
	double recPacketsDataOutBudget200ms = 0;
	double recPacketsDataOutBudget500ms = 0;
	double recPacketsDataOutBudget1s = 0;

	double recPacketsV2XOutBudget10ms = 0;
	double recPacketsV2XOutBudget20ms = 0;
	double recPacketsV2XOutBudget50ms = 0;
	double recPacketsV2XOutBudget100ms = 0;
	double recPacketsV2XOutBudget200ms = 0;
	double recPacketsV2XOutBudget500ms = 0;
	double recPacketsV2XOutBudget1s = 0;

	double recPacketsVoipOutBudget10ms = 0;
	double recPacketsVoipOutBudget20ms = 0;
	double recPacketsVoipOutBudget50ms = 0;
	double recPacketsVoipOutBudget100ms = 0;
	double recPacketsVoipOutBudget200ms = 0;
	double recPacketsVoipOutBudget500ms = 0;
	double recPacketsVoipOutBudget1s = 0;

	double recPacketsVideoOutBudget10ms = 0;
	double recPacketsVideoOutBudget20ms = 0;
	double recPacketsVideoOutBudget50ms = 0;
	double recPacketsVideoOutBudget100ms = 0;
	double recPacketsVideoOutBudget200ms = 0;
	double recPacketsVideoOutBudget500ms = 0;
	double recPacketsVideoOutBudget1s = 0;

	double v2vExchangeDelay = 0;
	double v2vExchangeDelayReal = 0;
	simtime_t v2vExchangeDelayFirstMessageCreationTime = 0;
	simtime_t v2vStartExchangeDelayMeasuring = 0;

};

/**
 * struct for recording the connection information
 * of sent/received packets
 */
struct Connection {
	L3Address sendIpAddress;
	std::string sendName;
	std::string destName;
	L3Address destIpAddress; //destinationAddress
	int sendPort;
	int destPort;    // client UDP port
	long videoSize;    // total size of video
	long dataSize;
	long videoBytesLeft;    // bytes left to transmit
	long dataBytesLeft;
	MacNodeId macNodeIdSender;
	OmnetId omnetIdSender;
	MacNodeId macNodeIdDest;
	OmnetId omnetIdDest;
	int messages;

	//for exchange delay
	unsigned int lastReceivedStatusUpdateSN = 0;
	simtime_t lastReceivedStatusUpdateTime = -1;

	bool ackReceived = false;

	simtime_t timeFirstDataPacketArrived = 0;
	simtime_t timeLastDataPacketArrived = 0;

	StatReport statReport;
};

typedef std::map<MacNodeId, Connection> ConnectionsMap;
typedef ConnectionsMap::iterator ConnectionsMapIterator;

/**
 * UDP Application for generating UDP Packets in
 * Downlink (TrafficGeneratorServerDL is the sender, TrafficGeneratorCarDL is the receiver)
 * and in Uplink (TrafficGeneratorCarUL is the sender, TrafficGeneratorServerUL is the receiver)
 */
class TrafficGenerator: public UDPBasicApp {
protected:

	std::string nodeType;
	L3Address localAddress_;
	L3Address destAddress_;
	unsigned int messageLength;
	unsigned int videoSize;
	unsigned int dataSize;
	ConnectionsMap connectionsUEtoServ;
	ConnectionsMap connectionsServToUE;

	bool autoReply;
	bool sendVideoPacket;
	bool sendDataPacket;

	// statistics
	int numberSentPackets = 0;
	int numberReceivedPackets = 0;

	//PacketDelay
	simsignal_t delayVoip;
	simsignal_t delayV2X;
	simsignal_t delayVideo;
	simsignal_t delayData;

	//Packet Delay Variation
	//considers the delay variation of the arrived packet and the last received packet before
	//(lost packets are neglected)
	simsignal_t delayVoipVariation;
	simsignal_t delayV2XVariation;
	simsignal_t delayVideoVariation;
	simsignal_t delayDataVariation;

	//just considers the delay variation of two consecutive packets
	simsignal_t delayVoipVariationReal;
	simsignal_t delayV2XVariationReal;
	simsignal_t delayVideoVariationReal;
	simsignal_t delayDataVariationReal;

//    simsignal_t v2vExchangeDelay;
	simsignal_t v2vExchangeDelayReal;

	//total lost Packets
	unsigned int lostPacketsVideo;
	unsigned int lostPacketsV2X;
	unsigned int lostPacketsVoip;
	unsigned int lostPacketsData;

	//total received Packets
	double recPacketsVideo;
	double recPacketsV2X;
	double recPacketsVoip;
	double recPacketsData;

	//total sent Packets
	unsigned int sentPacketsVideo;
	unsigned int sentPacketsV2X;
	unsigned int sentPacketsVoip;
	unsigned int sentPacketsData;

	//
	cOutVector delayDataTransferFinished;

	//server DL
	unsigned int messages;

	simtime_t delayBudget10ms;
	simtime_t delayBudget20ms;
	simtime_t delayBudget50ms;
	simtime_t delayBudget100ms;
	simtime_t delayBudget200ms;
	simtime_t delayBudget500ms;
	simtime_t delayBudget1s;

	bool considerDatasizeAndMessages;

	unsigned int lastSentStatusUpdateSN;

	std::set<std::string> carsV2X;
	std::set<std::string> carsData;

	double sendInterval;

protected:
	/*
	 * checks whether a car is a remote vehicle or not
	 * @param ueid the car id (>= UE_MIN_ID)
	 * @param remoteCarFactor value which defines how often a remote vehicle is added to the simulation (e.g., 5 meants every fifth car is a remote car)
	 */
	virtual bool isRemoteCar(unsigned short ueId, unsigned int remoteCarFactor) {

		if (getSystemModule()->par("remoteCarByColour")) {
			veins::TraCIScenarioManager *vinetmanager = check_and_cast<veins::TraCIScenarioManager*>(getSimulation()->getModuleByPath("veinsManager"));
			return vinetmanager->isRemoteVehicle(getBinder()->getMacFromMacNodeId(ueId)->getParentModule()->getParentModule()->getFullName());
		}

		//if remoteCarJustOne is true, only one remote vehicle is added to the simulation
		//its id is determined by the remoteCarFactor (e.g., remoteCarFactor=0 --> the remote car is the one with the ueId 1025)
		if (getSystemModule()->par("remoteCarJustOne").boolValue()) {
			if ((UE_MIN_ID + remoteCarFactor) == ueId) {
				return true;
			}
			return false;
		}

		//several remote cars are added to the simulation
		if (ueId % remoteCarFactor == 0) {
			return true;
		}
		return false;
	}

	virtual int numInitStages() const override {
		return NUM_INIT_STAGES;
	}
	virtual void initialize(int stage) override;
	virtual void handleMessageWhenUp(cMessage *msg) override;
	virtual void finish() override;

	virtual void processStop() override;

	virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
	virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;
	virtual void handleNodeCrash() override;

	virtual void recordReliability();

	/*
	 * records the position of the vehicle when a packet loss was detected at application layer
	 */
	virtual void recordVehiclePositionAndLostPackets(MacNodeId nodeId, Direction direction, unsigned int lostPackets) {

		Enter_Method_Silent("recordVehiclePositionAndLostPackets");

		if(!(getSystemModule()->par("recordPositionAndPacketLoss"))){
			return;
		}

		if (direction == DL) {
			//server sends to car, this is called on UE side, get access to its physical layer
			check_and_cast<NRPhyUe*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("phy"))->recordPositionAndLostPackets(lostPackets, direction);
			//
		} else {
			//car sends to server, this is called on server side, but the value is recorded in the corresponding car!
			cModule * module = getMacUe(nodeId);
			check_and_cast<NRPhyUe*>(module->getParentModule()->getSubmodule("phy"))->recordPositionAndLostPackets(lostPackets, direction);
			//
		}
	}

	/**
	 * Calculates the Packet delay variation, two variations are considered:
	 * - the statistic vector delayV2XVariationReal considers all calculated jitter values for consecutive packets (if at least one packet was lost, the calculated jitter is not recorded in that vector)
	 * - the statistic vector delayV2XVariation considers all calculated jitter values (the sequence numbers of the latest and the previous packet are not taken into account)
	 */
	virtual void calcPVV2XUL(MacNodeId nodeId, V2XMessage *pk, bool consecutivePacket) {
		V2XMessage *lastPk = connectionsUEtoServ[nodeId].statReport.lastV2X;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayV2XVariationReal, jitter);

		emit(delayV2XVariation, jitter);

	}
	virtual void calcPVVideoUL(MacNodeId nodeId, VideoMessage *pk, bool consecutivePacket) {
		VideoMessage *lastPk = connectionsUEtoServ[nodeId].statReport.lastVideo;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayVideoVariationReal, jitter);

		emit(delayVideoVariation, jitter);

	}
	virtual void calcPVVoipUL(MacNodeId nodeId, VoIPMessage *pk, bool consecutivePacket) {
		VoIPMessage *lastPk = connectionsUEtoServ[nodeId].statReport.lastVoIP;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayVoipVariationReal, jitter);

		emit(delayVoipVariation, jitter);

	}
	virtual void calcPVDataUL(MacNodeId nodeId, DataMessage *pk, bool consecutivePacket) {
		DataMessage *lastPk = connectionsUEtoServ[nodeId].statReport.lastData;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayDataVariationReal, jitter);

		emit(delayDataVariation, jitter);

	}

	virtual void calcPVV2XDL(MacNodeId nodeId, V2XMessage *pk, bool consecutivePacket) {
		V2XMessage *lastPk = connectionsServToUE[nodeId].statReport.lastV2X;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayV2XVariationReal, jitter);

		emit(delayV2XVariation, jitter);

	}
	virtual void calcPVVideoDL(MacNodeId nodeId, VideoMessage *pk, bool consecutivePacket) {
		VideoMessage *lastPk = connectionsServToUE[nodeId].statReport.lastVideo;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayVideoVariationReal, jitter);

		emit(delayVideoVariation, jitter);

	}
	virtual void calcPVVoipDL(MacNodeId nodeId, VoIPMessage *pk, bool consecutivePacket) {
		VoIPMessage *lastPk = connectionsServToUE[nodeId].statReport.lastVoIP;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayVoipVariationReal, jitter);

		emit(delayVoipVariation, jitter);

	}
	virtual void calcPVDataDL(MacNodeId nodeId, DataMessage *pk, bool consecutivePacket) {
		DataMessage *lastPk = connectionsServToUE[nodeId].statReport.lastData;
		simtime_t jitter = (pk->getArrivalTime() - lastPk->getArrivalTime()) - (pk->getCreationTime() - lastPk->getCreationTime());

		if (consecutivePacket)
			emit(delayDataVariationReal, jitter);

		emit(delayDataVariation, jitter);

	}

public:

	/**
	 * Used for the use case HDMap
	 * set to false, if all expected messages received
	 * @param flag
	 */
	void setDataPacketFlag(bool flag) {
		sendDataPacket = flag;
	}

	TrafficGenerator() {

	}
	virtual ~TrafficGenerator();
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Uplink/////SERVER//////////////////////////////////////////////////////
///
/// This class is the receiver of UDP Packets in the Uplink and records the statistics
///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////

class TrafficGeneratorServerUL: public TrafficGenerator {
public:
	TrafficGeneratorServerUL() {
	}
	virtual ~TrafficGeneratorServerUL() {

		for (auto &var : connectionsUEtoServ) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

		for (auto &var : connectionsServToUE) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

	}

protected:
	virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
	virtual void handleMessageWhenUp(cMessage *msg) override;
	virtual void processPacketServer(cPacket *msg);

};

//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Uplink/////CAR/////////////////////////////////////////////////////////
///
/// This class is the sender of UDP Packets in the Uplink and runs at the application layer at each car.
///
/// ////////////////////////////////////////////////////////////////////////////////////////////////////

class TrafficGeneratorCarUL: public TrafficGenerator {
public:
	TrafficGeneratorCarUL() {
	}
	virtual ~TrafficGeneratorCarUL() {

		for (auto &var : connectionsUEtoServ) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

		for (auto &var : connectionsServToUE) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

	}

protected:
	virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
	virtual void processStart() override;
	virtual void sendPacket(long bytes) override;
	virtual void processSend() override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////DownLink////////SERVER////////////////////////////////////////////////////////
///
/// This class is the sender of UDP Packets in the Downlink. The server sends UDP Packets to each car, after
/// the car appears in the simulation.
///
///////////////////////////////////////////////////////////////////////////////////////////////////////

class TrafficGeneratorServerDL: public TrafficGenerator {
public:
	TrafficGeneratorServerDL() {
	}
	virtual ~TrafficGeneratorServerDL() {
		for (auto &var : connectionsUEtoServ) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

		for (auto &var : connectionsServToUE) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

	}

	/**
	 * is called when a car is added to the simulation
	 * @param name Name of the car that was newly added to the simulation.
	 *
	 */
	void receiveSignal(std::string name) {
		Enter_Method_Silent
		();
		//startTime --> server determines the first time a packet is sent to this car
		simtime_t interval = par("sendInterval").doubleValue();
		simtime_t nextSelfMsgTime = NOW + interval + +uniform(0.0, par("startTimeDL").doubleValue());
		if (considerDatasizeAndMessages) {
			//HD Map
//			if ("car[159]" == name || "car[237]" == name) {
//				nextSelfMsg = NOW + par("startTime").doubleValue();
//			}
		}
		if (names.find(name) == names.end()) {
			carsSendingTimes[name] = nextSelfMsgTime;
		}
		names.insert(name);
		if (selfMsg->isScheduled())
			cancelEvent(selfMsg);
		selfMsg->setKind(START);

		for (auto &var : names) {
			if (carsSendingTimes[var] <= nextSelfMsgTime) {
				nextSelfMsgTime = carsSendingTimes[var];
			}
		}

		//save the random messageLength for each car in RemoteDrivingDL
		if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
			carsByteLengthRemoteDrivingDL[name] = par("messageLength").intValue();
			carsSendingIntervalRemoteDrivingDL[name] = interval;
		}
		//

		scheduleAt(nextSelfMsgTime, selfMsg);
	}

	void deleteNameFromQueuedNames(std::string name) {
		names.erase(name);
		carsSendingTimes.erase(name);
		carsByteLengthRemoteDrivingDL.erase(name);
		carsSendingIntervalRemoteDrivingDL.erase(name);
	}

protected:

	std::set<std::string> names;
	std::map<std::string, simtime_t> carsSendingTimes;
	std::map<std::string, unsigned int> carsByteLengthRemoteDrivingDL;
	std::map<std::string, simtime_t> carsSendingIntervalRemoteDrivingDL;

	virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
	virtual void sendPacket(long bytes) override;
	virtual void handleMessageWhenUp(cMessage *msg) override;
	virtual void initialize(int stage) override;

};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////DownLink////////CAR///////////////////////////////////////////////////////////
///
/// This class is the receiver of UDP Packets in the Downlink and runs at the application layer at each car.
/// Records the statistic of recieved packets.
///
////////////////////////////////////////////////////////////////////////////////////////////////////

class Listener;
class TrafficGeneratorCarDL: public TrafficGenerator {
public:
	TrafficGeneratorCarDL() {
	}
	virtual ~TrafficGeneratorCarDL() {
		for (auto &var : connectionsUEtoServ) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}

		for (auto &var : connectionsServToUE) {
			if (var.second.statReport.lastData != nullptr)
				delete var.second.statReport.lastData;
			if (var.second.statReport.lastVoIP != nullptr)
				delete var.second.statReport.lastVoIP;
			if (var.second.statReport.lastV2X != nullptr)
				delete var.second.statReport.lastV2X;
			if (var.second.statReport.lastVideo != nullptr)
				delete var.second.statReport.lastVideo;
		}
	}

protected:

	Listener *listener0;    		// for each server one listener
	Listener *listener1;
	Listener *listener2;
	Listener *listener3;
	simsignal_t carNameSignal;
	virtual void initialize(int stage) override;
	virtual void handleMessageWhenUp(cMessage *msg) override;
	virtual void processPacket(cPacket *msg) override;
	virtual bool handleNodeStart(IDoneCallback *doneCallback) override;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////
/// this listener is used to inform the server in the Downlink case about newly added cars to the
/// simulation
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class Listener: public cListener {
protected:
	TrafficGeneratorServerDL *module;

public:
	Listener() {
		module = nullptr;
	}
	Listener(TrafficGeneratorServerDL *module) {
		this->module = module;
	}
	~Listener() {
	}
	virtual void receiveSignal(cComponent *source, simsignal_t signalID, const char *s, cObject *details) override {
		//send packets to new car
		module->receiveSignal(std::string(s));
	}
};
