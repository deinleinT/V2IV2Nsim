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

#include "nr/stack/phy/feedback/NRDlFeedbackGenerator.h"

Define_Module(NRDlFeedbackGenerator);

void NRDlFeedbackGenerator::handleMessage(cMessage *msg){
    LteDlFeedbackGenerator::handleMessage(msg);
}

void NRDlFeedbackGenerator::initialize(int stage) {
	//LteDlFeedbackGenerator::initialize(stage);
	if (stage == INITSTAGE_LOCAL) {
		// Read NED parameters
		fbPeriod_ = (simtime_t) (int(par("fbPeriod")) * TTI); // TTI -> seconds
		fbDelay_ = (simtime_t) (int(par("fbDelay")) * TTI); // TTI -> seconds
		if (fbPeriod_ <= fbDelay_) {
			error("Feedback Period MUST be greater than Feedback Delay");
		}
		fbType_ = getFeedbackType(par("feedbackType").stringValue());
		rbAllocationType_ = getRbAllocationType(par("rbAllocationType").stringValue());
		usePeriodic_ = par("usePeriodic");
		currentTxMode_ = aToTxMode(getSimulation()->getSystemModule()->par("initialTxMode").stringValue());

		generatorType_ = getFeedbackGeneratorType(par("feedbackGeneratorType").stringValue());

		masterId_ = getAncestorPar("masterId");
		nodeId_ = getAncestorPar("macNodeId");

		/** Initialize timers **/

		tPeriodicSensing_ = new TTimer(this);
		tPeriodicSensing_->setTimerId(PERIODIC_SENSING);

		tPeriodicTx_ = new TTimer(this);
		tPeriodicTx_->setTimerId(PERIODIC_TX);

		tAperiodicTx_ = new TTimer(this);
		tAperiodicTx_->setTimerId(APERIODIC_TX);
		feedbackComputationPisa_ = false;
		WATCH(fbType_);
		WATCH(rbAllocationType_);
		WATCH(fbPeriod_);
		WATCH(fbDelay_);
		WATCH(usePeriodic_);
		WATCH(currentTxMode_);
	}
	else if (stage == INITSTAGE_LINK_LAYER) {

		if (masterId_ > 0)  // only if not detached
			initCellInfo();

		LtePhyUe *tmp = dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy"));
		dasFilter_ = tmp->getDasFilter();

		//        initializeFeedbackComputation(par("feedbackComputation").xmlValue());

		// TODO: remove this parameter
		feedbackComputationPisa_ = true;

		WATCH(numBands_);
		WATCH(numPreferredBands_);
		if (masterId_ > 0 && usePeriodic_) {
			tPeriodicSensing_->start(0);
		}
	}
}

void NRDlFeedbackGenerator::sendFeedback(LteFeedbackDoubleVector fb, FbPeriodicity per) {

	FeedbackRequest feedbackReq;
	if (feedbackComputationPisa_) {
		feedbackReq.request = true;
		feedbackReq.genType = getFeedbackGeneratorType(getAncestorPar("feedbackGeneratorType").stringValue());
		feedbackReq.type = getFeedbackType(par("feedbackType").stringValue());
		feedbackReq.txMode = currentTxMode_;
		feedbackReq.rbAllocationType = rbAllocationType_;
	}
	else {
		feedbackReq.request = false;
	}

	(dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy")))->sendFeedback(fb, fb, feedbackReq);
}
