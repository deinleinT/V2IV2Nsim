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

#include "nr/stack/mac/scheduler/NRLcgScheduler.h"

NRLcgScheduler::~NRLcgScheduler() {
	// TODO Auto-generated destructor stub
}

ScheduleListSizes& NRLcgScheduler::schedule(unsigned int availableBytes, Direction grantDir) {
	//std::cout << "NRLcgScheduler::schedule start at " << simTime().dbl() << std::endl;

	/* clean up old schedule decisions
	 for each cid, this map will store the the amount of sent data (in SDUs)
	 */
//    scheduleList_.clear();
	scheduleListSizes_.clear();

	/* clean up old schedule decisions
	 for each cid, this map will store the the amount of sent data (in bytes, useful for macSduRequest)
	 */
	scheduledBytesList_.clear();

	/*
	 * Clean up scheduling support status map
	 */
	statusMap_.clear();

	// amount of time passed since last scheduling operation
	simtime_t timeInterval = NOW - lastExecutionTime_;

	// these variables will contain the flow identifier and the priority of the lowest
	// priority flow encountered during the scheduling process
	int lowestBackloggedFlow = -1;
	int lowestBackloggedPriority = -1;

	// these variables will contain the flow identifier and the priority of the highest
	// priority flow encountered during the scheduling process
	int highestBackloggedFlow = -1;
	int highestBackloggedPriority = -1;

	// Considering 3GPP TS 36.321:
	//  - ConnDesc::parameters_.minReservedRate_    --> Prioritized Bit Rate (PBR) in Bytes/s
	//  - ConnDesc::parameters_.maximumBucketSizeurst_         --> Maximum Bucket size (MBS) = PBR*BSD product
	//  - ConnDesc::parameters_.bucket_         --> is the B parameter in Bytes

	// If true, assure a minimum reserved rate to all connection (LCP first
	// phase), if false, provide a best effort service (LCP second phase)
	bool priorityService = true;

	LcgMap &lcgMap = mac_->getLcgMap();
	QosHandler *qosHandler = mac_->getQosHandler();

	if (lcgMap.empty())
		return scheduleListSizes_;

	// for all traffic classes
//    for (unsigned short i = 0; i < UNKNOWN_TRAFFIC_TYPE; ++i)
	for (auto &var : qosHandler->getSortedQosInfos()) {
		if (var.second.qfi == 0)
			continue;
		// Prepare the iterators to cycle the entire scheduling set
		std::pair<LcgMap::iterator, LcgMap::iterator> it_pair;
		unsigned short i = var.second.trafficClass;
		it_pair = lcgMap.equal_range((LteTrafficClass) i);
		LcgMap::iterator it = it_pair.first, et = it_pair.second;

		//EV << NOW << " NRLcgScheduler::schedule - Node  " << mac_->getMacNodeId() << ", Starting priority service for traffic class " << i << endl;

		//! FIXME Allocation of the same resource to flows with same priority not implemented - not suitable with relays
		for (; it != et; ++it) {
			// processing all connections of same traffic class

			// get the connection virtual buffer
			LteMacBuffer *vQueue = it->second.second;

			// get the buffer size
			unsigned int queueLength = vQueue->getQueueOccupancy(); // in bytes

			// connection id of the processed connection
			MacCid cid = it->second.first;

			// get the Flow descriptor
			FlowControlInfo connDesc = mac_->getConnDesc().at(cid);
			// TODO get the QoS parameters

			// connection must have the same direction of the grant
			if (connDesc.getDirection() != grantDir)
				continue;

			unsigned int toServe = queueLength;
			// Check whether the virtual buffer is empty
			if (queueLength == 0) {
				//EV << "NRLcgScheduler::schedule scheduled connection is no more active " << endl;
				continue;// go to next connection
			} else {
				// we need to consider also the size of RLC and MAC headers
				if (connDesc.getRlcType() == UM)
					toServe += RLC_HEADER_UM;
				else if (connDesc.getRlcType() == AM)
					toServe += RLC_HEADER_AM;
				toServe += MAC_HEADER;
			}

			// get a pointer to the appropriate status element: we need a tracing element
			// in order to store information about connections and data transmitted. These
			// information may be consulted at the end of the LCP algorithm
			StatusElem *elem;

			if (statusMap_.find(cid) == statusMap_.end()) {
				// the element does not exist, initialize it
				elem = &statusMap_[cid];
				elem->occupancy_ = vQueue->getQueueOccupancy();
				elem->sentData_ = 0;
				elem->sentSdus_ = 0;
				// TODO set bucket from QoS parameters
				elem->bucket_ = 1000;
			} else {
				elem = &statusMap_[cid];
			}

			//EV << NOW << " NRLcgScheduler::schedule Node " << mac_->getMacNodeId() << " , Parameters:" << endl;
			//EV << "\t Logical Channel ID: " << MacCidToLcid(cid) << endl;
			//EV << "\t CID: " << cid << endl;
//                fprintf(stderr, "\tGroup ID: %d\n", desc->parameters_.groupId_);
//                fprintf(stderr, "\tPriority: %d\n", desc->parameters_.priority_);
//                fprintf(stderr, "\tMin Reserved Rate: %.0lf bytes/s\n", desc->parameters_.minReservedRate_);
//                fprintf(stderr, "\tMax Burst: %.0lf bytes\n", desc->parameters_.maxBurst_);

			if (priorityService) {
				// Update bucket value for this connection

				// get the actual bucket value and the configured max size
				double bucket = elem->bucket_; // TODO parameters -> bucket ;
				double maximumBucketSize = 10000.0; // TODO  parameters -> maxBurst;

				//EV << NOW << " NRLcgScheduler::schedule Bucket size: " << bucket << " bytes (max size " << maximumBucketSize << " bytes) - BEFORE SERVICE " << endl;

				// if the connection started before last scheduling event , use the
				// global time interval
				if (lastExecutionTime_ > 0) { // TODO desc->parameters_.startTime_) {
											  // PBR*(n*TTI) where n is the number of TTI from last update

					bucket += /* TODO desc->parameters_.minReservedRate_*/100.0 * getBinder()->getTTI();

				}
				// otherwise, set the bucket value accordingly to the start time
				else {
					simtime_t localTimeInterval = NOW - 0/* TODO desc->parameters_.startTime_ */;
					if (localTimeInterval < 0)
						localTimeInterval = 0;

					bucket = /* TODO desc->parameters_.minReservedRate_*/100.0 * localTimeInterval.dbl();
				}

				// do not overflow the maximum bucket size
				if (bucket > maximumBucketSize)
					bucket = maximumBucketSize;

				// update connection's bucket
// TODO                desc->parameters_.bucket_ = bucket;

				// update the tracing element accordingly
				elem->bucket_ = 100.0; // TODO desc->parameters_.bucket_;
				//EV << NOW << " NRLcgScheduler::schedule Bucket size: " << bucket << " bytes (max size " << maximumBucketSize << " bytes) - AFTER SERVICE " << endl;
			}

			//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", remaining grant: " << availableBytes << " bytes " << endl;
			//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << " buffer Size: " << toServe << " bytes " << endl;

			// If priority service: (availableBytes>0) && (desc->buffer_.occupancy() > 0) && (desc->parameters_.bucket_ > 0)
			// If best effort service: (availableBytes>0) && (desc->buffer_.occupancy() > 0)
			if ((availableBytes > 0) && (toServe > 0) && (!priorityService || 1 /*TODO (desc->parameters_.bucket_ > 0)*/)) {
				// Check if it is possible to serve the sdu, depending on the constraint
				// of the type of service
				// Priority service:
				//    ( sdu->size() <= availableBytes) && ( sdu->size() <= desc->parameters_.bucket_)
				// Best Effort service:
				//    ( sdu->size() <= availableBytes) && (!priorityService_)

				if ((toServe <= availableBytes) /*&& ( !priorityService || ( sduSize <= 0 TODO desc->parameters_.bucket_) )*/) {
					// remove SDU from virtual buffer
					vQueue->popFront();

					if (priorityService) {
//    TODO                        desc->parameters_.bucket_ -= sduSize;
//                        // update the tracing element accordingly
//    TODO                    elem->bucket_ = 100.0 /* TODO desc->parameters_.bucket_*/;
					}

					// update the tracing element
					elem->occupancy_ = vQueue->getQueueOccupancy();
					elem->sentData_ += toServe;
					scheduleListSizes_[cid].second = elem->sentData_;

					// check if there is space for a SDU
					int alloc = toServe;
					alloc -= MAC_HEADER;
					if (connDesc.getRlcType() == UM)
						alloc -= RLC_HEADER_UM;
					else if (connDesc.getRlcType() == AM)
						alloc -= RLC_HEADER_AM;

					if (alloc > 0)
						elem->sentSdus_++;

					availableBytes -= toServe;
					scheduleListSizes_[cid].first = elem->sentSdus_;

					while (!vQueue->isEmpty()) {
						// remove SDUs from virtual buffer
						vQueue->popFront();
//                        elem->sentSdus_++;
					}

					toServe = 0;

					//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << ",  SDU of size " << elem->sentData_ << " selected for transmission" << endl;
					//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", remaining grant: " << availableBytes << " bytes" << endl;
					//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << " buffer Size: " << toServe << " bytes" << endl;
				} else {

					if (priorityService) {
//    TODO                        desc->parameters_.bucket_ -= sduSize;
//                        // update the tracing element accordingly
//    TODO                    elem->bucket_ = 100.0 /* TODO desc->parameters_.bucket_*/;
					}

					// update the tracing element
					elem->occupancy_ = vQueue->getQueueOccupancy();
					elem->sentData_ += availableBytes;

					int alloc = availableBytes;
					alloc -= MAC_HEADER;
					if (connDesc.getRlcType() == UM)
						alloc -= RLC_HEADER_UM;
					else if (connDesc.getRlcType() == AM)
						alloc -= RLC_HEADER_AM;

					// check if there is space for a SDU
					if (alloc > 0) {
						elem->sentSdus_++;

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

					toServe -= availableBytes;
					scheduleListSizes_[cid].second = elem->sentData_;
					scheduleListSizes_[cid].first = elem->sentSdus_;

					availableBytes = 0;

					//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << ",  SDU of size " << elem->sentData_ << " selected for transmission" << endl;
					//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", remaining grant: " << availableBytes << " bytes" << endl;
					//EV << NOW << " NRLcgScheduler::schedule - Node " << mac_->getMacNodeId() << " buffer Size: " << toServe << " bytes" << endl;
				}
			}

			// check if flow is still backlogged
			if (availableBytes > 0) {
				// update the last schedule time
				lastExecutionTime_ = NOW;

				// signal service for current connection
				unsigned int *servicedSdu = NULL;

				if (scheduleListSizes_.find(cid) == scheduleListSizes_.end()) {
					// the element does not exist, initialize it
					servicedSdu = &scheduleListSizes_[cid].first;
					*servicedSdu = elem->sentSdus_;
				} else {
					// connection already scheduled during this TTI
					servicedSdu = &scheduleListSizes_.at(cid).first;
				}

				// update scheduled bytes
				if (scheduledBytesList_.find(cid) == scheduledBytesList_.end()) {
					scheduledBytesList_[cid] = elem->sentData_;
				} else {
					scheduledBytesList_[cid] += elem->sentData_;
				}

				// If the end of the connections map is reached and we were on priority and on last traffic class
				if (priorityService && (it == et) && ((var.second.trafficClass + 1) == (unsigned short) UNKNOWN_TRAFFIC_TYPE)) {
					// the first phase of the LCP algorithm has completed ...
					// ... switch to best effort allocation!
					priorityService = false;
					//  reset traffic class
					i = 0;
					//EV << "NRLcgScheduler::schedule - Node" << mac_->getMacNodeId() << ", Starting best effort service" << endl;
				}
			} // END of connections cycle
		} // END of Traffic Classes cycle

		//std::cout << "NRLcgScheduler::schedule end at " << simTime().dbl() << std::endl;

	}
	return scheduleListSizes_;
}
