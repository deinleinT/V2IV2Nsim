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

//
// This file is a part of 5G-Sim-V2I/N
//

#pragma once

#include <omnetpp.h>
#include <unordered_map>
#include "nr/common/NRCommon.h"
#include "common/LteControlInfo.h"


using namespace omnetpp;

struct QosInfo {
	QosInfo() {
	}
	MacNodeId destNodeId;
	MacNodeId senderNodeId;
	IPv4Address senderAddress;
	IPv4Address destAddress;
	ApplicationType appType;
	LteTrafficClass trafficClass;
	unsigned short rlcType;
	unsigned short lcid = 0;
	unsigned short qfi = 0;
	unsigned short _5Qi = 0;
	unsigned short radioBearerId = 0;
	unsigned int cid = 0;
	bool containsSeveralCids = false;
};

//used for Qos-relevant procedures,
class QosHandler: public cSimpleModule {
public:
	QosHandler() {
	}

	virtual ~QosHandler() {
	}

	virtual LteNodeType getNodeType() {
		return nodeType;
	}

	virtual std::unordered_map<unsigned int, QosInfo>& getQosInfo() {
		return QosInfos;
	}

    virtual unsigned short getLcid(unsigned int nodeId, unsigned short msgCat) {
        for (auto const & var : QosInfos) {
            if (var.second.destNodeId == nodeId
                    && var.second.appType == msgCat) {
                return var.second.lcid;
            }
        }
        return 0;
    }

    virtual void deleteNode(unsigned int nodeId) {
        std::vector<unsigned int> tmp;
        for (auto const & var : QosInfos) {
            if (var.second.destNodeId == nodeId
                    || var.second.senderNodeId == nodeId) {
                tmp.push_back(var.first);
            }
        }

        for (auto const & var : tmp) {
            QosInfos.erase(var);
        }
    }

    virtual std::vector<QosInfo> getAllQosInfos(unsigned int nodeId) {
            std::vector<QosInfo> tmp;
            for (auto const & var : QosInfos) {
                if (var.second.destNodeId == nodeId
                        || var.second.senderNodeId == nodeId) {
                    if(var.second.lcid != 0)
                        tmp.push_back(var.second);
                }
            }
            return tmp;
        }

    virtual int numInitStages() const {
        return 2;
    }

    //sorts the qosInfos by its qfi value (from smallest to highest)
    virtual std::vector<std::pair<unsigned int, QosInfo>> getSortedQosInfos() {
        std::vector<std::pair<unsigned int, QosInfo>> tmp;
        for(auto & var : QosInfos){
            tmp.push_back(var);
        }
        std::sort(tmp.begin(), tmp.end(),
                [&](std::pair<unsigned int, QosInfo> & a, std::pair<unsigned int, QosInfo> & b) {return a.second.qfi > b.second.qfi;});
        return tmp;
    }

    virtual void clearQosInfos(){
        QosInfos.clear();
    }

    //sorts the qosInfos by its qfi value (from smallest to highest)
	virtual std::vector<std::pair<unsigned int, QosInfo>> getPrioritySortedQosInfos() {
		std::vector<std::pair<unsigned int, QosInfo>> tmp;
		for (auto &var : QosInfos) {
			tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return (getPriority(a.second._5Qi) > getPriority(b.second._5Qi));
		});
		return tmp;
	}

    virtual void modifyControlInfo(LteControlInfo * info){
        info->setApplication(0);
        MacCid newMacCid = idToMacCid(MacCidToNodeId(info->getCid()),0);
        info->setCid(newMacCid);
        info->setLcid(0);
        info->setQfi(0);
        info->setTraffic(0);
        info->setContainsSeveralCids(true);
        QosInfo tmp;
        tmp.appType = (ApplicationType)0;
        tmp.cid = newMacCid;
        tmp.containsSeveralCids = true;
        tmp.destNodeId = info->getDestId();
        tmp.senderNodeId = info->getSourceId();
        tmp.lcid = 0;
        tmp.trafficClass = (LteTrafficClass)0;
        QosInfos[newMacCid] = tmp;
    }

    virtual ApplicationType getApplicationType(MacCid cid) {
        for (auto & var : QosInfos) {
            if (var.first == cid)
                return (ApplicationType) var.second.appType;
        }
        return (ApplicationType)0;
    }

    virtual unsigned short getQfi(MacCid cid) {
        for (auto & var : QosInfos) {
            if (var.first == cid)
                return var.second.qfi;
        }
        return 0;
    }

    virtual unsigned short get5Qi(MacCid cid) {
		for (auto &var : QosInfos) {
			if (var.first == cid)
				return var.second._5Qi;
		}
		return 0;
	}

    //type is the application type V2X, VOD, VOIP, DATA_FLOW
    //returns the QFI which is pre-configured in QosHandler.ned file
    virtual unsigned short getQfi(ApplicationType type){
        switch(type){
        case V2X:
            return v2xQfi;
        case VOD:
            return videoQfi;
        case VOIP:
            return voipQfi;
        case DATA_FLOW:
            return dataQfi;
        default:
        	return dataQfi;
        }
    }

	virtual unsigned short get5Qi(unsigned short qfi) {
		if (qfi == v2xQfi) {
			return v2x5Qi;
		} else if (qfi == videoQfi) {
			return video5Qi;
		} else if (qfi == voipQfi) {
			return voip5Qi;
		} else if (qfi == dataQfi) {
			return data5Qi;
		} else {
			return data5Qi;
		}
	}

    virtual unsigned short getRadioBearerId(unsigned short qfi){
        if (qfi == v2xQfi)
            return v2xQfiToRadioBearer;
        else if (qfi == videoQfi)
            return videoQfiToRadioBearer;
        else if (qfi == voipQfi)
            return voipQfiToRadioBearer;
        else if (qfi == dataQfi)
            return dataQfiToRadioBearer;
        else
        	return dataQfiToRadioBearer;
    }

    virtual void initQfiParams(){
        v2xQfi = par("v2xQfi").intValue();
        videoQfi = par("videoQfi").intValue();
        voipQfi = par("voipQfi").intValue();
        dataQfi = par("dataQfi").intValue();

	v2x5Qi = par("v2x5Qi").intValue();
	video5Qi = par("video5Qi").intValue();
	voip5Qi = par("voip5Qi").intValue();
	data5Qi = par("data5Qi").intValue();

        v2xQfiToRadioBearer = par("v2xQfiToRadioBearer").intValue();
        videoQfiToRadioBearer = par("videoQfiToRadioBearer").intValue();
        voipQfiToRadioBearer = par("voipQfiToRadioBearer").intValue();
        dataQfiToRadioBearer = par("dataQfiToRadioBearer").intValue();
    }

    virtual double getCharacteristic(std::string characteristic, unsigned short _5Qi) {
		QosCharacteristic qosCharacteristics = NRQosCharacteristics::getNRQosCharacteristics()->getQosCharacteristic(_5Qi);

		if (characteristic == "PDB") {
			return qosCharacteristics.getPdb();
		} else if (characteristic == "PER") {
			return qosCharacteristics.getPer();
		} else if (characteristic == "PRIO") {
			return qosCharacteristics.getPriorityLevel();
		} else {
			throw cRuntimeError("Unknown QoS characteristic");
		}
	}

    virtual unsigned short getPriority(unsigned short _5Qi){
    	return getCharacteristic("PRIO", _5Qi);
    }

    virtual double getPdb(unsigned short _5Qi){
    	return getCharacteristic("PDB", _5Qi);
    }

    virtual double getPer(unsigned short _5Qi){
    	return getCharacteristic("PER", _5Qi);
    }

protected:
    LteNodeType nodeType;
    std::unordered_map<MacCid, QosInfo> QosInfos;
    unsigned short v2xQfi;
    unsigned short videoQfi;
    unsigned short voipQfi;
    unsigned short dataQfi;

    unsigned short v2x5Qi;
    unsigned short video5Qi;
    unsigned short voip5Qi;
    unsigned short data5Qi;

    unsigned short v2xQfiToRadioBearer;
    unsigned short videoQfiToRadioBearer;
    unsigned short voipQfiToRadioBearer;
    unsigned short dataQfiToRadioBearer;

protected:
    virtual void initialize(int stage)=0;
    virtual void handleMessage(cMessage *msg)=0;
};

class QosHandlerUE: public QosHandler {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

class QosHandlerGNB: public QosHandler {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

class QosHandlerUPF: public QosHandler {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

