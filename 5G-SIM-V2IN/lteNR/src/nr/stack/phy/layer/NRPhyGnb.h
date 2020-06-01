/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include <omnetpp.h>
#include "stack/phy/layer/LtePhyEnb.h"
#include "nr/common/NRCommon.h"
#include "nr/stack/phy/ChannelModel/NRRealisticChannelModel.h"
#include "nr/stack/sdap/utils/QosHandler.h"

using namespace omnetpp;

//see inherit class for method description
class NRPhyGnb : public LtePhyEnb
{
    friend class DasFilter;
public:
    NRPhyGnb();
    virtual ~NRPhyGnb();
    //virtual LteRealisticChannelModel* initializeChannelModel(ParameterMap& params);
    virtual void recordAttenuation(const double & att);
    virtual void recordSNIR(const double & snirVal);
    virtual void recordDistance3d(const double & d3d);
    virtual void recordDistance2d(const double & d2d);
    virtual void recordTotalPer(const double & totalPer);
    virtual void recordBler(const double & blerVal);
    virtual void recordSpeed(const double & speedVal);
    virtual void errorDetected();


protected:

    simsignal_t averageTxPower;
    simsignal_t attenuation;
    simsignal_t snir;
    simsignal_t d3d;
    simsignal_t d2d;
    simsignal_t totalPer;
    simsignal_t bler;
    simsignal_t speed;

    QosHandler * qosHandler;

    int errorCount;

    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
};
