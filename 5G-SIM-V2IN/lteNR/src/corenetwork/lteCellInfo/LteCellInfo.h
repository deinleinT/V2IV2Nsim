//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2020
// Author: Thomas Deinlein
//

#pragma once

#include <string.h>
#include <omnetpp.h>
#include <math.h>
#include "inet/networklayer/contract/ipv4/IPv4Address.h"
#include "stack/phy/das/RemoteAntennaSet.h"
#include "common/LteCommon.h"
#include "corenetwork/binder/LteBinder.h"

class DasFilter;

/**
 * @class LteCellInfo
 * @brief There is one LteCellInfo module for each eNB (thus one for each cell). Keeps cross-layer information about the cell
 */
class LteCellInfo : public cSimpleModule
{
  protected:
    /// reference to the global module binder
    LteBinder *binder_;
    /// Remote Antennas for eNB
    RemoteAntennaSet *ruSet_;

    /// Cell Id
    MacCellId cellId_;

    // MACRO_ENB or MICRO_ENB
    EnbType eNbType_;

    /// x playground lower bound
    double pgnMinX_;
    /// y playground lower bound
    double pgnMinY_;
    /// x playground upper bound
    double pgnMaxX_;
    /// y playground upper bound
    double pgnMaxY_;
    /// z playground size
//    double pgnZ_;
    /// x eNB position
    double nodeX_;
    /// y eNB position
    double nodeY_;
    /// z eNB position
    double nodeZ_;

    /// Number of DAS RU
    int numRus_;
    /// Remote and its CW
    std::map<Remote, int> antennaCws_;

    /// number of cell logical bands
    int numBands_;
    /// number of preferred bands to use (meaningful only in PREFERRED mode)
    int numPreferredBands_;

    /// number of RB, DL
    int numRbDl_;
    /// number of RB, UL
    int numRbUl_;
    /// number of sub-carriers per RB, DL
    int rbyDl_;
    /// number of sub-carriers per RB, UL
    int rbyUl_;
    /// number of OFDM symbols per slot, DL
    int rbxDl_;
    /// number of OFDM symbols per slot, UL
    int rbxUl_;
    /// number of pilot REs per RB, DL
    int rbPilotDl_;
    /// number of pilot REs per RB, UL
    int rbPilotUl_;
    /// number of signaling symbols for RB, DL
    int signalDl_;
    /// number of signaling symbols for RB, UL
    int signalUl_;
    /// MCS scale UL
    double mcsScaleUl_;
    /// MCS scale DL
    double mcsScaleDl_;

    // Position of each UE
    std::map<MacNodeId, inet::Coord> uePosition;

    std::map<MacNodeId, Lambda> lambdaMap_;
    protected:

    virtual void initialize();

    virtual void handleMessage(cMessage *msg)
    {
    }

    /**
     * Deploys remote antennas.
     *
     * This is a virtual deployment: the cellInfo needs only to inform
     * the eNB nic module about the position of the deployed antennas and
     * their TX power. These parameters are configured via the cellInfo, but
     * no NED module is created here.
     *
     * @param nodeX x coordinate of the center of the master node
     * @param nodeY y coordinate of the center of the master node
     * @param numRu number of remote units to be deployed
     * @param ruRange distance between eNB and RUs
     */
    virtual void deployRu(double nodeX, double nodeY, int numRu, int ruRange);
    virtual void calculateMCSScale(double *mcsUl, double *mcsDl);
	virtual void updateMCSScale(double *mcs, double signalRe, double signalCarriers = 0, Direction dir = DL);

  protected:
    /**
     * Calculates node position around a circumference.
     *
     * @param centerX x coordinate of the center
     * @param centerY y coordinate of the center
     * @param nTh ordering number of the UE to be placed in this circumference
     * @param totalNodes total number of nodes that will be placed
     * @param range circumference range
     * @param startingAngle angle of the first deployed node (degrees)
     * @param[out] xPos calculated x coordinate
     * @param[out] yPos calculated y coordinate
     */
    // Used by remote Units only
    void calculateNodePosition(double centerX, double centerY, int nTh,
        int totalNodes, int range, double startingAngle, double *xPos,
        double *yPos);

    void createAntennaCwMap();

  public:

    LteCellInfo();

    // Getters
    int getNumRbDl()
    {
        Enter_Method_Silent("getNumRbDl");

        //std::cout << "LteCellInfo::getNumRbDl at " << simTime().dbl() << std::endl;

        return numRbDl_;
    }
    int getNumRbUl()
    {
        Enter_Method_Silent("getNumRbUl");

        //std::cout << "LteCellInfo::getNumRbUl at " << simTime().dbl() << std::endl;

        return numRbUl_;
    }
    int getRbyDl()
    {
        Enter_Method_Silent("getRbyDl");

        //std::cout << "LteCellInfo::getRbyDl at " << simTime().dbl() << std::endl;

        return rbyDl_;
    }
    int getRbyUl()
    {
        Enter_Method_Silent("getRbyUl");

        //std::cout << "LteCellInfo::getRbyUl at " << simTime().dbl() << std::endl;

        return rbyUl_;
    }
    int getRbxDl()
    {
        Enter_Method_Silent("getRbxDl");

        //std::cout << "LteCellInfo::getRbxDl at " << simTime().dbl() << std::endl;

        return rbxDl_;
    }
    int getRbxUl()
    {
        Enter_Method_Silent("getRbxUl");

        //std::cout << "LteCellInfo::getRbxUl at " << simTime().dbl() << std::endl;

        return rbxUl_;
    }
    int getRbPilotDl()
    {
        Enter_Method_Silent("getRbPilotDl");

        //std::cout << "LteCellInfo::getRbPilotDl at " << simTime().dbl() << std::endl;

        return rbPilotDl_;
    }
    int getRbPilotUl()
    {
        Enter_Method_Silent("getRbPilotUl");

        //std::cout << "LteCellInfo::getRbPilotUl at " << simTime().dbl() << std::endl;

        return rbPilotUl_;
    }
    int getSignalDl()
    {
        Enter_Method_Silent("getSignalDl");

        //std::cout << "LteCellInfo::getSignalDl at " << simTime().dbl() << std::endl;

        return signalDl_;
    }
    int getSignalUl()
    {
        Enter_Method_Silent("getSignalUl");

        //std::cout << "LteCellInfo::getSignalUl at " << simTime().dbl() << std::endl;

        return signalUl_;
    }
    int getNumBands()
    {
        Enter_Method_Silent("getNumBands");

        //std::cout << "LteCellInfo::getNumBands at " << simTime().dbl() << std::endl;

        return numBands_;
    }
    double getMcsScaleUl()
    {
        Enter_Method_Silent("getMcsScaleUl");

        //std::cout << "LteCellInfo::getMcsScaleUl at " << simTime().dbl() << std::endl;

        return mcsScaleUl_;
    }
    double getMcsScaleDl()
    {
        Enter_Method_Silent("getMcsScaleDl");

        //std::cout << "LteCellInfo::getMcsScaleDl at " << simTime().dbl() << std::endl;

        return mcsScaleDl_;
    }
    int getNumRus()
    {
        Enter_Method_Silent("getNumRus");

        //std::cout << "LteCellInfo::getNumRus at " << simTime().dbl() << std::endl;

        return numRus_;
    }

    MacCellId getCellId()
    {
        Enter_Method_Silent("getCellId");

        //std::cout << "LteCellInfo::getCellId at " << simTime().dbl() << std::endl;

        return cellId_;
    }

    std::map<Remote, int> getAntennaCws()
    {
        Enter_Method_Silent("getAntennaCws");

        //std::cout << "LteCellInfo::getAntennaCws at " << simTime().dbl() << std::endl;

        return antennaCws_;
    }

    int getNumPreferredBands()
    {
        Enter_Method_Silent("getNumPreferredBands");

        //std::cout << "LteCellInfo::getNumPreferredBands at " << simTime().dbl() << std::endl;

        return numPreferredBands_;
    }

    RemoteAntennaSet* getRemoteAntennaSet()
    {
        Enter_Method_Silent("getRemoteAntennaSet");

        //std::cout << "LteCellInfo::getRemoteAntennaSet at " << simTime().dbl() << std::endl;

        return ruSet_;
    }

    void setEnbType(EnbType t)
    {
        Enter_Method_Silent("setEnbType");

        //std::cout << "LteCellInfo::setEnbType at " << simTime().dbl() << std::endl;

        eNbType_ = t;
    }

    EnbType getEnbType()
    {
        Enter_Method_Silent("getEnbType");

        //std::cout << "LteCellInfo::getEnbType at " << simTime().dbl() << std::endl;

        return eNbType_;
    }

    inet::Coord getUePosition(MacNodeId id)
    {
        Enter_Method_Silent("getUePosition");

        //std::cout << "LteCellInfo::getUePosition at " << simTime().dbl() << std::endl;

        return uePosition[id];
    }
    void setUePosition(MacNodeId id, inet::Coord c)
    {
        Enter_Method_Silent("setUePosition");

        //std::cout << "LteCellInfo::setUePosition at " << simTime().dbl() << std::endl;

        uePosition[id] = c;
    }

    // changes eNb position (used for micro deployment)
    void setEnbPosition(inet::Coord c)
    {
        Enter_Method_Silent("setEnbPosition");

        //std::cout << "LteCellInfo::setEnbPosition at " << simTime().dbl() << std::endl;

        nodeX_ = c.x;
        nodeY_ = c.y;
    }

    void lambdaUpdate(MacNodeId id, unsigned int index)
    {
        Enter_Method_Silent("lambdaUpdate");

        //std::cout << "LteCellInfo::lambdaUpdate at " << simTime().dbl() << std::endl;

        lambdaMap_[id].lambdaMax = binder_->phyPisaData.getLambda(index, 0);
        lambdaMap_[id].index = index;
        lambdaMap_[id].lambdaMin = binder_->phyPisaData.getLambda(index, 1);
        lambdaMap_[id].lambdaRatio = binder_->phyPisaData.getLambda(index, 2);
    }
    void lambdaIncrease(MacNodeId id, unsigned int i)
    {
        Enter_Method_Silent("lambdaIncrease");

        //std::cout << "LteCellInfo::lambdaIncrease at " << simTime().dbl() << std::endl;

        lambdaMap_[id].index = lambdaMap_[id].lambdaStart + i;
        lambdaUpdate(id, lambdaMap_[id].index);
    }
    void lambdaInit(MacNodeId id, unsigned int i)
    {
        Enter_Method_Silent("lambdaInit");

        //std::cout << "LteCellInfo::lambdaInit at " << simTime().dbl() << std::endl;

        lambdaMap_[id].lambdaStart = i;
        lambdaMap_[id].index = lambdaMap_[id].lambdaStart;
        lambdaUpdate(id, lambdaMap_[id].index);
    }
    void channelUpdate(MacNodeId id, unsigned int in)
    {
        Enter_Method_Silent("channelUpdate");

        //std::cout << "LteCellInfo::channelUpdate at " << simTime().dbl() << std::endl;

        unsigned int index = in % binder_->phyPisaData.maxChannel2();
        lambdaMap_[id].channelIndex = index;
    }
    void channelIncrease(MacNodeId id)
    {
        Enter_Method_Silent("channelIncrease");

        //std::cout << "LteCellInfo::channelIncrease at " << simTime().dbl() << std::endl;

        unsigned int i = getNumBands();
        channelUpdate(id, lambdaMap_[id].channelIndex + i);
    }
    Lambda* getLambda(MacNodeId id)
    {
        Enter_Method_Silent("getLambda");

        //std::cout << "LteCellInfo::getLambda at " << simTime().dbl() << std::endl;

        return &(lambdaMap_.at(id));
    }

    std::map<MacNodeId, Lambda>* getLambda()
    {
        Enter_Method_Silent("getLambda");

        //std::cout << "LteCellInfo::getLambda at " << simTime().dbl() << std::endl;

        return &lambdaMap_;
    }

    std::vector<MacNodeId> getConnectedUes() {
		Enter_Method_Silent("getConnectedUes");

		//std::cout << "LteCellInfo::getConnectedUes at " << simTime().dbl() << std::endl;
		std::vector<MacNodeId> tmp;
		for (auto & var : lambdaMap_) {
			tmp.push_back(var.first);
		}

		return tmp;
	}
    //---------------------------------------------------------------

    void detachUser(MacNodeId nodeId);
    void attachUser(MacNodeId nodeId);

    ~LteCellInfo();
};


