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

#include "nr/common/NRCommon.h"

NRBinder* getNRBinder() {

    //std::cout << "NRBinder::getNRBinder start at " << simTime().dbl() << std::endl;

	return check_and_cast<NRBinder*>(getSimulation()->getModuleByPath("binder"));
}


ResourceType convertStringToResourceType(string type){

    //std::cout << "NRBinder::convertStringToResourceType start at " << simTime().dbl() << std::endl;

	if(type == "GBR")
		return GBR;
	else if (type == "NGBR")
		return NGBR;
	else if(type == "DCGBR")
		return DCGBR;
	else
	    return UNKNOWN_RES;
}


NRCellInfo* getNRCellInfo(MacNodeId nodeId)
{
    //std::cout << "NRBinder::getNRCellInfo start at " << simTime().dbl() << std::endl;

    NRBinder* temp = getNRBinder();
    // Check if nodeId is a relay, if nodeId is a eNodeB
    // function GetNextHop returns nodeId
    // TODO change this behavior (its not needed unless we don't implement relays)
    MacNodeId id = temp->getNextHop(nodeId);
    OmnetId omnetid = temp->getOmnetId(id);

    //std::cout << "NRBinder::getNRCellInfo end at " << simTime().dbl() << std::endl;

    return check_and_cast<NRCellInfo*>(getSimulation()->getModule(omnetid)->getSubmodule("cellInfo"));
}

NRQosCharacteristics * NRQosCharacteristics::instance = nullptr;

//REMARK
//added for 5G NR Channel Models
const std::string DeploymentScenarioNRToA(DeploymentScenarioNR type)
{
    int i = 0;
    while (DeploymentScenarioTableNR[i].scenarioNR != UNKNOW_SCENARIO_NR) {
        if (DeploymentScenarioTableNR[i].scenarioNR == type)
            return DeploymentScenarioTableNR[i].scenarioNRName;
        i++;
    }
    return "UNKNOW_SCENARIO_NR";
}

DeploymentScenarioNR aToDeploymentScenarioNR(std::string s)
{
    int i = 0;
    while (DeploymentScenarioTableNR[i].scenarioNR != UNKNOW_SCENARIO_NR) {
        if (DeploymentScenarioTableNR[i].scenarioNRName == s)
            return DeploymentScenarioTableNR[i].scenarioNR;
        i++;
    }
    return UNKNOW_SCENARIO_NR;
}

const std::string NRChannelModelToA(NRChannelModels type)
{
    int i = 0;
    while (NRChannelModelTable[i].channelModel != UNKNOWN) {
        if (NRChannelModelTable[i].channelModel == type)
            return NRChannelModelTable[i].channelModelName;
        i++;
    }
    return "UNKNOWN";
}

NRChannelModels aToNRChannelModel(std::string s)
{
    int i = 0;
    while (NRChannelModelTable[i].channelModel != UNKNOWN) {
        if (NRChannelModelTable[i].channelModelName == s)
            return NRChannelModelTable[i].channelModel;
        i++;
    }
    return UNKNOWN;
}
