//
// SPDX-FileCopyrightText: 2021 Friedrich-Alexander University Erlangen-Nuernberg (FAU), Computer Science 7 - Computer Networks and Communication Systems
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

#include "common/LteCommon.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "nr/stack/mac/scheduling_modules/NRQoSModel.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/layer/LteMacUe.h"

NRQoSModel::NRQoSModel(Direction dir) {

	LteScheduler();
	this->dir = dir;
}

NRQoSModel::~NRQoSModel() {

}

void NRQoSModel::notifyActiveConnection(MacCid cid) {
	//std::cout << "NRQoSModel::notifyActiveConnection start at " << simTime().dbl() << std::endl;

	activeConnectionSet_.insert(cid);

	//std::cout << "NRQoSModel::notifyActiveConnection end at " << simTime().dbl() << std::endl;
}

void NRQoSModel::removeActiveConnection(MacCid cid) {
	//std::cout << "NRQoSModel::removeActiveConnection start at " << simTime().dbl() << std::endl;

	activeConnectionSet_.erase(cid);

	//std::cout << "NRQoSModel::removeActiveConnection end at " << simTime().dbl() << std::endl;
}

//DL and UL
void NRQoSModel::prepareSchedule() //
{
	//std::cout << "NRQoSModel::prepareSchedule start at " << simTime().dbl() << std::endl;//XXX

	//std::cout << "NRQoSModel::notifyActiveConnection start at " << simTime().dbl() << std::endl;

	if (binder_ == NULL)
		binder_ = getBinder();

	// Clear structures
	grantedBytes_.clear();

//	//Look into the macBuffers and find out all active connections
//	for (auto &var : *mac_->getMacBuffers()) {
//		if (!var.second->isEmpty()) {
//			eNbScheduler_->backlog(var.first);
//		}
//	}

	// Create a working copy of the active set
	activeConnectionTempSet_ = activeConnectionSet_;

	//find out which cid has highest priority via qosHandler
	std::map<double, std::vector<QosInfo>> sortedCids = mac_->getQosHandler()->getEqualPriorityMap(dir);
	std::map<double, std::vector<QosInfo>>::reverse_iterator sortedCidsIter = sortedCids.rbegin();

	//create a map with cids which are in the racStatusMap
	std::map<double, std::vector<QosInfo>> activeCids;
	for (auto &var : sortedCids) {
		for (auto &qosinfo : var.second) {
			for (auto &cid : activeConnectionTempSet_) {
				MacCid realCidQosInfo;
				if(dir == DL){
					realCidQosInfo = idToMacCid(qosinfo.destNodeId, qosinfo.lcid);
				}else{
					realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid);
				}
				if (realCidQosInfo == cid) {
					//we have the QosInfos for the cid in the racStatus
					activeCids[var.first].push_back(qosinfo);
				}
			}
		}
	}

	for (auto &var : activeCids) {

		for (auto &qosinfos : var.second) {

			MacCid cid; /* = idToMacCid(qosinfos.destNodeId, qosinfos.lcid);*/
			if (dir == DL) {
				cid = idToMacCid(qosinfos.destNodeId, qosinfos.lcid);
			} else {
				cid = idToMacCid(qosinfos.senderNodeId, qosinfos.lcid);
			}

			MacNodeId nodeId = MacCidToNodeId(cid);
			OmnetId id = binder_->getOmnetId(nodeId);

			if (this->mac_->getRtxSignalised(nodeId)) {
				continue;
			}

			if (nodeId == 0 || id == 0) {
				// node has left the simulation - erase corresponding CIDs
				activeConnectionSet_.erase(cid);
				activeConnectionTempSet_.erase(cid);
				continue;
			}

			// check if node is still a valid node in the simulation - might have been dynamically removed
			if (getBinder()->getOmnetId(nodeId) == 0) {
				activeConnectionTempSet_.erase(cid);
				//EV << "CID " << cid << " of node " << nodeId << " removed from active connection set - no OmnetId in Binder known.";
				continue;
			}

			// compute available blocks for the current user
			const UserTxParams &info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId, direction_);
			const std::set<Band> &bands = info.readBands();
			unsigned int codeword = info.getLayers().size();
			if (eNbScheduler_->allocatedCws(nodeId) == codeword)
				continue;
			std::set<Band>::const_iterator it = bands.begin(), et = bands.end();

			std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt = info.readAntennaSet().end();

			bool cqiNull = false;
			for (unsigned int i = 0; i < codeword; i++) {
				if (info.readCqiVector()[i] == 0)
					cqiNull = true;
			}
			if (cqiNull)
				continue;
			// compute score based on total available bytes
			unsigned int availableBlocks = 0;
			unsigned int availableBytes = 0;
			// for each antenna
			for (; antennaIt != antennaEt; ++antennaIt) {
				// for each logical band
				for (; it != et; ++it) {
					availableBlocks += eNbScheduler_->readAvailableRbs(nodeId, *antennaIt, *it);
					availableBytes += eNbScheduler_->getMac()->getAmc()->computeBytesOnNRbs(nodeId, *it, availableBlocks, direction_);
				}
			}

			// Grant data to that connection.
			bool terminate = false;
			bool active = true;
			bool eligible = true;

			unsigned int granted = eNbScheduler_->scheduleGrant(cid, 4294967295U, terminate, active, eligible);
			grantedBytes_[cid] += granted;

			// Exit immediately if the terminate flag is set.
			if (terminate) {
				//EV << NOW << "LtePf::execSchedule TERMINATE " << endl;
				break;
			}

			if (!active) {
				//EV << NOW << "LtePf::execSchedule NOT ACTIVE" << endl;
				activeConnectionTempSet_.erase(cid);
			}
		}
	}

	//std::cout << "NRQoSModel::prepareSchedule end at " << simTime().dbl() << std::endl;
}

void NRQoSModel::commitSchedule() {
	//std::cout << "NRQoSModel::commitSchedule start at " << simTime().dbl() << std::endl;

	//first approach --> no need to do something here, grantedBytes already includes the granted blocks/bytes
	activeConnectionSet_ = activeConnectionTempSet_;

	//std::cout << "NRQoSModel::commitSchedule start at " << simTime().dbl() << std::endl;
}

