//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nürnberg (FAU),
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

#pragma once

#include <omnetpp.h>
#include <string>

#include "corenetwork/binder/LteBinder.h"
#include "nr/common/NRCommon.h"
#include "veins/modules/obstacle/ObstacleControl.h"


using std::string;
using std::vector;

class NRQosCharacteristics;

class NRBinder: public LteBinder {
public:
    NRBinder();
    virtual ~NRBinder();
    void testPrintQosValues();
    virtual bool checkIsNLOS(const inet::Coord & sender, const inet::Coord & receiver, const double hBuilding){
        veins::ObstacleControl * tmp = check_and_cast<veins::ObstacleControl*>(getSimulation()->getModuleByPath("obstacles"));
        veins::Coord send(sender.x,sender.y,sender.z);
        veins::Coord rec(receiver.x,receiver.y,receiver.z);
        return tmp->isNLOS(send, rec, hBuilding);
    }
    virtual double calculateAttenuationPerCutAndMeter(const Coord& senderPos, const Coord& receiverPos) const{
        veins::ObstacleControl * tmp = check_and_cast<veins::ObstacleControl*>(getSimulation()->getModuleByPath("obstacles"));
        veins::Coord send(senderPos.x,senderPos.y,senderPos.z);
        veins::Coord rec(receiverPos.x,receiverPos.y,receiverPos.z);
        return tmp->calculateAttenuation(send, rec);
    }
    virtual void setExchangeBuffersHandoverFlag(bool flag){
        this->exchangeBuffersOnHandover = flag;
    }

    virtual bool getExchangeOnBuffersHandoverFlag(){
        return exchangeBuffersOnHandover;
    }
protected:
    NRQosCharacteristics * qosChar;
    bool exchangeBuffersOnHandover;
    virtual void initialize(int stages);
};

