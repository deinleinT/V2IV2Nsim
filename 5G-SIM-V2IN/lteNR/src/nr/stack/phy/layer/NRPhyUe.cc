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

#include "nr/stack/phy/layer/NRPhyUe.h"
#include "veins/base/modules/BaseWorldUtility.h"
#include "nr/apps/TrafficGenerator/TrafficGenerator.h"

Define_Module(NRPhyUe);

NRPhyUe::NRPhyUe() :
		LtePhyUe() {

}
NRPhyUe::~NRPhyUe() {

}

void NRPhyUe::initialize(int stage) {
	LtePhyBase::initialize(stage);

	//std::cout << "NRPhyUe::init start at " << simTime().dbl() << std::endl;
	if (stage == inet::INITSTAGE_LOCAL) {
		nodeType_ = UE;
		useBattery_ = false;  // disabled
		enableHandover_ = par("enableHandover");
		handoverLatency_ = par("handoverLatency").doubleValue();
		dynamicCellAssociation_ = par("dynamicCellAssociation");
		currentMasterRssi_ = 0;
		candidateMasterRssi_ = 0;
		candidateMasterId_ = 0;
		hysteresisTh_ = 0;
		hysteresisFactor_ = 10;
		//handoverDelta_ = 0.00001;
		handoverDelta_ = par("handoverDelta").doubleValue();

		dasRssiThreshold_ = par("dasRssiThreshold").doubleValue();

		das_ = new DasFilter(this, binder_, NULL, dasRssiThreshold_);

		servingCell_ = registerSignal("servingCell");
		averageCqiDl_ = registerSignal("averageCqiDl");
		averageCqiUl_ = registerSignal("averageCqiUl");

		averageTxPower = registerSignal("averageTxPower");
		attenuation = registerSignal("attenuation");
		snir = registerSignal("snir");
		totalPer = registerSignal("totalPer");
		d2d = registerSignal("d2d");
		d3d = registerSignal("d3d");
		bler = registerSignal("bler");
		speed = registerSignal("speed");

		emit(averageTxPower, txPower_);
		errorCount = 0;

		if (!hasListeners(averageCqiDl_))
			error("no phy listeners");

		WATCH(nodeType_);
		WATCH(masterId_);
		WATCH(candidateMasterId_);
		WATCH(dasRssiThreshold_);
		WATCH(currentMasterRssi_);
		WATCH(candidateMasterRssi_);
		WATCH(hysteresisTh_);
		WATCH(hysteresisFactor_);
		WATCH(handoverDelta_);
		WATCH(das_);
	} else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT) {

		txPower_ = ueTxPower_;

		lastFeedback_ = 0;

		handoverStarter_ = new cMessage("handoverStarter");

		mac_ = check_and_cast<LteMacUe *>(getParentModule()-> // nic
		getSubmodule("mac"));
		rlcUm_ = check_and_cast<LteRlcUm *>(getParentModule()-> // nic
		getSubmodule("rlc")->getSubmodule("um"));
	} else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
		// find the best candidate master cell
		if (dynamicCellAssociation_) {
			// this is a fictitious frame that needs to compute the SINR
			LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
			UserControlInfo *cInfo = new UserControlInfo();

			// get the list of all eNodeBs in the network
			std::vector<EnbInfo*>* enbList = binder_->getEnbList();
			std::vector<EnbInfo*>::iterator it = enbList->begin();

			for (; it != enbList->end(); ++it) {
				MacNodeId cellId = (*it)->id;
				LtePhyBase* cellPhy = check_and_cast<LtePhyBase*>((*it)->eNodeB->getSubmodule("lteNic")->getSubmodule("phy"));
				double cellTxPower = cellPhy->getTxPwr();
				Coord cellPos = cellPhy->getCoord();

				// build a control info
				cInfo->setSourceId(cellId);
				cInfo->setTxPower(cellTxPower);
				cInfo->setCoord(cellPos);
				cInfo->setDirection(DL);
//				cInfo->setFrameType(FEEDBACKPKT);

				// get RSSI from the eNB
				std::vector<double>::iterator it;
				double rssi = 0;
				std::vector<double> rssiV = channelModel_->getSINR(frame, cInfo);
				for (it = rssiV.begin(); it != rssiV.end(); ++it)
					rssi += *it;
				rssi /= rssiV.size();   // compute the mean over all RBs

				//EV << "NRPhyUe::initialize - RSSI from eNodeB " << cellId << ": " << rssi << " dB (current candidate eNodeB " << candidateMasterId_ << ": " << candidateMasterRssi_ << " dB" << endl;

				if (rssi > candidateMasterRssi_ || candidateMasterId_ == 0) {
					candidateMasterId_ = cellId;
					candidateMasterRssi_ = rssi;
				}

			}
			delete cInfo;
			delete frame;

			// set serving cell
			masterId_ = candidateMasterId_;

			getAncestorPar("masterId").setIntValue(masterId_);

			currentMasterRssi_ = candidateMasterRssi_;
			updateHysteresisTh(candidateMasterRssi_);
		} else {
			// get serving cell from configuration
			masterId_ = getAncestorPar("masterId");
			candidateMasterId_ = masterId_;
		}
		//EV << "NRPhyUe::initialize - Attaching to eNodeB " << masterId_ << endl;

		das_->setMasterRuSet(masterId_);
		emit(servingCell_, (long) masterId_);
	} else if (stage == inet::INITSTAGE_NETWORK_LAYER_3) {

		// get local id
		nodeId_ = getAncestorPar("macNodeId");

		//EV << "Local MacNodeId: " << nodeId_ << endl;

		// get cellInfo at this stage because the next hop of the node is registered in the IP2Lte module at the INITSTAGE_NETWORK_LAYER
		cellInfo_ = getCellInfo(nodeId_);
		int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
		cellInfo_->lambdaInit(nodeId_, index);
		cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));

		qosHandler = check_and_cast<QosHandlerUE*>(getParentModule()->getSubmodule("qosHandler"));
		exchangeBuffersOnHandover_ = getNRBinder()->getExchangeOnBuffersHandoverFlag();
	}
	//std::cout << "NRPhyUe::init end at " << simTime().dbl() << std::endl;
}

void NRPhyUe::recordAttenuation(const double & att) {
	emit(attenuation, att);
	//std::cout << "Test PhyUE " << att << std::endl;
}

void NRPhyUe::recordSNIR(const double & snirVal) {
	emit(snir, snirVal);
	//std::cout << "Test PhyUE SNIR" << snir << std::endl;
}

void NRPhyUe::recordDistance3d(const double & d3dVal) {
	emit(d3d, d3dVal);
}

void NRPhyUe::recordDistance2d(const double & d2dVal) {
	emit(d2d, d2dVal);
}

void NRPhyUe::handleMessage(cMessage *msg) {

	//std::cout << "NRPhyUe handleMessage start at " << simTime().dbl() << std::endl;

//	if (strcmp(msg->getName(), "RRC") == 0) {
//		if (msg->getArrivalGate()->getId() == radioInGate_) {
//			//RRC Message from a GNODEB
//			//TODO
//		}
//
//		// message from stack
//		else if (msg->getArrivalGate()->getId() == upperGateIn_) {
//			//send RRC Message to a GNODEB
//			//TODO
//		}
//	}

	LtePhyUe::handleMessage(msg);

	//std::cout << "NRPhyUe handleMessage end at " << simTime().dbl() << std::endl;
}

void NRPhyUe::finish() {
	recordScalar("#errorCounts", errorCount);
	LtePhyUe::finish();
}

void NRPhyUe::recordBler(const double & blerVal) {
	emit(bler, blerVal);
}

void NRPhyUe::recordSpeed(const double & speedVal) {
	emit(speed, speedVal);
}

void NRPhyUe::errorDetected() {
	++errorCount;
}

void NRPhyUe::recordTotalPer(const double & totalPerVal) {
	emit(totalPer, totalPerVal);
}

//when UE is leaving the simulation, for simplified Handover
void NRPhyUe::deleteOldBuffers(MacNodeId masterId) {

	//std::cout << "NRPhyUe deleteOldBuffers start at " << simTime().dbl() << std::endl;

	OmnetId masterOmnetId = binder_->getOmnetId(masterId);

	channelModel_->resetOnHandover(nodeId_, masterId);

	LtePhyBase *masterPhy = check_and_cast<LtePhyBase *>(getSimulation()->getModule(masterOmnetId)->getSubmodule("lteNic")->getSubmodule("phy"));
	masterPhy->getChannelModel()->resetOnHandover(nodeId_, masterId);

	/* Delete Mac Buffers */

	// delete macBuffer[nodeId_] at old master
	LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(getSimulation()->getModule(masterOmnetId)->getSubmodule("lteNic")->getSubmodule("mac"));
	masterMac->deleteQueues(nodeId_);
	//qosHandler GNB
	masterMac->deleteNodeFromQosHandler(nodeId_);
	masterMac->deleteOnHandoverRtxSignalised(nodeId_);
	masterMac->deleteFromRtxMap(nodeId_);

	// delete queues for master at this ue
	mac_->deleteQueues(masterId_);
	//qosHandler UE
	mac_->deleteNodeFromQosHandler(masterId_);

	//added, Thomas Deinlein
	NRMacUe * macReal = check_and_cast<NRMacUe*>(mac_);
	macReal->resetScheduleList();
	macReal->getRtxSignalised() = false;
	macReal->resetSchedulingGrantMap();

	/////////////////////////////////////////////////////////////////////////////////

	/* Delete Rlc UM Buffers */
	LteRlcUm * masterRlcUmRealistic = check_and_cast<LteRlcUm *>(getSimulation()->getModule(masterOmnetId)->getSubmodule("lteNic")->getSubmodule("rlc")->getSubmodule("um"));
	masterRlcUmRealistic->deleteQueues(nodeId_);

	// delete queues for master at this ue
	rlcUm_->deleteQueues(nodeId_);

	////////////////////////////////////////////////////////////////////////////////

	//pdcp_rrc --> connectionTable reset, masterId == oldENB --> delete entry
	// nodeId_ --> delete whole
	NRPdcpRrcGnb *masterPdcp = check_and_cast<NRPdcpRrcGnb *>(getSimulation()->getModule(masterOmnetId)->getSubmodule("lteNic")->getSubmodule("pdcpRrc"));
	masterPdcp->resetConnectionTable(masterId, nodeId_);

	NRPdcpRrcUe * pdcp = check_and_cast<NRPdcpRrcUe *>(getParentModule()-> // nic
	getSubmodule("pdcpRrc"));
	pdcp->resetConnectionTable(masterId, nodeId_);

	//SDAP///////////////////////////////////////////////////////////////////////////
	NRsdapUE * sdapUe = check_and_cast<NRsdapUE *>(getParentModule()-> // nic
	getSubmodule("sdap"));

	sdapUe->deleteEntities(masterId);

	NRsdapGNB *sdapGNB = check_and_cast<NRsdapGNB *>(getSimulation()->getModule(masterOmnetId)->getSubmodule("lteNic")->getSubmodule("sdap"));

	sdapGNB->deleteEntities(nodeId_);

	//std::cout << "NRPhyUe deleteOldBuffers end at " << simTime().dbl() << std::endl;
}

//for buffer exchange on handover
void NRPhyUe::exchangeBuffersOnHandover(MacNodeId oldMasterId, MacNodeId newMasterId) {

	//std::cout << "NRPhyUe deleteOldBuffers start at " << simTime().dbl() << std::endl;

	OmnetId oldMasterOmnetId = binder_->getOmnetId(oldMasterId);
	OmnetId newMasterOmnetId = binder_->getOmnetId(newMasterId);

	channelModel_->resetOnHandover(nodeId_, oldMasterId);

	LtePhyBase *masterPhy = check_and_cast<LtePhyBase *>(getSimulation()->getModule(oldMasterId)->getSubmodule("lteNic")->getSubmodule("phy"));
	masterPhy->getChannelModel()->resetOnHandover(nodeId_, oldMasterId);

	/////////////////////////////////////////MAC START////////////////////////////////////
	//exchange macBuffers
	LteMacEnb *oldMasterMac = check_and_cast<LteMacEnb *>(getSimulation()->getModule(oldMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("mac"));
	LteMacEnb *newMasterMac = check_and_cast<LteMacEnb *>(getSimulation()->getModule(newMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("mac"));

	//
	std::vector<QosInfo> nodeQosInfos = oldMasterMac->getQosHandler()->getAllQosInfos(nodeId_);
	// change the oldMasterId in the QosInfos to the new One
	for (auto & var : nodeQosInfos) {
		if (var.senderNodeId == oldMasterId) {
			var.senderNodeId = newMasterId;
		}
		if (var.destNodeId == oldMasterId) {
			var.destNodeId = newMasterId;
//            newMasterMac->getSchedulerEnbDl()->backlog(idToMacCid(nodeId_, var.lcid));
		}
	}
	//
	oldMasterMac->exchangeQueues(nodeId_, newMasterMac, nodeQosInfos, oldMasterId, newMasterId);    //delete HARQ but copy the rest
	//
	oldMasterMac->exchangeQosInfosFromQosHandler(nodeId_, nodeQosInfos, newMasterMac, oldMasterId, newMasterId);    //TEST OK
	mac_->changeMasterId(oldMasterId, newMasterId);    //TEST OK

	////////////////////////////////////////////////RLC/////////////////////////////////
	/* Rlc UM Buffers */

	cModule *newMasterRlc = check_and_cast<cModule *>(getSimulation()->getModule(newMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("rlc"));

	LteRlcUm * oldMasterRlcUmRealistic = check_and_cast<LteRlcUm *>(getSimulation()->getModule(oldMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("rlc")->getSubmodule("um"));
	LteRlcUm * newMasterRlcUmRealistic = check_and_cast<LteRlcUm *>(getSimulation()->getModule(newMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("rlc")->getSubmodule("um"));
	LteRlcUm * ueRlcUmRealistic = check_and_cast<LteRlcUm *>(rlcUm_);

	oldMasterRlcUmRealistic->exchangeEntities(nodeId_, newMasterRlcUmRealistic, newMasterId, oldMasterId);    //txEntities rxEntities --> TODO

	ueRlcUmRealistic->modifyEntitiesUe(nodeId_, newMasterRlcUmRealistic, newMasterId, oldMasterId);

	//////////////////////////////////PDCP//////////////////////////////////////////////

	NRPdcpRrcGnb *oldMasterPdcp = check_and_cast<NRPdcpRrcGnb *>(getSimulation()->getModule(oldMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("pdcpRrc"));

	NRPdcpRrcGnb *newMasterPdcp = check_and_cast<NRPdcpRrcGnb *>(getSimulation()->getModule(newMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("pdcpRrc"));

	oldMasterPdcp->exchangeConnection(nodeId_, oldMasterId, newMasterId, oldMasterPdcp, newMasterPdcp);    //ConnectionTable PdcpEntities

	///////////////////////////////////////SDAP///////////////////////////////////////
	NRsdapUE * sdapUe = check_and_cast<NRsdapUE *>(getParentModule()->getSubmodule("sdap"));

	NRsdapGNB *oldSdapGNB = check_and_cast<NRsdapGNB *>(getSimulation()->getModule(oldMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("sdap"));
	NRsdapGNB *newSdapGNB = check_and_cast<NRsdapGNB *>(getSimulation()->getModule(newMasterOmnetId)->getSubmodule("lteNic")->getSubmodule("sdap"));

	//exchange entities
	NRSdapEntities oldEntities = oldSdapGNB->getEntities();

	for (auto & var : oldEntities) {
		if (std::get < 0 > (var.first) == nodeId_ || std::get < 1 > (var.first) == nodeId_) {

			NRSdapEntity * sdapTmp = new NRSdapEntity(*(var.second));
			AddressTuple old = var.first;
			if (std::get < 0 > (old) == oldMasterId) {
				std::get < 0 > (old) = newMasterId;
			} else if (std::get < 1 > (old) == oldMasterId) {
				std::get < 1 > (old) = newMasterId;
			}
			newSdapGNB->getEntities().insert(std::make_pair(old, sdapTmp));
		}
	}

	NRSdapEntities ueEntities = sdapUe->getEntities();
	NRSdapEntities newUeEntities;
	std::vector<AddressTuple> deleteEntities;
	for (auto & var : ueEntities) {
		if (std::get < 0 > (var.first) == oldMasterId) {
			NRSdapEntity * tmpEntity = var.second;
			AddressTuple tmpTuple = std::make_tuple(newMasterId, std::get < 1 > (var.first), std::get < 2 > (var.first)); //gen new key
			newUeEntities[tmpTuple] = tmpEntity;
		} else if (std::get < 1 > (var.first) == oldMasterId) {
			NRSdapEntity * tmpEntity = var.second;
			AddressTuple tmpTuple = std::make_tuple(std::get < 0 > (var.first), newMasterId, std::get < 2 > (var.first));
			newUeEntities[tmpTuple] = tmpEntity;
		} else {
			NRSdapEntity * tmpEntity = var.second;
			AddressTuple tmpTuple = std::make_tuple(std::get < 0 > (var.first), std::get < 1 > (var.first), std::get < 2 > (var.first));
			newUeEntities[tmpTuple] = tmpEntity;
		}
	}

	ueEntities.clear();
	sdapUe->getEntities() = newUeEntities;

	//////////////////////////////////////////////////////////////////////////////////
	//delete node from oldMasterNode
	//qosHandler GNB
	oldMasterMac->deleteNodeFromQosHandler(nodeId_);
	oldMasterMac->deleteQueues(nodeId_);
	oldMasterRlcUmRealistic->deleteQueues(nodeId_);
	oldMasterPdcp->resetConnectionTable(oldMasterId, nodeId_);
	oldSdapGNB->deleteEntities(nodeId_);

	//std::cout << "NRPhyUe deleteOldBuffers end at " << simTime().dbl() << std::endl;
}
