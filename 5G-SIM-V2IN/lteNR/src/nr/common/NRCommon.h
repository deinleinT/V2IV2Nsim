//
// SPDX-FileCopyrightText: 2020 Thomas Deinlein <thomas.deinlein@fau.de>
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

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <omnetpp.h>

#include "nr/corenetwork/cellInfo/NRCellInfo.h"
#include "common/LteCommon.h"

#include "nr/corenetwork/binder/NRBinder.h"

class NRBinder;
class NRCellInfo;

NRBinder* getNRBinder();

enum ResourceType {
    GBR, NGBR, DCGBR, UNKNOWN_RES
//GBR -> Guaranteed Bitrate, NGBR -> NonGBR, DCGBR -> Delay-CriticalGBR
};

ResourceType convertStringToResourceType(std::string type);

//QFI --> defined in 23.501, Chapter 5.7
typedef unsigned short QFI;

//Value for 5QI --> TS 23.501, Chapter 5.7
typedef unsigned short QI;

//TS 38.413, 9.3.1.4, unit --> bit/s
typedef unsigned int bitrate;

//*********************************************************

//REMARK
//added for the NR Channel Models
enum DeploymentScenarioNR {
    INDOOR_HOTSPOT_EMBB = 0,
    DENSE_URBAN_EMBB,
    RURAL_EMBB,
    URBAN_MACRO_MMTC,
    URBAN_MACRO_URLLC,
    UNKNOW_SCENARIO_NR
};

struct DeploymentScenarioMappingNR {
    DeploymentScenarioNR scenarioNR;
    std::string scenarioNRName;
};

const DeploymentScenarioMappingNR DeploymentScenarioTableNR[] = {
ELEM(INDOOR_HOTSPOT_EMBB), ELEM(DENSE_URBAN_EMBB),
ELEM(RURAL_EMBB), ELEM(URBAN_MACRO_MMTC), ELEM(URBAN_MACRO_URLLC),
ELEM(UNKNOW_SCENARIO_NR) };

const std::string DeploymentScenarioNRToA(DeploymentScenarioNR type);
DeploymentScenarioNR aToDeploymentScenarioNR(std::string s);

//
enum NRChannelModel {
    InH_A, InH_B, UMa_A, UMa_B, UMi_A, UMi_B, RMa_A, RMa_B, UNKNOWN
};

struct NRChannelModelMapping {
    NRChannelModel channelModel;
    std::string channelModelName;
};

const NRChannelModelMapping NRChannelModelTable[] = { ELEM(InH_A), ELEM(InH_B),
ELEM(UMa_A), ELEM(UMa_B), ELEM(UMi_A), ELEM(UMi_B), ELEM(RMa_A),
ELEM(RMa_B) };

const std::string NRChannelModelToA(NRChannelModel type);
NRChannelModel aToNRChannelModel(std::string s);


// QosCharacteristics to which a 5QI refers to
class QosCharacteristic {
public:
    QosCharacteristic() {
    }
    QosCharacteristic(ResourceType resType, uint16_t priorityLevel, double PDB,
            double PER, uint16_t DMDBV, uint16_t defAveragingWindow) :
            resType(resType), priorityLevel(priorityLevel), PDB(PDB), PER(PER), DMDBV(
                    DMDBV), defAveragingWindow(defAveragingWindow) {
        ASSERT(priorityLevel >= 0 && priorityLevel <= 127);
        ASSERT(DMDBV >= 0 && DMDBV <= 4095);
        ASSERT(defAveragingWindow >= 0 && defAveragingWindow <= 4095);
    }

    ResourceType resType; //GBR,nonGBR,DC-GBR
    uint16_t priorityLevel; // TS 38.413, 9.3.1.84, 1...127
    double PDB; //Packet Delay Budget, in milliseconds,
    double PER; //Packet Error Rate
    uint16_t DMDBV; //Default Maximum Data Burst Volume, in Bytes, 0...4095, 38.413, 9.3.1.83
    uint16_t defAveragingWindow; // in milliseconds, 38.413, 9.3.1.82, 0...4095
};

// see in TS 23.501 ->  Chapter 5.7, Table 5.7.4-1
class NRQosCharacteristics {
public:
    static NRQosCharacteristics * getNRQosCharacteristics() {
        if (instance == nullptr) {
            instance = new NRQosCharacteristics();
        }
        return instance;
    }
    ~NRQosCharacteristics() {
        values.clear();
        instance = nullptr;
    }
    std::map<QI, QosCharacteristic> & getValues() {
        return values;
    }
    QosCharacteristic & getQosCharacteristic(QI value5qi) {
        return values[value5qi];
    }
private:
    static NRQosCharacteristics * instance;
    NRQosCharacteristics() {
    }

    //QosCharacteristics from 23.501 V16.0.2, Table 5.7.4-1, map is filled with values in initialization method in NRBinder
    std::map<QI, QosCharacteristic> values;
};

//QosParameters
class NRQosParameters {
public:
    NRQosParameters() {
    }
    NRQosParameters(QI qi, unsigned short arp, bool reflectiveQosAttribute,
            bool notificationControl, bitrate MFBR_UL, bitrate MFBR_DL,
            bitrate GFBR_DL, bitrate GFBR_UL, bitrate sessionAMBR,
            bitrate ueAMBR, unsigned int MPLR_DL, unsigned int MPLR_UL) {
        qosCharacteristics =
                NRQosCharacteristics::getNRQosCharacteristics()->getQosCharacteristic(
                        qi);
        this->arp = arp;
        this->reflectiveQosAttribute = reflectiveQosAttribute;
        this->notificationControl = notificationControl;
        this->MFBR_UL = MFBR_UL;
        this->GFBR_DL = GFBR_DL;
        this->MFBR_DL = MFBR_DL;
        this->GFBR_UL = GFBR_UL;
        this->MPLR_DL = MPLR_DL;
        this->MPLR_UL = MPLR_UL;
        this->sessionAMBR = sessionAMBR;
        this->ueAMBR = ueAMBR;
        ASSERT(arp >= 1 && arp <= 15);
    }
    QosCharacteristic qosCharacteristics;
    uint16_t arp; //1...15
    bool reflectiveQosAttribute; //optional, non-GBR
    bool notificationControl; //GBR
    bitrate MFBR_UL; //maximum flow bit rate, GBR / DC-GBR only
    bitrate MFBR_DL;
    bitrate GFBR_UL; // guaranteed flow bit rate
    bitrate GFBR_DL;
    bitrate sessionAMBR;
    bitrate ueAMBR;
    uint32_t MPLR_DL; //Maximum Packet Loss Rate
    uint32_t MPLR_UL;
    // maximum packet loss rate
};

//QosProfile --> TS 23.501
class NRQosProfile {
public:
    NRQosProfile(QI qi, unsigned short arp, bool reflectiveQosAttribute,
            bool notificationControl, bitrate MFBR_UL, bitrate MFBR_DL,
            bitrate GFBR_DL, bitrate GFBR_UL, bitrate sessionAMBR,
            bitrate ueAMBR, unsigned int MPLR_DL, unsigned int MPLR_UL) {
        this->qosParam = new NRQosParameters(qi, arp, reflectiveQosAttribute,
                notificationControl, MFBR_UL, MFBR_DL, GFBR_DL, GFBR_UL,
                sessionAMBR, ueAMBR, MPLR_DL, MPLR_UL);
    }

    virtual ~NRQosProfile() {
    }

    NRQosParameters * getQosParameters() {
        return qosParam;
    }
protected:
    NRQosParameters * qosParam;
};

