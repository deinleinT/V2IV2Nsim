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

#include "stack/ip2nic/IP2Nic.h"


using namespace std;
using namespace inet;
using namespace omnetpp;

class IP2NR: public IP2Nic {
protected:
    virtual void toStackBs(inet::Packet *datagram) override;
    virtual void toIpBs(inet::Packet *datagram) override;
    virtual void fromIpUe(inet::Packet *datagram) override;
    virtual void initialize(int stage) override;
    virtual void toStackUe(inet::Packet *datagram) override;
    virtual void registerInterface() override;
public:
    virtual void triggerHandoverUe(MacNodeId newMasterId, bool isNr) override;
};
