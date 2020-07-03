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
 * currently not in usage
 *
*/

#include "nr/stack/rrc/layer/Rrc.h"

Define_Module(Rrc);

void Rrc::initialize()
{
//    getSimulation()->getModuleByPath("lteInternet");
//    lteInternet->getSubmodule("mux");
    //nodeType
    if(strcmp(par("nodeType").stringValue(),"UE")==0){
        nodeType = UE;
    }else if(strcmp(par("nodeType").stringValue(),"GNODEB")==0){
        nodeType = GNODEB;
    }else{
        throw cRuntimeError("Unknown NodeType");
    }
    incomingGate = gate("gateRRC$i");
    outgoingGate = gate("gateRRC$o");

}

void Rrc::handleMessage(cMessage *msg) {

    //std::cout << "Rrc::handleMessage start at " << simTime().dbl() << std::endl;

	if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
    }else{
        incomingRRC(msg);
    }

    //std::cout << "Rrc::handleMessage end at " << simTime().dbl() << std::endl;
}

void Rrc::handleSelfMessage(cMessage *msg){

    //std::cout << "Rrc::handleSelfMessage start at " << simTime().dbl() << std::endl;

    cMessage * tmp = new RrcMsg("RRC");
    cancelAndDelete(msg);
    send(tmp,outgoingGate);

    //std::cout << "Rrc::handleSelfMessage end at " << simTime().dbl() << std::endl;
}

void Rrc::incomingRRC(cMessage *& msg){

    //std::cout << "Rrc::incomingRRC start at " << simTime().dbl() << std::endl;
	//TODO

    //std::cout << "Rrc::incomingRRC end at " << simTime().dbl() << std::endl;

}
