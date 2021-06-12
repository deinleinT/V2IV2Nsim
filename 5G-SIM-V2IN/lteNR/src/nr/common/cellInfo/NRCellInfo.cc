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

#include "nr/common/cellInfo/NRCellInfo.h"

Define_Module(NRCellInfo);

void NRCellInfo::initialize(int stage) {

	// same as calling CellInfo::initialize(stage);
	if (stage == inet::INITSTAGE_LOCAL) {
		pgnMinX_ = par("constraintAreaMinX");
		pgnMinY_ = par("constraintAreaMinY");
		pgnMaxX_ = par("constraintAreaMaxX");
		pgnMaxY_ = par("constraintAreaMaxY");

		eNbType_ = par("microCell").boolValue() ? MICRO_ENB : MACRO_ENB;
		rbyDl_ = par("rbyDl");
		rbyUl_ = par("rbyUl");
		rbxDl_ = par("rbxDl");
		rbxUl_ = par("rbxUl");
		rbPilotDl_ = par("rbPilotDl");
		rbPilotUl_ = par("rbPilotUl");
		signalDl_ = par("signalDl");
		signalUl_ = par("signalUl");
	}
	if (stage == inet::INITSTAGE_LOCAL + 1) {
		// get the total number of bands in the system
		totalBands_ = binder_->getTotalBands();

		numRus_ = par("numRus");

		numPreferredBands_ = par("numPreferredBands");

		if (numRus_ > NUM_RUS)
			throw omnetpp::cRuntimeError("The number of Antennas specified exceeds the limit of %d", NUM_RUS);

		// register the containing eNB  to the binder
		cellId_ = getParentModule()->par("macCellId");

		int ruRange = par("ruRange");
		double nodebTxPower = getAncestorPar("txPower");

		// first RU to be registered is the MACRO
		ruSet_->addRemoteAntenna(nodeX_, nodeY_, nodebTxPower);

		// REFACTORING: has no effect, as long as numRus_ == 0
		// deploy RUs
		deployRu(nodeX_, nodeY_, numRus_, ruRange);

		// MCS scaling factor
		calculateMCSScale(&mcsScaleUl_, &mcsScaleDl_);

		createAntennaCwMap();
	}

}

