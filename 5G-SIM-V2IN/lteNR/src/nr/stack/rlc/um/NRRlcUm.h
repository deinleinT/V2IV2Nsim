/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "stack/rlc/um/LteRlcUm.h"
#include "nr/stack/phy/layer/NRPhyUe.h"
#include "nr/stack/phy/layer/NRPhyGnb.h"

//see inherit class for method description
class NRRlcUm: public LteRlcUm {

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
	virtual void sendDefragmented(cPacket *pkt);
	virtual void handleLowerMessage(cPacket *pkt);
    virtual void handleUpperMessage(cPacket *pkt);

    simsignal_t UEtotalRlcThroughputDlMean;
    simsignal_t UEtotalRlcThroughputUlMean;

public:
	virtual void recordTotalRlcThroughputUl(double length) {
		this->totalRcvdBytesUl += length;
		double tp = totalRcvdBytesUl / (NOW - getSimulation()->getWarmupPeriod());
		totalRlcThroughputUl.record(tp);
		emit(UEtotalRlcThroughputUlMean,tp);
	}

	virtual void recordTotalRlcThroughputDl(double length) {
		this->totalRcvdBytesDl += length;
		double tp = totalRcvdBytesDl / (NOW - getSimulation()->getWarmupPeriod());
		totalRlcThroughputDl.record(tp);
		emit(UEtotalRlcThroughputDlMean,tp);
	}

};

