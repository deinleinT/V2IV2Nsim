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

#include "../lteCellInfo/LteCellInfo.h"

#include "world/radio/ChannelControl.h"
#include "world/radio/ChannelAccess.h"
#include "inet/mobility/static/StationaryMobility.h"

Define_Module(LteCellInfo);


LteCellInfo::LteCellInfo()
{
    nodeX_ = 0;
    nodeY_ = 0;
    nodeZ_ = 0;
    binder_ = NULL;
    mcsScaleDl_ = 0;
    mcsScaleUl_ = 0;
    numRus_ = 0;
    ruSet_ = new RemoteAntennaSet();
    binder_ = getBinder();
}

LteCellInfo::~LteCellInfo()
{
    binder_ = NULL;
    delete ruSet_;
}


void LteCellInfo::initialize()
{
    pgnMinX_ = par("constraintAreaMinX");
    pgnMinY_ = par("constraintAreaMinY");
    pgnMaxX_ = par("constraintAreaMaxX");
    pgnMaxY_ = par("constraintAreaMaxY");

    int ruRange = par("ruRange");
    double nodebTxPower = getAncestorPar("txPower");
    eNbType_ = par("microCell").boolValue() ? MICRO_ENB : MACRO_ENB;
    numRbDl_ = par("numRbDl");
    numRbUl_ = par("numRbUl");
    rbyDl_ = par("rbyDl");
    rbyUl_ = par("rbyUl");
    rbxDl_ = par("rbxDl");
    rbxUl_ = par("rbxUl");
    rbPilotDl_ = par("rbPilotDl");
    rbPilotUl_ = par("rbPilotUl");
    signalDl_ = par("signalDl");
    signalUl_ = par("signalUl");
    numBands_ = binder_->getNumBands();
    numRus_ = par("numRus");

    numPreferredBands_ = par("numPreferredBands");

    if (numRus_ > NUM_RUS)
        throw cRuntimeError("The number of Antennas specified exceeds the limit of %d", NUM_RUS);

    //EV << "CellInfo: eNB coordinates: " << nodeX_ << " " << nodeY_ << " " << nodeZ_ << endl;
    //EV << "CellInfo: playground size: " << pgnMinX_ << "," << pgnMinY_ << " - " << pgnMaxX_ << "," << pgnMaxY_ << " " << endl;

    // register the containing eNB  to the binder
    cellId_ = getParentModule()->par("macCellId");

    // first RU to be registered is the MACRO
    ruSet_->addRemoteAntenna(nodeX_, nodeY_, nodebTxPower);

    // REFACTORING: has no effect, as long as numRus_ == 0
    // deploy RUs
    deployRu(nodeX_, nodeY_, numRus_, ruRange);

    // MCS scaling factor
    calculateMCSScale(&mcsScaleUl_, &mcsScaleDl_);

    createAntennaCwMap();

    //std::cout << "LteCellInfo::preInitialize end at " << simTime().dbl() << std::endl;
}



void LteCellInfo::calculateNodePosition(double centerX, double centerY, int nTh,
    int totalNodes, int range, double startingAngle, double *xPos,
    double *yPos)
{

    Enter_Method_Silent("calculateNodePosition");

    //std::cout << "LteCellInfo::calculateNodePosition start at " << simTime().dbl() << std::endl;

    if (totalNodes == 0)
        error("LteCellInfo::calculateNodePosition: divide by 0");
    // radians (minus sign because position 0,0 is top-left, not bottom-left)
    double theta = -startingAngle * M_PI / 180;

    double thetaSpacing = (2 * M_PI) / totalNodes;
    // angle of n-th node
    theta += nTh * thetaSpacing;
    double x = centerX + (range * cos(theta));
    double y = centerY + (range * sin(theta));

    *xPos = (x < pgnMinX_) ? pgnMinX_ : (x > pgnMaxX_) ? pgnMaxX_ : x;
    *yPos = (y < pgnMinY_) ? pgnMinY_ : (y > pgnMaxY_) ? pgnMaxY_ : y;

    //EV << NOW << " LteCellInfo::calculateNodePosition: Computed node position " << *xPos << " , " << *yPos << endl;

    //std::cout << "LteCellInfo::calculateNodePosition end at " << simTime().dbl() << std::endl;

    return;
}


void LteCellInfo::deployRu(double nodeX, double nodeY, int numRu, int ruRange)
{
    Enter_Method_Silent("deployRu");

    //std::cout << "LteCellInfo::deployRu start at " << simTime().dbl() << std::endl;

    if (numRu == 0)
        return;
    double x = 0;
    double y = 0;
    double angle = par("ruStartingAngle");
    std::string txPowersString = par("ruTxPower");
    int *txPowers = new int[numRu];
    parseStringToIntArray(txPowersString, txPowers, numRu, 0);
    for (int i = 0; i < numRu; i++)
    {
        calculateNodePosition(nodeX, nodeY, i, numRu, ruRange, angle, &x, &y);
        ruSet_->addRemoteAntenna(x, y, (double) txPowers[i]);
    }
    delete[] txPowers;

    //std::cout << "LteCellInfo::deployRu end at " << simTime().dbl() << std::endl;
}

void LteCellInfo::calculateMCSScale(double *mcsUl, double *mcsDl)
{
    Enter_Method_Silent("calculateMCSScale");

    //std::cout << "LteCellInfo::calculateMCSScale start at " << simTime().dbl() << std::endl;

    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs
    int ulRbSubcarriers = par("rbyUl");
    int dlRbSubCarriers = par("rbyDl");
    int ulRbSymbols = par("rbxUl");
    ulRbSymbols *= 2; // slot --> RB
    int dlRbSymbols = par("rbxDl");
    dlRbSymbols *= 2; // slot --> RB
    int ulSigSymbols = par("signalUl");
    int dlSigSymbols = par("signalDl");
    int ulPilotRe = par("rbPilotUl");
    int dlPilotRe = par("rbPilotDl");

    *mcsUl = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    *mcsDl = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;

    //std::cout << "LteCellInfo::calculateMCSScale end at " << simTime().dbl() << std::endl;

    return;
}

void LteCellInfo::updateMCSScale(double *mcs, double signalRe,
    double signalCarriers, Direction dir)
{
    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs

    int rbSubcarriers = ((dir == DL) ? par("rbyDl") : par("rbyUl"));
    int rbSymbols = ((dir == DL) ? par("rbxDl") : par("rbxUl"));

    rbSymbols *= 2; // slot --> RB

    int sigSymbols = signalRe;
    int pilotRe = signalCarriers;

    *mcs = rbSubcarriers * (rbSymbols - sigSymbols) - pilotRe;
    return;
}

void LteCellInfo::createAntennaCwMap()
{
    Enter_Method_Silent("createAntennaCwMap");

    //std::cout << "LteCellInfo::createAntennaCwMap start at " << simTime().dbl() << std::endl;

    std::string cws = par("antennaCws");
    // values for the RUs including the MACRO
    int dim = numRus_ + 1;
    int *values = new int[dim];
    // default for missing values is 1
    parseStringToIntArray(cws, values, dim, 1);
    for (int i = 0; i < dim; i++)
    {
        antennaCws_[(Remote) i] = values[i];
    }
    delete[] values;

    //std::cout << "LteCellInfo::createAntennaCwMap end at " << simTime().dbl() << std::endl;
}

void LteCellInfo::detachUser(MacNodeId nodeId)
{
    Enter_Method_Silent("detachUser");

    //std::cout << "LteCellInfo::detachUser start at " << simTime().dbl() << std::endl;

    // remove UE from cellInfo's structures

    std::map<MacNodeId, inet::Coord>::iterator pt = uePosition.find(nodeId);
    if (pt != uePosition.end())
        uePosition.erase(pt);

    std::map<MacNodeId, Lambda>::iterator lt = lambdaMap_.find(nodeId);
    if (lt != lambdaMap_.end())
        lambdaMap_.erase(lt);

    //std::cout << "LteCellInfo::detachUser end at " << simTime().dbl() << std::endl;
}

void LteCellInfo::attachUser(MacNodeId nodeId)
{
    Enter_Method_Silent("attachUser");

    //std::cout << "LteCellInfo::attachUser start at " << simTime().dbl() << std::endl;

    // add UE to cellInfo's structures (lambda maps)
    // position will be added by the eNB while computing feedback

    int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
    lambdaInit(nodeId, index);

    //std::cout << "LteCellInfo::attachUser end at " << simTime().dbl() << std::endl;
}
