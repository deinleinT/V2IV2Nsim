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

#include "nr/stack/mac/scheduler/NRQoSModelScheduler.h"

NRQoSModelScheduler::~NRQoSModelScheduler() {

}

//grant is configured in UE
ScheduleList & NRQoSModelScheduler::schedule(MacCid cid, unsigned int availableBytes, Direction grantDir) {

	scheduleList_.clear();
    scheduledBytesList_.clear();

	statusMap_.clear();
	statusMapEnh_.clear();

	LcgMap &lcgMap = mac_->getLcgMap();

	for (auto &var : lcgMap) {

		//cid is the cid which was scheduled by the base station
		if (var.second.first == cid) {

			LteMacBuffer *vQueue;
			vQueue = var.second.second;

			// get the buffer size

			unsigned int queueLength = vQueue->getQueueOccupancy(); // in bytes
			if (getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
				queueLength = (vQueue->getQueueOccupancy() == 0) ? 0 : vQueue->getQueueOccupancy() / vQueue->getQueueLength(); // in bytes single Packet
			}

			// get the Flow descriptor
			FlowControlInfo connDesc = mac_->getConnDesc().at(cid);

			// connection must have the same direction of the grant
			if (connDesc.getDirection() != grantDir)
				continue;

			unsigned int queueSizeToServe = queueLength;

			if (queueLength == 0) {
				//EV << "NRQoSModelScheduler::schedule scheduled connection is no more active " << endl;
				continue;// go to next connection
			} else {
				// we need to consider also the size of RLC and MAC headers
				if (connDesc.getRlcType() == UM)
					queueSizeToServe += RLC_HEADER_UM;
				else if (connDesc.getRlcType() == AM)
					queueSizeToServe += RLC_HEADER_AM;
				queueSizeToServe += MAC_HEADER;
			}

			StatusElemEnhanced *bufferInfo;
			if (statusMapEnh_.find(cid) == statusMapEnh_.end()) {
				// the element does not exist, initialize it
				bufferInfo = &statusMapEnh_[cid];
				bufferInfo->occupancy_ = vQueue->getQueueOccupancy();
				bufferInfo->sentData_ = 0;
				bufferInfo->sentSdus_ = 0;
			} else {
				bufferInfo = &statusMapEnh_[cid];
			}

			if ((availableBytes > 0) && (queueSizeToServe > 0)) {
				if ((queueSizeToServe <= availableBytes)) {
					// remove SDU from virtual buffer
					vQueue->popFront();

					// update the status element
					bufferInfo->occupancy_ = vQueue->getQueueOccupancy();
					bufferInfo->sentData_ += queueSizeToServe;
					scheduledBytesList_[cid] = bufferInfo->sentData_;

					// check if there is space for a SDU
					int alloc = queueSizeToServe;
					alloc -= MAC_HEADER;
					if (connDesc.getRlcType() == UM)
						alloc -= RLC_HEADER_UM;
					else if (connDesc.getRlcType() == AM)
						alloc -= RLC_HEADER_AM;

					if (alloc > 0)
						bufferInfo->sentSdus_++;

					availableBytes -= queueSizeToServe;
					scheduleList_[cid] = bufferInfo->sentSdus_;

					if (!getSimulation()->getSystemModule()->par("useSinglePacketSizeDuringScheduling").boolValue()) {
						while (!vQueue->isEmpty()) {
							// remove SDUs from virtual buffer
							vQueue->popFront();
						}
					}

					queueSizeToServe = 0;

				} else {

					// update the status element
					bufferInfo->occupancy_ = vQueue->getQueueOccupancy();
					bufferInfo->sentData_ += availableBytes;

					int alloc = availableBytes;
					alloc -= MAC_HEADER;
					if (connDesc.getRlcType() == UM)
						alloc -= RLC_HEADER_UM;
					else if (connDesc.getRlcType() == AM)
						alloc -= RLC_HEADER_AM;

					// check if there is space for a SDU
					if (alloc > 0) {
						bufferInfo->sentSdus_++;
					}

					// update buffer
					while (alloc > 0) {
						// update pkt info
						PacketInfo newPktInfo = vQueue->popFront();
						if (newPktInfo.first > alloc) {
							newPktInfo.first = newPktInfo.first - alloc;
							vQueue->pushFront(newPktInfo);
							alloc = 0;
						} else {
							alloc -= newPktInfo.first;
						}

					}

					queueSizeToServe -= availableBytes;
					scheduledBytesList_[cid] = bufferInfo->sentData_;
					scheduleList_[cid] = bufferInfo->sentSdus_;

					availableBytes = 0;
				}
			} // END of connections cycle
			if (scheduleList_.size() == 1) {
				return scheduleList_;
			}
		} else {
			continue;
		}

	}

	return scheduleList_;
}

