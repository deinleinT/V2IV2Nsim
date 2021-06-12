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

#include "common/binder/Binder.h"
#include "nr/common/NRCommon.h"
#include "veins/modules/obstacle/ObstacleControl.h"
#include "veins_inet/VeinsInetMobility.h"

using std::string;
using std::vector;

class NRQosCharacteristics;

class NRBinder: public Binder {
public:
	NRBinder();
	virtual ~NRBinder();
	virtual MacNodeId getConnectedGnb(MacNodeId ueId);
	virtual void fillUpfGnbMap(MacNodeId gnbId, std::string upfName) {
		upfGnbMap[gnbId] = upfName;
	}

    inet::Ipv4Address getIpAddressByMacNodeId(MacNodeId nodeId) {
        Enter_Method_Silent
        ("getIPAddressByMacNodeId");

        //std::cout << "NRBinder::getIPAddressByMacNodeId start at " << simTime().dbl() << std::endl;

        for (auto &var : macNodeIdToIPAddress_) {
            if (var.second == nodeId) {
                //std::cout << "LteBinder::getIPAddressByMacNodeId end at " << simTime().dbl() << std::endl;
                return var.first;
            }
        }

        //std::cout << "NRBinder::getIPAddressByMacNodeId end at " << simTime().dbl() << std::endl;

        return inet::Ipv4Address();
    }

	virtual std::string getConnectedUpf(MacNodeId gnbId) {
		return upfGnbMap[gnbId];
	}

	virtual bool checkIsNLOS(const inet::Coord &sender, const inet::Coord &receiver, const double hBuilding, bool NlosEvaluationIn3D) {
		veins::ObstacleControl *tmp = check_and_cast<veins::ObstacleControl*>(getSimulation()->getModuleByPath("obstacles"));
		veins::Coord send(sender.x, sender.y, sender.z);
		veins::Coord rec(receiver.x, receiver.y, receiver.z);
		return tmp->isNLOS(send, rec, hBuilding, NlosEvaluationIn3D);
	}

	virtual double calculateAttenuationPerCutAndMeter(const inet::Coord &senderPos, const inet::Coord &receiverPos) const {
		veins::ObstacleControl *tmp = check_and_cast<veins::ObstacleControl*>(getSimulation()->getModuleByPath("obstacles"));
		veins::Coord send(senderPos.x, senderPos.y, senderPos.z);
		veins::Coord rec(receiverPos.x, receiverPos.y, receiverPos.z);
		return tmp->calculateAttenuation(send, rec);
	}

    virtual double getVehicleSpeed(MacNodeId ueId){
        const std::pair<MacNodeId,simtime_t> key = std::make_pair(ueId, NOW);
        std::map<std::pair<MacNodeId,simtime_t>, double>::const_iterator vehiclesSpeedMapIt = vehiclesSpeedMap.begin();
        if(vehiclesSpeedMap.find(key) != vehiclesSpeedMap.end()){
            //found the value
            return vehiclesSpeedMap[key];
        }else{
            //not found --> remove old values
            for (; vehiclesSpeedMapIt != vehiclesSpeedMap.end();) {
                if (vehiclesSpeedMapIt->first.first == ueId) {
                    vehiclesSpeedMapIt = vehiclesSpeedMap.erase(vehiclesSpeedMapIt);
                }else{
                    ++vehiclesSpeedMapIt;
                }
            }
            LteMacBase *mac = getMacFromMacNodeId(ueId);
            cModule * car = mac->getParentModule()->getParentModule();
            veins::VeinsInetMobility *mobility = check_and_cast<veins::VeinsInetMobility*>(car->getSubmodule("mobility"));
            double speed = mobility->getVehicleCommandInterface()->getSpeed();
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

	//returns true if the ue is not connected to a base station
    //only considered if the flag useSINRThreshold
    virtual bool isNotConnected(MacNodeId ueId) {

        ASSERT(ueId >= UE_MIN_ID && ueId <= UE_MAX_ID);

        for (auto var : ueNotConnectedList_) {
            if (var == ueId) {
                return true;
            }
        }

        return false;
    }

    virtual void insertUeToNotConnectedList(MacNodeId ueId) {
        ueNotConnectedList_.insert(ueId);
    }

    virtual void deleteFromUeNotConnectedList(MacNodeId ueId) {
        ueNotConnectedList_.erase(ueId);
    }

protected:
    std::map<std::pair<MacNodeId,simtime_t>, double> vehiclesSpeedMap;

	virtual void finish();
	//Gnb Id, the connected UPFs
	std::map<MacNodeId, std::string> upfGnbMap;
	NRQosCharacteristics *qosChar;
	bool exchangeBuffersOnHandover;
	virtual void initialize(int stages);
	unsigned int losDetected;
	unsigned int nlosDetected;

    // list of all UEs, which are NOT connected
    std::set<MacNodeId> ueNotConnectedList_;
};

