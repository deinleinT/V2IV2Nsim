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

#include "nr/corenetwork/cellInfo/NRCellInfo.h"

Define_Module(NRCellInfo);

void NRCellInfo::initialize() {

    int numerology = getBinder()->getNumerology();
    if (numerology == 15 || numerology == 30 || numerology == 60) {
        rbxDl_ = 14; // number of symbols per slot DL
        rbxUl_ = 14; // number of symbols per slot UL
		if (getSimulation()->getSystemModule()->par("useTdd").boolValue() && !getSimulation()->getSystemModule()->par("printTBS").boolValue()) {
			rbxDl_ = getSimulation()->getSystemModule()->par("dlSymbols"); // number of symbols per slot DL
			rbxUl_ = getSimulation()->getSystemModule()->par("ulSymbols"); // number of symbols per slot UL
		}
    } else
        throw cRuntimeError(
                "Unknown numerology of %d, possible values are 15, 30 or 60");

    pgnMinX_ = par("constraintAreaMinX");
    pgnMinY_ = par("constraintAreaMinY");
    pgnMaxX_ = par("constraintAreaMaxX");
    pgnMaxY_ = par("constraintAreaMaxY");

    int ruRange = par("ruRange");
    double nodebTxPower = getAncestorPar("txPower");
    eNbType_ = par("microCell").boolValue() ? MICRO_ENB : MACRO_ENB;
    numRbDl_ = par("numRbDl");
    numRbUl_ = par("numRbUl");
    rbyDl_ = par("rbyDl"); // subcarriers
    rbyUl_ = par("rbyUl");
    rbPilotDl_ = par("rbPilotDl");
    rbPilotUl_ = par("rbPilotUl");
    signalDl_ = par("signalDl");
    signalUl_ = par("signalUl");
    numBands_ = binder_->getNumBands();
    numRus_ = par("numRus");

    numPreferredBands_ = par("numPreferredBands");

    if (numRus_ > NUM_RUS)
        throw cRuntimeError(
                "The number of Antennas specified exceeds the limit of %d",
                NUM_RUS);

    //EV << "CellInfo: eNB coordinates: " << nodeX_ << " " << nodeY_ << " " << nodeZ_ << endl;
    //EV << "CellInfo: playground size: " << pgnMinX_ << "," << pgnMinY_ << " - " << pgnMaxX_ << "," << pgnMaxY_ << " " << endl;

    // register the containing eNB  to the binder
    cellId_ = getParentModule()->par("macCellId");

    // first RU to be registered is the MACRO
    ruSet_->addRemoteAntenna(nodeX_, nodeY_, nodebTxPower);

    // deploy RUs
    deployRu(nodeX_, nodeY_, numRus_, ruRange);

    // MCS scaling factor
    calculateMCSScale(&mcsScaleUl_, &mcsScaleDl_);

    createAntennaCwMap();

}


//38.211 Table 4.2.2.1 --> l = Number of symbols
void NRCellInfo::calculateMCSScale(double *mcsUl, double *mcsDl) {
    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs

    int ulRbSubcarriers = par("rbyUl");
    int dlRbSubCarriers = par("rbyDl");

    int ulRbSymbols = rbxDl_;
    int dlRbSymbols = rbxUl_;

    //TODO
    int ulSigSymbols = par("signalUl");
    int dlSigSymbols = par("signalDl");
    int ulPilotRe = par("rbPilotUl");
    int dlPilotRe = par("rbPilotDl");

    *mcsUl = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    *mcsDl = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;
    return;
}

