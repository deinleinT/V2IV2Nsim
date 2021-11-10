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
#include "nr/stack/phy/layer/NRPhyUe.h"
#include "nr/stack/phy/layer/NRPhyGnb.h"

//see inherit class for method description
class NRRlcUm: public LteRlcUm {

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
	virtual void sendDefragmented(cPacket *pkt);
	virtual void handleLowerMessage(cPacket *pkt);
    virtual void handleUpperMessage(cPacket *pkt);

    simsignal_t UEtotalRlcThroughputDlMean;
    simsignal_t UEtotalRlcThroughputUlMean;

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
				if (getNRBinder()->isRemoteCar(ueId, getSystemModule()->par("remoteCarFactor").intValue())) {
					ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUlREMOTE");
				} else {
					ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
				}
			}
		} else {
			//human driven vehicle
			if (!ueTotalRlcThroughputUlInit) {
				ueTotalRlcThroughputUl.setName("UEtotalRlcThroughputUl");
				ueTotalRlcThroughputUlInit = true;
				ueTotalRlcThroughputUlStartTime = NOW;
			}
		}

		//check if remoteDriving Uplink
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
				} else {
					ueTotalRlcThroughputDl.setName("UEtotalRlcThroughputDl");
				}
			}
		} else {
			if (!ueTotalRlcThroughputDlInit) {
				ueTotalRlcThroughputDl.setName("UEtotalRlcThroughputDl");
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
		this->totalRcvdBytesUl += length;
		//double tp = totalRcvdBytesUl / (NOW - getSimulation()->getWarmupPeriod());

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
		this->totalRcvdBytesDl += length;
		//double tp = totalRcvdBytesDl / (NOW - getSimulation()->getWarmupPeriod());

		//to avoid a division through 0
		if(NOW - ueTotalRlcThroughputDlStartTime < 1)
			return;

		double tp = totalRcvdBytesDl / (NOW - ueTotalRlcThroughputDlStartTime);
		ueTotalRlcThroughputDl.record(tp);
		emit(UEtotalRlcThroughputDlMean,tp);
	}

};

