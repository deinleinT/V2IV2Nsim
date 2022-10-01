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
#include "stack/rlc/um/LteRlcUm.h"
#include "nr/stack/phy/layer/NRPhyUE.h"
#include "nr/stack/phy/layer/NRPhyGnb.h"
#include "nr/stack/sdap/utils/QosHandler.h"

//see inherit class for method description
class NRRlcUm: public LteRlcUm {

public:
    NRRlcUm()
    {
    }
    virtual ~NRRlcUm()
    {
    }

    virtual QosHandler * getQoSHandler(){
        Enter_Method_Silent();
        return check_and_cast<QosHandler*>(getParentModule()->getParentModule()->getSubmodule("qosHandler"));;
    }

protected:
    cOutVector totalRlcThroughputUl;
    double totalRcvdBytesUl;
    double totalRcvdBytesDl;
    cOutVector totalRlcThroughputDl;

    double numberOfConnectedUes;
    cOutVector cellConnectedUes;

    simsignal_t UEtotalRlcThroughputDlMean;
    simsignal_t UEtotalRlcThroughputUlMean;

    QosHandler * qosHandler;
    unsigned int throughputInBitsPerSecondDL;
    unsigned int throughputInBitsPerSecondUL;

    cOutVector UERlcThroughputPerSecondUl;
    cOutVector UERlcThroughputPerSecondDl;

    cMessage * throughputTimer;
    double throughputInterval;
    std::string nodeType;

    virtual void initialize(int stage) override;
    virtual void finish() override
    {
        if (throughputTimer) {
            cancelEvent(throughputTimer);
            delete throughputTimer;
            throughputTimer = nullptr;
        }
    }
    virtual void handleMessage(cMessage *msg);
    virtual void sendDefragmented(cPacket *pkt);
    virtual void handleLowerMessage(cPacket *pkt);
    virtual void handleUpperMessage(cPacket *pkt);

	//important for initializing the UEThroughputvector in a correct way
	virtual void checkRemoteCarStatus() {
		//check if remoteDriving Uplink
		if (getSimulation()->getSystemModule()->par("remoteDrivingUL")) {
			//check if the vector was already initialized
			if (!ueTotalRlcThroughputUlInit) {
				NRMacUe *mac = check_and_cast<NRMacUe*>(getParentModule()->getParentModule()->getSubmodule("mac"));
				MacNodeId ueId = mac->getMacNodeId();
				ueTotalRlcThroughputUlInit = true;
				//start time when the first packet was transmitted
				ueTotalRlcThroughputUlStartTime = NOW;
				//check if this vehicle is a remote vehicle or not
				if (getNRBinder()->isRemoteCar(ueId, getSimulation()->getSystemModule()->par("remoteCarFactor").intValue())) {
					ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUlREMOTE");
					UERlcThroughputPerSecondUl.setName("UERlcThroughputPerSecondUlREMOTE");
				} else {
					ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
					UERlcThroughputPerSecondUl.setName("UERlcThroughputPerSecondUl");
				}
			}
		} else {
			//human driven vehicle
			if (!ueTotalRlcThroughputUlInit) {
				ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
				UERlcThroughputPerSecondUl.setName("UERlcThroughputPerSecondUl");
				ueTotalRlcThroughputUlInit = true;
				ueTotalRlcThroughputUlStartTime = NOW;
			}
		}

		//check if remoteDriving Downlink
		if (getSimulation()->getSystemModule()->par("remoteDrivingDL")) {
			//check if the vector was already initialized
			if (!ueTotalRlcThroughputDlInit) {
				NRMacUe *mac = check_and_cast<NRMacUe*>(getParentModule()->getParentModule()->getSubmodule("mac"));
				MacNodeId ueId = mac->getMacNodeId();
				ueTotalRlcThroughputDlInit = true;
				//start time when the first packet was transmitted
				ueTotalRlcThroughputDlStartTime = NOW;
				if (getNRBinder()->isRemoteCar(ueId, getSystemModule()->par("remoteCarFactor").intValue())) {
					ueTotalRlcThroughputDl.setName("UEtotalRlcThroughputDlREMOTE");
					UERlcThroughputPerSecondDl.setName("UERlcThroughputPerSecondDlREMOTE");
				} else {
					ueTotalRlcThroughputDl.setName("UEtotalRlcThroughputDl");
					UERlcThroughputPerSecondDl.setName("UERlcThroughputPerSecondDl");
				}
			}
		} else {
			if (!ueTotalRlcThroughputDlInit) {
				ueTotalRlcThroughputDl.setName("UEtotalRlcThroughputDl");
				UERlcThroughputPerSecondDl.setName("UERlcThroughputPerSecondDl");
				ueTotalRlcThroughputDlInit = true;
				ueTotalRlcThroughputDlStartTime = NOW;
			}
		}
	}

public:
	//length --> packet size
	//tp is the throughput per second
	virtual void recordUETotalRlcThroughputUl(double length) {
		Enter_Method_Silent();
		checkRemoteCarStatus();
		this->totalRcvdBytesUl += (length * 8);

		throughputInBitsPerSecondUL = throughputInBitsPerSecondUL + (length * 8);

		//to avoid a division through 0
		if(NOW - ueTotalRlcThroughputUlStartTime < 1)
			return;
		double tp = totalRcvdBytesUl / (NOW - ueTotalRlcThroughputUlStartTime);
		ueTotalRlcThroughputUl.record(tp);
		emit(UEtotalRlcThroughputUlMean,tp);
	}

	//length --> packet size
	//tp is the throughput per second
	virtual void recordUETotalRlcThroughputDl(double length) {
		Enter_Method_Silent();
		checkRemoteCarStatus();
		this->totalRcvdBytesDl += (length * 8);

		throughputInBitsPerSecondDL = throughputInBitsPerSecondDL + (length * 8);

		//to avoid a division through 0
		if(NOW - ueTotalRlcThroughputDlStartTime < 1)
			return;

		double tp = totalRcvdBytesDl / (NOW - ueTotalRlcThroughputDlStartTime);
		ueTotalRlcThroughputDl.record(tp);
		emit(UEtotalRlcThroughputDlMean,tp);
	}

};

