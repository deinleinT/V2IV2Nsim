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
#include "stack/pdcp_rrc/layer/NRPdcpRrcEnb.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "nr/stack/sdap/utils/QosHandler.h"

// see inherit class for method description
class NRPdcpRrcGnb : public NRPdcpRrcEnb {
public:
    virtual void resetConnectionTable(MacNodeId masterId, MacNodeId nodeId);

protected:
    QosHandler * qosHandler;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void fromDataPort(cPacket *pkt);
    virtual void toDataPort(cPacket * pkt);
    virtual void setTrafficInformation(omnetpp::cPacket* pkt, FlowControlInfo* lteInfo);
    virtual void sendToLowerLayer(inet::Packet *pkt);
    virtual bool isDualConnectivityEnabled() { return true; }

};
