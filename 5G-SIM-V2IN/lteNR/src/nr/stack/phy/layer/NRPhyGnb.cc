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

#include "nr/stack/phy/layer/NRPhyGnb.h"

Define_Module(NRPhyGnb);

NRPhyGnb::NRPhyGnb() :
        LtePhyEnb() {

}

NRPhyGnb::~NRPhyGnb() {

}

void NRPhyGnb::initialize(int stage) {

	cSimpleModule::initialize(stage);

	//ChannelAccess::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		cc = getChannelControl();
		hostModule = inet::findContainingNode(this);
		myRadioRef = nullptr;

		positionUpdateArrived = false;

		// subscribe to the correct mobility module

		if (hostModule->findSubmodule("mobility") != -1) {
			// register to get a notification when position changes
			hostModule->subscribe(inet::IMobility::mobilityStateChangedSignal, this);
		}
	}
	else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
		if (!positionUpdateArrived && hostModule->isSubscribed(inet::IMobility::mobilityStateChangedSignal, this)) {
			// ...else, get the initial position from the display string
			radioPos.x = parseInt(hostModule->getDisplayString().getTagArg("p", 0), -1);
			radioPos.y = parseInt(hostModule->getDisplayString().getTagArg("p", 1), -1);

			if (radioPos.x == -1 || radioPos.y == -1)
				error("The coordinates of '%s' host are invalid. Please set coordinates in "
						"'@display' attribute, or configure Mobility for this host.", hostModule->getFullPath().c_str());

			const char *s = hostModule->getDisplayString().getTagArg("p", 2);
			if (s && *s)
				error("The coordinates of '%s' host are invalid. Please remove automatic arrangement"
						" (3rd argument of 'p' tag)"
						" from '@display' attribute, or configure Mobility for this host.", hostModule->getFullPath().c_str());
		}
		myRadioRef = cc->registerRadio(this);
		cc->setRadioPosition(myRadioRef, radioPos);
	}

	//LtePhyBase::initialize(stage);

	if (stage == inet::INITSTAGE_LOCAL) {
		binder_ = getBinder();
		cellInfo_ = nullptr;
		// get gate ids
		upperGateIn_ = findGate("upperGateIn");
		upperGateOut_ = findGate("upperGateOut");
		radioInGate_ = findGate("radioIn");

		// Initialize and watch statistics
		numAirFrameReceived_ = numAirFrameNotReceived_ = 0;
		ueTxPower_ = par("ueTxPower");
		eNodeBtxPower_ = par("eNodeBTxPower");
		microTxPower_ = par("microTxPower");

		//carrierFrequency_ = 2.1e+9;
		WATCH(numAirFrameReceived_);
		WATCH(numAirFrameNotReceived_);

		multicastD2DRange_ = par("multicastD2DRange");
		enableMulticastD2DRangeCheck_ = par("enableMulticastD2DRangeCheck");
	}
	else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
		initializeChannelModel();
	}

	//LtePhyEnb::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		// get local id
		nodeId_ = getAncestorPar("macNodeId");

		cellInfo_ = getCellInfo(nodeId_);
		if (cellInfo_ != NULL) {
			cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
			das_ = new DasFilter(this, binder_, cellInfo_->getRemoteAntennaSet(), 0);
		}
		isNr_ = false;

		nodeType_ = GNODEB;
	}
	else if (stage == 1) {
		initializeFeedbackComputation();

		//check eNb type and set TX power
		if (cellInfo_->getEnbType() == MICRO_ENB)
			txPower_ = microTxPower_;
		else
			txPower_ = eNodeBtxPower_;

		// set TX direction
		std::string txDir = par("txDirection");
		if (txDir.compare(txDirections[OMNI].txDirectionName) == 0) {
			txDirection_ = OMNI;
		}
		else   // ANISOTROPIC
		{
			txDirection_ = ANISOTROPIC;

			// set TX angle
			txAngle_ = par("txAngle");
		}

		bdcUpdateInterval_ = cellInfo_->par("broadcastMessageInterval");
		if (bdcUpdateInterval_ != 0 && par("enableHandover").boolValue()) {
			// self message provoking the generation of a broadcast message
			bdcStarter_ = new cMessage("bdcStarter");
			scheduleAt(NOW, bdcStarter_);
		}
	}

	//local
	if (stage == inet::INITSTAGE_APPLICATION_LAYER) {

		averageTxPower = registerSignal("averageTxPower");
		attenuation = registerSignal("attenuation");
		snir = registerSignal("snir");
		d2d = registerSignal("d2d");
		d3d = registerSignal("d3d");
		totalPer = registerSignal("totalPer");
		bler = registerSignal("bler");
		speed = registerSignal("speed");

		emit(averageTxPower, txPower_);
		errorCount = 0;

		qosHandler = check_and_cast<QosHandlerGNB*>(getParentModule()->getSubmodule("qosHandler"));

	}
}

void NRPhyGnb::initializeChannelModel()
{
    std::string moduleName = "nrChannelModel";
    primaryChannelModel_ = check_and_cast<NRRealisticChannelModel*>(getParentModule()->getSubmodule(moduleName.c_str(),0));
    primaryChannelModel_->setPhy(this);
    double carrierFreq = primaryChannelModel_->getCarrierFrequency();
    unsigned int numerologyIndex = primaryChannelModel_->getNumerologyIndex();
    channelModel_[carrierFreq] = primaryChannelModel_;

    if (nodeType_ == UE)
        binder_->registerCarrierUe(carrierFreq, numerologyIndex, nodeId_);

    int vectSize = primaryChannelModel_->getVectorSize();
    NRRealisticChannelModel* chanModel = NULL;
    for (int index=1; index<vectSize; index++)
    {
        chanModel = check_and_cast<NRRealisticChannelModel*>(getParentModule()->getSubmodule(moduleName.c_str(),index));
        chanModel->setPhy(this);
        carrierFreq = chanModel->getCarrierFrequency();
        numerologyIndex = chanModel->getNumerologyIndex();
        channelModel_[carrierFreq] = chanModel;
        if (nodeType_ == UE)
            binder_->registerCarrierUe(carrierFreq, numerologyIndex, nodeId_);
    }
}

void NRPhyGnb::recordAttenuation(const double & att) {
    emit(attenuation, att);
}

void NRPhyGnb::recordSNIR(const double & snirVal) {
    emit(snir, snirVal);
}

void NRPhyGnb::recordDistance3d(const double & d3dVal) {
    emit(d3d, d3dVal);
}

void NRPhyGnb::recordDistance2d(const double & d2dVal) {
    emit(d2d, d2dVal);
}

void NRPhyGnb::recordTotalPer(const double & totalPerVal) {
    emit(totalPer, totalPerVal);
}

void NRPhyGnb::recordBler(const double & blerVal) {
    emit(bler, blerVal);
}

void NRPhyGnb::recordSpeed(const double & speedVal) {
    emit(speed, speedVal);
}

void NRPhyGnb::errorDetected() {
    ++errorCount;
}

void NRPhyGnb::finish() {
    recordScalar("#errorCounts", errorCount);
}
