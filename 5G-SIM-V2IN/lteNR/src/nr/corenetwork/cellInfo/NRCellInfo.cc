//
// SPDX-FileCopyrightText: 2020 Thomas Deinlein <thomas.deinlein@fau.de>
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
    //NRCellInfo::initialize();

    int numerology = par("numerology").intValue();
    if (numerology == 15) {
        //in 5g 1 Slot == 1 ms, in LTE 1 Slot == 0.5 ms, in 5G 1 Slot consists always of 14 Symbols!
        //7 symbols per 0.5ms --> also applied in 5G
        //slot in Simulte is 0.5ms
        rbxDl_ = 7;
        rbxUl_ = 7;
    } else if (numerology == 30) {
        //1 Slot is 0.5ms
        rbxDl_ = 14;
        rbxUl_ = 14;
    } else if (numerology == 60) {
        //1 Slot is 0.25ms
        rbxDl_ = 28;
        rbxUl_ = 28;
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
    rbyDl_ = par("rbyDl");
    rbyUl_ = par("rbyUl");
    rbxDl_ = par("rbxDl");
    rbxUl_ = par("rbxUl");
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

    int numerology = par("numerology").intValue();
    int ulRbSubcarriers = par("rbyUl");
    int dlRbSubCarriers = par("rbyDl");

    int ulRbSymbols;
    int dlRbSymbols;

    if (numerology == 15) {
        //in 5g 1 Slot == 1 ms, in LTE 1 Slot == 0.5 ms, in 5G 1 Slot consists always of 14 Symbols!
        //7 symbols per 0.5ms --> also applied in 5G
        //slot in Simulte is 0.5ms, TTI is 1
        dlRbSymbols = 7;
        ulRbSymbols = 7;
    } else if (numerology == 30) {
        //1 Slot is 0.5ms
        dlRbSymbols = 14;
        ulRbSymbols = 14;
    } else if (numerology == 60) {
        //1 Slot is 0.25ms
        dlRbSymbols = 28;
        ulRbSymbols = 28;
    } else
        throw cRuntimeError(
                "Unknown numerology of %d, possible values are 15, 30 or 60");

	//see 38.211 Table 4.2.2.1
	ulRbSymbols *= 2; // slot --> RB
	dlRbSymbols *= 2; // slot --> RB

    ulSymbolsOneMS = ulRbSymbols;
    dlSymbolsOneMS = dlRbSymbols;

    //TODO
    int ulSigSymbols = par("signalUl");
    int dlSigSymbols = par("signalDl");
    int ulPilotRe = par("rbPilotUl");
    int dlPilotRe = par("rbPilotDl");

    *mcsUl = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    *mcsDl = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;
    return;
}

