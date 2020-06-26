//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU),
// Computer Science 7 - Computer Networks and Communication Systems
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

#include "nr/world/radio/NRChannelControl.h"

Define_Module(NRChannelControl);

NRChannelControl::NRChannelControl() :
        LteChannelControl() {
}

NRChannelControl::~NRChannelControl() {
}

void NRChannelControl::initialize() {
    LteChannelControl::initialize();
}

double NRChannelControl::calcInterfDist() {
    return LteChannelControl::calcInterfDist();
}

void NRChannelControl::sendToChannel(RadioRef srcRadio, AirFrame *airFrame) {
    //std::cout << "NRChannelControl::sendToChannel start at " << simTime().dbl() << std::endl;

    LteChannelControl::sendToChannel(srcRadio, airFrame);

    //std::cout << "NRChannelControl::sendToChannel end at " << simTime().dbl() << std::endl;

}
