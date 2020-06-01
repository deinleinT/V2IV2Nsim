
/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/corenetwork/binder/NRBinder.h"

using namespace std;

Define_Module(NRBinder);

NRBinder::NRBinder()
{

}

NRBinder::~NRBinder()
{
    delete qosChar;
}

void NRBinder::testPrintQosValues()
{
    for (const auto & var : qosChar->getValues()) {
        //std::cout << "5QI: " << var.first << endl;
        //std::cout << "ResourceType: " << var.second.resType << endl;
        //std::cout << "PriorityLevel: " << var.second.priorityLevel << endl;
        //std::cout << "PDB: " << var.second.PDB << endl;
        //std::cout << "PER: " << var.second.PER << endl;
        //std::cout << "MaxDataBurst: " << var.second.DMDBV << endl;
        //std::cout << "AvWindow: " << var.second.defAveragingWindow << endl << endl;
    }
}

void NRBinder::initialize(int stages)
{
    LteBinder::initialize(stages);

    if (stages == inet::INITSTAGE_LOCAL) {

        const char * stringValue;
        qosChar = NRQosCharacteristics::getNRQosCharacteristics();
        exchangeBuffersOnHandover = par("exchangeBuffersOnHandover").boolValue();

        //for QosFlows
        vector<int> qiValue;
        vector<string> resourceType;
        vector<int> priorityLevel;
        vector<double> packetDelayBudgetNR; //in s
        vector<double> packetErrorRate;
        vector<int> maxDataBurstVolume; //in bytes
        vector<int> defAveragingWindow; //in s

        stringValue = par("qiValue");
        qiValue = cStringTokenizer(stringValue).asIntVector();
        short sizeQiValue = qiValue.size();

        stringValue = par("resourceType");
        resourceType = cStringTokenizer(stringValue).asVector();
        short sizeResourceType = resourceType.size();
        ASSERT(sizeQiValue == sizeResourceType);

        stringValue = par("priorityLevel");
        priorityLevel = cStringTokenizer(stringValue).asIntVector();
        short sizePriorityLevel = priorityLevel.size();
        ASSERT(sizeResourceType == sizePriorityLevel);

        stringValue = par("packetDelayBudgetNR");
        packetDelayBudgetNR = cStringTokenizer(stringValue).asDoubleVector();
        short sizePacketDelayBudgetNR = packetDelayBudgetNR.size();
        ASSERT(sizePriorityLevel == sizePacketDelayBudgetNR);

        stringValue = par("packetErrorRate");
        packetErrorRate = cStringTokenizer(stringValue).asDoubleVector();
        short sizePacketErrorRate = packetErrorRate.size();
        ASSERT(sizePacketDelayBudgetNR == sizePacketErrorRate);

        stringValue = par("maxDataBurstVolume");
        maxDataBurstVolume = cStringTokenizer(stringValue).asIntVector();
        short sizeMaxDataBurstVolume = maxDataBurstVolume.size();
        ASSERT(sizePacketErrorRate == sizeMaxDataBurstVolume);

        stringValue = par("defAveragingWindow");
        defAveragingWindow = cStringTokenizer(stringValue).asIntVector();
        short sizeDefAveragingWindow = qiValue.size();
        ASSERT(sizeMaxDataBurstVolume == sizeDefAveragingWindow);

        for (short i = 0; i < sizeDefAveragingWindow; i++) {
            qosChar->getValues()[qiValue[i]] = QosCharacteristic(convertStringToResourceType(resourceType[i]),
                    priorityLevel[i], packetDelayBudgetNR[i], packetErrorRate[i], maxDataBurstVolume[i],
                    defAveragingWindow[i]);
        }
    }
}

