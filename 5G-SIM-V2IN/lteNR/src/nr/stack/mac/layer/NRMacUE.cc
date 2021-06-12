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

#include "nr/stack/mac/layer/NRMacUE.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/common/TimeTag_m.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"

Define_Module(NRMacUE);

void NRMacUE::initialize(int stage) {

	//LteMacBase::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		/* Gates initialization */
		up_[IN_GATE] = gate("RLC_to_MAC");
		up_[OUT_GATE] = gate("MAC_to_RLC");
		down_[IN_GATE] = gate("PHY_to_MAC");
		down_[OUT_GATE] = gate("MAC_to_PHY");

		/* Create buffers */
		queueSize_ = par("queueSize");

		/* Get reference to binder */
		binder_ = getNRBinder();

		/* Set The MAC MIB */

		muMimo_ = par("muMimo");

		harqProcesses_ = par("harqProcesses");

		/* statistics */
		statDisplay_ = par("statDisplay");

		totalOverflowedBytes_ = 0;
		nrFromUpper_ = 0;
		nrFromLower_ = 0;
		nrToUpper_ = 0;
		nrToLower_ = 0;

		/* register signals */
		macBufferOverflowDl_ = registerSignal("macBufferOverFlowDl");
		macBufferOverflowUl_ = registerSignal("macBufferOverFlowUl");
		if (isD2DCapable())
			macBufferOverflowD2D_ = registerSignal("macBufferOverFlowD2D");

		receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
		receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
		sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
		sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

		measuredItbs_ = registerSignal("measuredItbs");
		WATCH(queueSize_);
		WATCH(nodeId_);
		WATCH_MAP(mbuf_);
		WATCH_MAP(macBuffers_);
	}

	//LteMacUe::initialize(stage);
	if (stage == INITSTAGE_LOCAL) {
		cqiDlMuMimo0_ = registerSignal("cqiDlMuMimo0");
		cqiDlMuMimo1_ = registerSignal("cqiDlMuMimo1");
		cqiDlMuMimo2_ = registerSignal("cqiDlMuMimo2");
		cqiDlMuMimo3_ = registerSignal("cqiDlMuMimo3");
		cqiDlMuMimo4_ = registerSignal("cqiDlMuMimo4");

		cqiDlTxDiv0_ = registerSignal("cqiDlTxDiv0");
		cqiDlTxDiv1_ = registerSignal("cqiDlTxDiv1");
		cqiDlTxDiv2_ = registerSignal("cqiDlTxDiv2");
		cqiDlTxDiv3_ = registerSignal("cqiDlTxDiv3");
		cqiDlTxDiv4_ = registerSignal("cqiDlTxDiv4");

		cqiDlSpmux0_ = registerSignal("cqiDlSpmux0");
		cqiDlSpmux1_ = registerSignal("cqiDlSpmux1");
		cqiDlSpmux2_ = registerSignal("cqiDlSpmux2");
		cqiDlSpmux3_ = registerSignal("cqiDlSpmux3");
		cqiDlSpmux4_ = registerSignal("cqiDlSpmux4");

		cqiDlSiso0_ = registerSignal("cqiDlSiso0");
		cqiDlSiso1_ = registerSignal("cqiDlSiso1");
		cqiDlSiso2_ = registerSignal("cqiDlSiso2");
		cqiDlSiso3_ = registerSignal("cqiDlSiso3");
		cqiDlSiso4_ = registerSignal("cqiDlSiso4");
	}
	else if (stage == INITSTAGE_LINK_LAYER) {
		cellId_ = getAncestorPar("masterId");
	}
	else if (stage == INITSTAGE_NETWORK_LAYER) {
		nodeId_ = getAncestorPar("macNodeId");

		/* Insert UeInfo in the Binder */
		UeInfo *info = new UeInfo();
		info->id = nodeId_;            // local mac ID
		info->cellId = cellId_;        // cell ID
		info->init = false;            // flag for phy initialization
		info->ue = this->getParentModule()->getParentModule(); // reference to the UE module
		info->phy = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

		phy_ = info->phy;

		binder_->addUeInfo(info);

		if (cellId_ > 0) {
			LteAmc *amc = check_and_cast<LteMacEnb*>(getMacByMacNodeId(cellId_))->getAmc();
			amc->attachUser(nodeId_, UL);
			amc->attachUser(nodeId_, DL);
		}

		// find interface entry and use its address
		IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
		// TODO: how do we find the LTE interface?
		InterfaceEntry *interfaceEntry = interfaceTable->findInterfaceByName("wlan");
		if (interfaceEntry == nullptr)
			throw new cRuntimeError("no interface entry for lte interface - cannot bind node %i", nodeId_);

		inet::Ipv4InterfaceData *ipv4if = interfaceEntry->getProtocolData<inet::Ipv4InterfaceData>();
		if (ipv4if == nullptr)
			throw new cRuntimeError("no Ipv4 interface data - cannot bind node %i", nodeId_);
		binder_->setMacNodeId(ipv4if->getIPAddress(), nodeId_);

		// Register the "ext" interface, if present
		if (hasPar("enableExtInterface") && getAncestorPar("enableExtInterface").boolValue()) {
			// get address of the localhost to enable forwarding
			Ipv4Address extHostAddress = Ipv4Address(getAncestorPar("extHostAddress").stringValue());
			binder_->setMacNodeId(extHostAddress, nodeId_);
		}
	}
	else if (stage == inet::INITSTAGE_TRANSPORT_LAYER) {
		const std::map<double, LteChannelModel*> *channelModels = phy_->getChannelModels();
		std::map<double, LteChannelModel*>::const_iterator it = channelModels->begin();
		for (; it != channelModels->end(); ++it) {
			lcgScheduler_[it->first] = new NRSchedulerUeUl(this, it->first);
		}
	}
	else if (stage == inet::INITSTAGE_LAST) {
		/* Start TTI tick */
		// the period is equal to the minimum period according to the numerologies used by the carriers in this node
		ttiTick_ = new cMessage("ttiTick_");
		ttiTick_->setSchedulingPriority(1);     // TTI TICK after other messages
		ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(binder_->getUeMaxNumerologyIndex(nodeId_));
		scheduleAt(NOW + ttiPeriod_, ttiTick_);

		const std::set<NumerologyIndex> *numerologyIndexSet = binder_->getUeNumerologyIndex(nodeId_);
		if (numerologyIndexSet != NULL) {
			std::set<NumerologyIndex>::const_iterator it = numerologyIndexSet->begin();
			for (; it != numerologyIndexSet->end(); ++it) {
				// set periodicity for this carrier according to its numerology
				NumerologyPeriodCounter info;
				info.max = 1 << (binder_->getUeMaxNumerologyIndex(nodeId_) - *it); // 2^(maxNumerologyIndex - numerologyIndex)
				info.current = info.max - 1;
				numerologyPeriodCounter_[*it] = info;
			}
		}
	}

	//NRMacUe::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		rcvdD2DModeSwitchNotification_ = registerSignal("rcvdD2DModeSwitchNotification");
	}
	if (stage == inet::INITSTAGE_NETWORK_LAYER) {
		// get parameters
		usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");

		if (cellId_ > 0) {
			preconfiguredTxParams_ = getPreconfiguredTxParams();

			// get the reference to the eNB
			enb_ = check_and_cast<LteMacEnbD2D*>(getMacByMacNodeId(cellId_));

			LteAmc *amc = check_and_cast<LteMacEnb*>(getSimulation()->getModule(binder_->getOmnetId(cellId_))->getSubmodule("cellularNic")->getSubmodule("mac"))->getAmc();
			amc->attachUser(nodeId_, D2D);
		}
		else
			enb_ = NULL;
	}

	//local
	if (stage == inet::INITSTAGE_LOCAL) {

		qosHandler = check_and_cast<QosHandlerUE*>(getParentModule()->getSubmodule("qosHandler"));

		harqProcesses_ = getSystemModule()->par("numberHarqProcesses").intValue();
		harqProcessesNR_ = getSystemModule()->par("numberHarqProcessesNR").intValue();
		if (getSystemModule()->par("nrHarq").boolValue()) {
			harqProcesses_ = harqProcessesNR_;
		}
	}
}

void NRMacUE::macHandleGrant(cPacket * pkt) {
	//std::cout << "NRMacUe macHandleGrant start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::macHandleGrant(pkt);

//std::cout << "NRMacUe macHandleGrant start at " << simTime().dbl() << std::endl;
}

void NRMacUE::handleMessage(omnetpp::cMessage * msg) {

//std::cout << "NRMacUe::handleMessage start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::handleMessage(msg);

//std::cout << "NRMacUe::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacUE::fromPhy(omnetpp::cPacket * pktAux) {

//std::cout << "NRMacUe::fromPhy start at " << simTime().dbl() << std::endl;

	auto pkt = check_and_cast<inet::Packet*>(pktAux);
	auto userInfo = pkt->getTag<UserControlInfo>();

	if (userInfo->getFrameType() == DATAPKT) {

		if (qosHandler->getQosInfo().find(userInfo->getCid()) == qosHandler->getQosInfo().end()) {
			QosInfo tmp;
			tmp.appType = (ApplicationType) userInfo->getApplication();
			tmp.cid = userInfo->getCid();
			tmp.lcid = userInfo->getLcid();
			tmp.qfi = userInfo->getQfi();
			tmp.radioBearerId = userInfo->getRadioBearerId();
			tmp.destNodeId = userInfo->getDestId();
			tmp.senderNodeId = userInfo->getSourceId();
			tmp.containsSeveralCids = userInfo->getContainsSeveralCids();
			tmp.rlcType = userInfo->getRlcType();
			tmp.trafficClass = (LteTrafficClass) userInfo->getTraffic();
			qosHandler->getQosInfo()[userInfo->getCid()] = tmp;
		}
	}

	//TODO check if code has to be overwritten
	NRMacUe::fromPhy(pkt);

//std::cout << "NRMacUe::fromPhy end at " << simTime().dbl() << std::endl;
}

int NRMacUE::macSduRequest() {

//std::cout << "NRMacUe macSduRequest start at " << simTime().dbl() << std::endl;

//std::cout << "NRMacUe macSduRequest end at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return NRMacUe::macSduRequest();

}

void NRMacUE::macPduUnmake(cPacket * pkt) {

	//TODO check if code has to be overwritten
	NRMacUe::macPduUnmake(pkt);

}

void NRMacUE::handleSelfMessage() {
//std::cout << "NRMacUe handleSelfMessage start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::handleSelfMessage();

//std::cout << "NRMacUe handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void NRMacUE::macPduMake(MacCid cid) {

//std::cout << "NRMacUe macPduMake start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::macPduMake(cid);

//std::cout << "NRMacUe macPduMake end at " << simTime().dbl() << std::endl;
}

void NRMacUE::handleUpperMessage(cPacket * pkt) {
//std::cout << "NRMacUe handleUpperMessage start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::handleUpperMessage(pkt);

//std::cout << "NRMacUe handleUpperMessage end at " << simTime().dbl() << std::endl;
}

bool NRMacUE::bufferizePacket(omnetpp::cPacket * pkt) {

//std::cout << "NRMacUe bufferizePacket start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	return NRMacUe::bufferizePacket(pkt);

}

void NRMacUE::flushHarqBuffers() {

//std::cout << "NRMacUe flushHarqBuffers start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::flushHarqBuffers();

//std::cout << "NRMacUe flushHarqBuffers end at " << simTime().dbl() << std::endl;
}

void NRMacUE::checkRAC() {
//std::cout << "NRMacUe checkRAC start at " << simTime().dbl() << std::endl;

	//TODO check if code has to be overwritten
	NRMacUe::checkRAC();

//std::cout << "NRMacUe checkRAC end at " << simTime().dbl() << std::endl;
}

