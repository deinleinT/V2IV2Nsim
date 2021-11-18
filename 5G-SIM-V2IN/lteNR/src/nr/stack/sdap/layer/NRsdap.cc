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

#include "nr/stack/sdap/layer/NRsdap.h"

void NRsdap::handleMessage(cMessage *msg) {

    //std::cout << "NRsdap::handleMessage start at " << simTime().dbl() << std::endl;

    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
    } else if (strcmp(msg->getArrivalGate()->getBaseName(), "upperLayer")
            == 0) {

        fromUpperToLower(msg);

    } else if (strcmp(msg->getArrivalGate()->getBaseName(), "lowerLayer")
            == 0) {

        fromLowerToUpper(msg);

    }

    //std::cout << "NRsdap::handleMessage end at " << simTime().dbl() << std::endl;
}

//sets the qfi for each packet
void NRsdap::setTrafficInformation(cPacket* pkt, FlowControlInfo* lteInfo) {
    //std::cout << "NRsdap::setTrafficInformation start at " << simTime().dbl() << std::endl;

	std::string applName = std::string(pkt->getName());
    if (strcmp(pkt->getName(), "V2X") == 0
    		|| (applName.find("V2X") != string::npos)
			|| (applName.find("v2x") != string::npos)) {
        lteInfo->setApplication(V2X);
        lteInfo->setQfi(qosHandler->getQfi(V2X));
        lteInfo->setRadioBearerId(qosHandler->getRadioBearerId(lteInfo->getQfi()));
    } else if (strcmp(pkt->getName(), "VoIP") == 0
    		|| (applName.find("VoIP") != string::npos)
			|| (applName.find("voip") != string::npos)
			|| (applName.find("Voip") != string::npos)) {
        lteInfo->setQfi(qosHandler->getQfi(VOIP));
        lteInfo->setApplication(VOIP);
        lteInfo->setRadioBearerId(qosHandler->getRadioBearerId(lteInfo->getQfi()));
    } else if (strcmp(pkt->getName(), "Video") == 0
    		|| (applName.find("Video") != string::npos)
			|| (applName.find("video") != string::npos)) {
        lteInfo->setQfi(qosHandler->getQfi(VOD));
        lteInfo->setApplication(VOD);
        lteInfo->setRadioBearerId(qosHandler->getRadioBearerId(lteInfo->getQfi()));
    } else /*if (strcmp(pkt->getName(), "Data") == 0 || strcmp(pkt->getName(), "Data-frag") == 0) */{
        lteInfo->setQfi(qosHandler->getQfi(DATA_FLOW));
        lteInfo->setApplication(DATA_FLOW);
        lteInfo->setRadioBearerId(qosHandler->getRadioBearerId(lteInfo->getQfi()));
    }

    if (nodeType == UE) {
        lteInfo->setDirection(UL);
    } else if (nodeType == ENODEB || nodeType == GNODEB) {
        lteInfo->setDirection(DL);
    } else
        throw cRuntimeError("Unknown nodeType");

    //std::cout << "NRsdap::setTrafficInformation end at " << simTime().dbl() << std::endl;

}

NRSdapEntity * NRsdap::getEntity(MacNodeId sender, MacNodeId dest,
        ApplicationType appType) {

    //std::cout << "NRsdap::setTrafficInformation start at " << simTime().dbl() << std::endl;

    AddressTuple tmp = std::make_tuple(sender,dest,appType);
    if(entities.find(tmp) == entities.end()){
        entities[tmp] = new NRSdapEntity();
    }
    return entities[tmp];

    //std::cout << "NRsdap::setTrafficInformation start at " << simTime().dbl() << std::endl;
}

void NRsdap::finish(){
    for(auto var : entities){
        delete var.second;
    }
    entities.clear();
    recordScalar("hoErrorCounts", hoErrorCount);
}

void NRsdap::deleteEntities(MacNodeId nodeId){

    Enter_Method("deleteEntities");
    std::vector<AddressTuple> tuples;

    for (auto & var : entities) {
        if (std::get<0>(var.first) == nodeId || std::get<1>(var.first) == nodeId){
            tuples.push_back(var.first);
        }
    }

    for(auto & var : tuples){
        delete entities[var];
        entities.erase(var);
    }
}
