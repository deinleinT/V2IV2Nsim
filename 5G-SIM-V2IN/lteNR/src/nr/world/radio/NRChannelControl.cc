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

double NRChannelControl::calcInterfDist()
{
    double interfDistance;

    //the carrier frequency used
    double carrierFrequency = par("carrierfrequency").doubleValue() * 1000000000;
    //maximum transmission power possible
    double pMax = par("pMax");
    //signal attenuation threshold
    double sat = par("sat");
    //path loss coefficient
    double alpha = par("alpha");

    double waveLength = (SPEED_OF_LIGHT / carrierFrequency);
    //minimum power level to be able to physically receive a signal
    double minReceivePower = pow(10.0, sat / 10.0);

    interfDistance = pow(waveLength * waveLength * pMax / (16.0 * M_PI * M_PI * minReceivePower), 1.0 / alpha);

    //EV << "max interference distance:" << interfDistance << endl;

    return interfDistance;
}

void NRChannelControl::sendToChannel(RadioRef srcRadio, AirFrame *airFrame) {
    //std::cout << "NRChannelControl::sendToChannel start at " << simTime().dbl() << std::endl;

    LteChannelControl::sendToChannel(srcRadio, airFrame);

    //std::cout << "NRChannelControl::sendToChannel end at " << simTime().dbl() << std::endl;

}

double NRChannelControl::calcInterfDist(double pMax){
    double interfDistance;

    //the carrier frequency used
    double carrierFrequency = par("carrierfrequency").doubleValue() * 1000000000;
    //signal attenuation threshold
    double sat = par("sat");
    //path loss coefficient
    double alpha = par("alpha");

    double waveLength = (SPEED_OF_LIGHT / carrierFrequency);
    //minimum power level to be able to physically receive a signal
    double minReceivePower = pow(10.0, sat / 10.0);

    interfDistance = pow(waveLength * waveLength * pMax / (16.0 * M_PI * M_PI * minReceivePower), 1.0 / alpha);

    //EV << "max interference distance:" << interfDistance << endl;

    return interfDistance;
}
