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

const std::string NRChannelModelToA(NRChannelModel type)
{
    int i = 0;
    while (NRChannelModelTable[i].channelModel != UNKNOWN) {
        if (NRChannelModelTable[i].channelModel == type)
            return NRChannelModelTable[i].channelModelName;
        i++;
    }
    return "UNKNOWN";
}

NRChannelModel aToNRChannelModel(std::string s)
{
    int i = 0;
    while (NRChannelModelTable[i].channelModel != UNKNOWN) {
        if (NRChannelModelTable[i].channelModelName == s)
            return NRChannelModelTable[i].channelModel;
        i++;
    }
    return UNKNOWN;
}
