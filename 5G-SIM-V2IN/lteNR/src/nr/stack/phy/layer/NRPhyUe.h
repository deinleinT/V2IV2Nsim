/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "stack/phy/layer/LtePhyUe.h"
#include "nr/stack/mac/layer/NRMacUe.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "nr/stack/phy/ChannelModel/NRRealisticChannelModel.h"
#include "nr/stack/pdcp_rrc/layer/NRPdcpRrcGnb.h"
#include "nr/stack/pdcp_rrc/layer/NRPdcpRrcUe.h"
#include "nr/stack/sdap/layer/NRsdapGNB.h"
#include "nr/stack/sdap/layer/NRsdapUE.h"
#include "nr/stack/sdap/utils/QosHandler.h"

//see inherit class for method description
class NRPhyUe : public LtePhyUe
{
public:
    NRPhyUe();
    virtual ~NRPhyUe();
    virtual void recordAttenuation(const double & att);
    virtual void recordSNIR(const double & snirVal);
    virtual void recordDistance3d(const double & d3d);
    virtual void recordDistance2d(const double & d2d);
    virtual void recordTotalPer(const double & totalPerVal);
    virtual void recordBler(const double & blerVal);
    virtual void recordSpeed(const double & speedVal);
    virtual void errorDetected();
    virtual void deleteOldBuffers(MacNodeId masterId);
    virtual void exchangeBuffersOnHandover(MacNodeId masterId, MacNodeId newMaster);


protected:
    QosHandler * qosHandler;
    simsignal_t averageTxPower;
    simsignal_t attenuation;
    simsignal_t snir;
    simsignal_t d3d;
    simsignal_t d2d;
    simsignal_t totalPer;
    simsignal_t bler;
    simsignal_t speed;
    int errorCount;
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
};
