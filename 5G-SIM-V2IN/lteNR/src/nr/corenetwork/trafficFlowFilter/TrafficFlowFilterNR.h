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

#pragma once

#include <omnetpp.h>
#include "corenetwork/trafficFlowFilter/TrafficFlowFilter.h"
#include "nr/common/NRCommon.h"
#include "nr/common/binder/NRBinder.h"
#include "nr/stack/sdap/utils/QosHandler.h"
#include "corenetwork/trafficFlowFilter/TftControlInfo_m.h"

class TrafficFlowFilterNR : public TrafficFlowFilter
{

  protected:
    QosHandler * qosHandler;

    std::string connectedUPF_;

    virtual void initialize(int stage);

    // TrafficFlowFilter module may receive messages only from the input interface of its compound module
    virtual void handleMessage(cMessage *msg);

    // functions for managing filter tables
    TrafficFlowTemplateId findTrafficFlow(inet::L3Address srcAddress, inet::L3Address destAddress);

};
