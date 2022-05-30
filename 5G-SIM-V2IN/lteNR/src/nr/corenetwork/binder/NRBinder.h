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
 *
 * Part of 5G-Sim-V2I/N
 *
 *
 */

#pragma once

#include <omnetpp.h>
#include <string>

#include "corenetwork/binder/LteBinder.h"
#include "nr/common/NRCommon.h"
#include "veins/modules/obstacle/ObstacleControl.h"
#include "veins_inet/VeinsInetMobility.h"

using std::string;
using std::vector;

class NRQosCharacteristics;

class NRBinder: public LteBinder {
public:
	NRBinder();
	virtual ~NRBinder();
	virtual MacNodeId getConnectedGnb(MacNodeId ueId);
	virtual void fillUpfGnbMap(MacNodeId gnbId, const std::string &upfName) {
		upfGnbMap[gnbId] = upfName;
	}

	virtual const std::string &getConnectedUpf(MacNodeId gnbId) {
		return upfGnbMap[gnbId];
	}

	virtual bool checkIsNLOS(const inet::Coord &sender, const inet::Coord &receiver, const double hBuilding, bool NlosEvaluationIn3D) {
		veins::ObstacleControl *tmp = check_and_cast<veins::ObstacleControl*>(getSimulation()->getModuleByPath("obstacles"));
		veins::Coord send(sender.x, sender.y, sender.z);
		veins::Coord rec(receiver.x, receiver.y, receiver.z);
		return tmp->isNLOS(send, rec, hBuilding, NlosEvaluationIn3D);
	}

	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  setQueueStatus
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:    used when simplifiedFlowControl is activated (useSimplifiedFlowControl)
	 |                  tracks the macBufferStatus for each queue
	 |
	 |  Parameter:      MacNodeId ueId, unsigned short dir, unsigned short application, bool queueFull
	 |					ueId --> the corresponding ueId
	 |	 	 	 	 	dir --> sending direction, downlink / uplink
	 |	 	 	 	 	application --> the corresponding application
	 |	 	 	 	 	queueFull --> true if the queue is full
	 |
	 |  Return Value:   void
	 |
	 +-----------------------------------------------------------------------------*/
	virtual void setQueueStatus(MacNodeId ueId, unsigned short dir, unsigned short application, bool queueFull) {
		Enter_Method_Silent("queueStatus");

		if (queueStatusMap.find(ueId) != queueStatusMap.end()) {

			bool queueAvailable = false;

			for (auto &var : queueStatusMap[ueId]) {
				if (var.dir == dir && var.application == application) {
					var.queueFull = queueFull;
					var.timestamp = NOW;
					queueAvailable = true;
				}
			}

			if (!queueAvailable) {
				QueueStatus tmp;
				tmp.dir = dir;
				tmp.application = application;
				tmp.timestamp = NOW;
				tmp.queueFull = queueFull;
				queueStatusMap[ueId].push_back(tmp);
			}
		} else {
			QueueStatus tmp;
			tmp.dir = dir;
			tmp.application = application;
			tmp.timestamp = NOW;
			tmp.queueFull = queueFull;
			queueStatusMap[ueId].push_back(tmp);
		}
	}

	virtual std::pair<simtime_t, bool> getQueueStatus(MacNodeId ueId, unsigned short dir, unsigned short application) {

		if (queueStatusMap.find(ueId) == queueStatusMap.end()) {
			//no queue for this ueId available
			return std::make_pair(0, false);
		} else {

			for (auto &var : queueStatusMap[ueId]) {
				if (var.dir == dir && var.application == application) {
					return std::make_pair(var.timestamp, var.queueFull);
				}
			}

			return std::make_pair(0, false);
		}
	}

	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  calculateAttenuationPerCutAndMeter
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:	used if the parameter veinsObstacleShadowing is set to true the ini-file
	 |					returns the calculated attenuation
	 |					the calculation uses the dB values from the config.xml file (db-per-cut / meter)
	 |
	 |  Parameter:      const Coord& senderPos, const Coord& receiverPos
	 |
	 |  Return Value:   double --> the calculated attenuation value
	 |
	 +-----------------------------------------------------------------------------*/
	virtual double calculateAttenuationPerCutAndMeter(const Coord &senderPos, const Coord &receiverPos) const {
		veins::ObstacleControl *tmp = check_and_cast<veins::ObstacleControl*>(getSimulation()->getModuleByPath("obstacles"));
		veins::Coord send(senderPos.x, senderPos.y, senderPos.z);
		veins::Coord rec(receiverPos.x, receiverPos.y, receiverPos.z);
		return tmp->calculateAttenuation(send, rec);
	}

	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  getVehicleSpeed
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:    retrieves the vehicle speed (m/s) from SUMO by veins
	 |		    calls sumo only if the speed was not retrieved at the current simulation time
	 |
	 |  Parameter:      MacNodeId ueId --> the vehicle id (1025 is the start value, represents car[0])
	 |
	 |  Return Value:   double --> the vehicle speed in meter/s
	 |
	 +-----------------------------------------------------------------------------*/
	virtual double getVehicleSpeed(MacNodeId ueId) {
		const std::pair<MacNodeId, simtime_t> key = std::make_pair(ueId, NOW);
		std::map<std::pair<MacNodeId, simtime_t>, double>::const_iterator vehiclesSpeedMapIt = vehiclesSpeedMap.begin();
		if (vehiclesSpeedMap.find(key) != vehiclesSpeedMap.end()) {
			//found the value
			return vehiclesSpeedMap[key];
		} else {
			//not found --> remove old values
			for (; vehiclesSpeedMapIt != vehiclesSpeedMap.end();) {
				if (vehiclesSpeedMapIt->first.first == ueId) {
					vehiclesSpeedMapIt = vehiclesSpeedMap.erase(vehiclesSpeedMapIt);
				} else {
					++vehiclesSpeedMapIt;
				}
			}
			LteMacBase *mac = getMacFromMacNodeId(ueId);
			cModule *car = mac->getParentModule()->getParentModule();
			veins::VeinsInetMobility *mobility = check_and_cast<veins::VeinsInetMobility*>(car->getSubmodule("mobility"));
			double speed = mobility->getSpeed();
			vehiclesSpeedMap[key] = speed;
			return speed;
		}
	}

	virtual void setExchangeBuffersHandoverFlag(bool flag) {
		this->exchangeBuffersOnHandover = flag;
	}

	virtual bool getExchangeOnBuffersHandoverFlag() {
		return exchangeBuffersOnHandover;
	}

	virtual void incrementLosDetected() {
		++losDetected;
	}

	virtual void incrementNlosDetected() {
		++nlosDetected;
	}

	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  isRemoteCar
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:    checks whether the ueId vehicle is a remote car or not
	 |					the remoteCarFactor is taken into account --> e.g., 5 means
	 |	 	 	 	 	every fifth car is a remote vehicle
	 |					if the ini-file param remoteCarByColour is set to true then
	 |					only the vehicle color from sudo is checked --> if red, the corresponding
	 |					vehicle is a remote vehicle, else not
	 |
	 |  Parameter:      unsigned short ueId --> vehicle id, unsigned int remoteCarFactor
	 |
	 |  Return Value:   bool (true if the ueId belongs to a remote vehicle)
	 |
	 +-----------------------------------------------------------------------------*/
	virtual bool isRemoteCar(unsigned short ueId, unsigned int remoteCarFactor) {

		if (remoteCarByColour) {
			veins::TraCIScenarioManager *vinetmanager = check_and_cast<veins::TraCIScenarioManager*>(getSimulation()->getModuleByPath("veinsManager"));
			return vinetmanager->isRemoteVehicle(getBinder()->getMacFromMacNodeId(ueId)->getParentModule()->getParentModule()->getFullName());
		}

		//if remoteCarJustOne is true, only one remote vehicle is added to the simulation
		//its id is determined by the remoteCarFactor (e.g., remoteCarFactor=0 --> the remote car is the one with the ueId 1025)
		if (remoteCarJustOne) {
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


	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  insertVehicleApplicationSummary
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:    used for a more realistic orchestration of multi-applications scenarios
	 |					creates for the ueId a set of bool values based on a uniform random value
	 |					the bool value is used afterwards at the application layer to decide whether
	 |					the corresponding application (v2x, voip, video, data) should be executed or not
	 |
	 |
	 |  Parameter:      MacNodeId ueId --> vehicle Id
	 |
	 |  Return Value:   void
	 |
	 +-----------------------------------------------------------------------------*/
	virtual void insertVehicleApplicationSummary(MacNodeId ueId) {

		if (!(vehicleApplicationMap.find(ueId) != vehicleApplicationMap.end())) {
			VehicleApplicationSummary tmp;
			tmp.v2x = (uniform(0.0, 1.0) <= 0.5) ? true : false;
			tmp.voip = (uniform(0.0, 1.0) <= 0.5) ? true : false;
			tmp.video = (uniform(0.0, 1.0) <= 0.5) ? true : false;
			tmp.data = (uniform(0.0, 1.0) <= 0.5) ? true : false;
			vehicleApplicationMap[ueId] = tmp;
		}

	}

	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  resetVehicleApplicationSummary
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:    resets the values of the vehicleApplicationSummary
	 |					--> the random values for all applications are newly randomized
	 |
	 |  Parameter:      MacNodeId ueId
	 |
	 |  Return Value:   void OK
	 |
	 +-----------------------------------------------------------------------------*/
	virtual void resetVehicleApplicationSummary(MacNodeId ueId) {

		if (vehicleApplicationMap.find(ueId) != vehicleApplicationMap.end()) {
			vehicleApplicationMap.erase(vehicleApplicationMap.find(ueId));
			//call the insert method to reset the values
			insertVehicleApplicationSummary(ueId);
		}
	}

	/*-----------------------------------------------------------------------------+
	 |    F U N C T I O N   I N F O R M A T I O N                                   |
	 +------------------------------------------------------------------------------+
	 |
	 |
	 |  Function Name:  getVehicleApplicationSummaryValue
	 |
	 |  Prototype at:   NRBinder.h
	 |
	 |  Description:    returns a bool value for the corresponding applicationString (e.g., V2X)
	 |					if true is returned, the corresponding application is executed for the
	 |					vehicle with the ueId
	 |
	 |  Parameter:      MacNodeId ueId, std::string applicationString
	 |
	 |  Return Value:   bool
	 |
	 +-----------------------------------------------------------------------------*/
	virtual bool getVehicleApplicationSummaryValue(MacNodeId ueId, std::string applicationString) {

		if (realisticApproachMultiApplication) {

			std::transform(applicationString.begin(), applicationString.end(), applicationString.begin(), ::tolower);

			//call first to ensure that the values are initialized
			insertVehicleApplicationSummary(ueId);

			if (applicationString == "v2x") {
				return vehicleApplicationMap[ueId].v2x;
			} else if (applicationString == "voip") {
				return vehicleApplicationMap[ueId].voip;
			} else if (applicationString == "video") {
				return vehicleApplicationMap[ueId].video;
			} else if (applicationString == "data") {
				return vehicleApplicationMap[ueId].data;
			} else {
				throw cRuntimeError("Error");
			}
		} else {
			return true;
		}
	}

protected:

	struct VehicleApplicationSummary {
		bool v2x = true;
		bool voip = true;
		bool video = true;
		bool data = true;
	};

	//used for simplified flow control
	struct QueueStatus {
		unsigned short dir;		//DL or UL
		unsigned short application;	//application type
		bool queueFull;			//true, if the corresponding queue on mac layer is full
		simtime_t timestamp;	//simtime when the status was updated the last time
	};

	//used for simplified flow control
	std::map<MacNodeId, std::vector<QueueStatus>> queueStatusMap;

	std::map<MacNodeId, VehicleApplicationSummary> vehicleApplicationMap;

	std::map<std::pair<MacNodeId, simtime_t>, double> vehiclesSpeedMap;

	virtual void finish();
	//Gnb Id, the connected UPFs
	std::map<MacNodeId, std::string> upfGnbMap;
	NRQosCharacteristics *qosChar;
	bool exchangeBuffersOnHandover;
	virtual void initialize(int stages);
	unsigned int losDetected;
	unsigned int nlosDetected;

	//ini / ned flags
	bool remoteCarByColour;
	bool remoteCarJustOne;
	bool realisticApproachMultiApplication;
	//
};

