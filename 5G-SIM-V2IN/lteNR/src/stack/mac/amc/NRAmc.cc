//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2021
// Author: Thomas Deinlein
//

#include "stack/mac/amc/NRAmc.h"

using namespace std;
using namespace omnetpp;

/********************
 * PUBLIC FUNCTIONS
 ********************/

NRAmc::NRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas, bool flag) :
        LteAmc(mac, binder, cellInfo, numAntennas)
{
    dlNrMcsTable_.loadTable(flag);
    ulNrMcsTable_.loadTable(flag);
}

NRAmc::~NRAmc()
{
}

unsigned int NRAmc::getSymbolsPerSlot(double carrierFrequency, Direction dir)
{
    unsigned totSymbols = 14;   // TODO get this parameter from CellInfo/Carrier

    if (!omnetpp::getSimulation()->getSystemModule()->par("printTBS").boolValue()) {
        // use a function from the binder
        SlotFormat sf = binder_->getSlotFormat(carrierFrequency);
        if (!sf.tdd)
            return totSymbols;

        // TODO handle FLEX symbols: so far, they are used as guard (hence, not used for scheduling)
        if (dir == DL)
            return sf.numDlSymbols;
        // else UL
        return sf.numUlSymbols;
    }
    else {
        return totSymbols;
    }
}

unsigned int NRAmc::getResourceElementsPerBlock(unsigned int symbolsPerSlot)
{
    unsigned int numSubcarriers = 12;   // TODO get this parameter from CellInfo/Carrier
    unsigned int reSignal = 0;
    unsigned int nOverhead = 0;

    return (numSubcarriers * symbolsPerSlot) - reSignal - nOverhead;
}

unsigned int NRAmc::getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot)
{
    unsigned int numRePerBlock = getResourceElementsPerBlock(symbolsPerSlot);

    if (numRePerBlock >= 156)
        return 156 * blocks;

    return numRePerBlock * blocks;
}

//added pow instead of shift
unsigned int NRAmc::computeTbsFromNinfo(double nInfo, double coderate)
{
    unsigned int tbs = 0;
    unsigned int _nInfo = 0;
    unsigned int n = 0;
    if (nInfo <= 3824) {
//        n = std::max((int) 3, (int) (floor(log2(nInfo) - 6))); --> original simu5G Code
        n = std::max((int) 3, (int) (std::floor(log2(nInfo)) - 6));
//        _nInfo = std::max((unsigned int) 24.0, (unsigned int) ((1 << n) * floor(nInfo / (1 << n)))); --> original simu5G Code
        _nInfo = std::max((unsigned int) 24.0, (unsigned int) (std::pow(2.0, n) * floor(nInfo / std::pow(2.0, n))));

        // get tbs from table
        unsigned int j = 0;
        for (j = 0; j < TBSTABLESIZE - 1; j++) {
            if (nInfoToTbs[j] >= _nInfo)
                break;
        }

        tbs = nInfoToTbs[j];
    }
    else {
        double C;
//        n = floor(log2(nInfo - 24.0) - 5.0); --> original simu5G Code
        n = std::floor(log2(nInfo - 24.0)) - 5.0;
//        _nInfo = (1 << n) * round((nInfo - 24.0) / (1 << n)); --> original simu5G Code
        _nInfo = std::max(3840.0, std::pow(2.0, n) * std::round((nInfo - 24.0) / std::pow(2.0, n)));
        if (coderate <= 0.25) {
            C = std::ceil((_nInfo + 24.0) / 3816.0);
            tbs = 8.0 * C * std::ceil((_nInfo + 24.0) / (8.0 * C)) - 24.0;
        }
        else {
//            if (_nInfo >= 8424.0) { --> original simu5G Code
            if (_nInfo > 8424.0) {
                C = std::ceil((_nInfo + 24.0) / 8424.0);
                tbs = 8.0 * C * std::ceil((_nInfo + 24.0) / (8.0 * C)) - 24.0;
            }
            else {
                tbs = 8.0 * std::ceil((_nInfo + 24.0) / 8.0) - 24.0;
            }
        }
    }
    return tbs;
}

unsigned int NRAmc::computeCodewordTbs(UserTxParams *info, Codeword cw, Direction dir, unsigned int numRe)
{
    std::vector<unsigned char> layers = info->getLayers();
    NRMCSelem mcsElem = getMcsElemPerCqi(info->readCqiVector().at(cw), dir);
    //unsigned int modFactor = 2 << mcsElem.mod_; --> lead to conversion errors
    unsigned int modFactor = getQm(mcsElem.mod_);
    double coderate = mcsElem.coderate_ / 1024;
    double nInfo = numRe * coderate * modFactor * layers.at(cw);

    return computeTbsFromNinfo(floor(nInfo), coderate);
}

/*******************************************
 *      Scheduler interface functions      *
 *******************************************/

unsigned int NRAmc::computeReqRbs(MacNodeId id, Band b, Codeword cw, unsigned int bytes, const Direction dir,
        double carrierFrequency)
{
    EV << NOW << " NRAmc::computeReqRbs Node " << id << ", Band " << b << ", Codeword " << cw << ", direction "
            << dirToA(dir) << endl;

    if (bytes == 0) {
        // DEBUG
        EV << NOW << " NRAmc::computeReqRbs Occupation: 0 bytes\n";
        EV << NOW << " NRAmc::computeReqRbs Number of RBs: 0\n";

        return 0;
    }

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    unsigned int bits = bytes * 8;
    unsigned int numRe = getResourceElements(1, getSymbolsPerSlot(carrierFrequency, dir));

    // Computing RB occupation
    unsigned int j = 0;
    for (j = 0; j < 110; ++j)   // TODO check number of blocks
        if (computeCodewordTbs(&info, cw, dir, numRe) >= bits)
            break;

    // DEBUG
    EV << NOW << " NRAmc::computeReqRbs Occupation: " << bytes << " bytes , CQI : " << info.readCqiVector().at(cw)
            << " \n";
    EV << NOW << " NRAmc::computeReqRbs Number of RBs: " << j + 1 << "\n";

    return j + 1;
}

//added
unsigned int NRAmc::computeReqRbs(MacNodeId id, Codeword cw, unsigned int bytes, const Direction dir,
        unsigned int avBlocks, double carrierFrequency)
{

    if (bytes == 0) {
        return 0;
    }

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    unsigned int bits = bytes * 8;

    // Computing RB occupation
    unsigned int j = 0;
    for (; j < avBlocks; j++) {
        unsigned int numRe = getResourceElements(j, getSymbolsPerSlot(carrierFrequency, dir));
        if (computeCodewordTbs(&info, cw, dir, numRe) >= bits)
            break;
    }

    if(j <= 0){
        std::cout << "";
    }

    return j+1;
}

unsigned int NRAmc::computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir,
        double carrierFrequency)
{
    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Node: " << id << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Band: " << b << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Direction: " << dirToA(dir) << "\n";

    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    unsigned int bits = 0;
    unsigned int codewords = info.getLayers().size();
    for (Codeword cw = 0; cw < codewords; ++cw) {
        // if CQI == 0 the UE is out of range, thus bits=0
        if (info.readCqiVector().at(cw) == 0) {
            EV << NOW << " NRAmc::computeBitsOnNRbs - CQI equal to zero on cw " << cw << ", return no blocks available"
                    << endl;
            continue;
        }

        unsigned int tbs = computeCodewordTbs(&info, cw, dir, numRe);
        bits += tbs;
    }

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Resource Blocks: " << blocks << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Available space: " << bits << "\n";

    return bits;
}

unsigned int NRAmc::computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir,
        double carrierFrequency)
{
    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Node: " << id << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Band: " << b << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Codeword: " << cw << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Direction: " << dirToA(dir) << "\n";

    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    // if CQI == 0 the UE is out of range, thus return 0
    if (info.readCqiVector().at(cw) == 0) {
        EV << NOW << " NRAmc::computeBitsOnNRbs - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }

    unsigned int tbs = computeCodewordTbs(&info, cw, dir, numRe);

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Resource Blocks: " << blocks << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Available space: " << tbs << "\n";

    return tbs;
}

NRMCSelem NRAmc::getMcsElemPerCqi(Cqi cqi, const Direction dir)
{
    // CQI threshold table selection
    NRMcsTable *mcsTable;
    if (dir == DL)
        mcsTable = &dlNrMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulNrMcsTable_;
    else {
        throw omnetpp::cRuntimeError("NRAmc::getIMcsPerCqi(): Unrecognized direction");
    }
    CQIelem entry = mcsTable->getCqiElem(cqi);
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the McsTable (extended reporting supported)
    unsigned int min = mcsTable->getMinIndex(mod);
    unsigned int max = mcsTable->getMaxIndex(mod);

    // Initialize the working variables at the minimum value.
    NRMCSelem ret = mcsTable->at(min);

    // Search in the McsTable from min to max until the rate exceeds
    // the coderate in an entry of the table.
    for (unsigned int i = min; i <= max; i++) {
        NRMCSelem elem = mcsTable->at(i);
        if (elem.coderate_ <= rate)
            ret = elem;
        else
            break;
    }

    // Return the MCSElem found.
    return ret;
}

unsigned int NRAmc::getMcsIndexCqi(Cqi cqi, const Direction dir)
{
    // CQI threshold table selection
    NRMcsTable *mcsTable;
    if (dir == DL)
        mcsTable = &dlNrMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulNrMcsTable_;
    else {
        throw omnetpp::cRuntimeError("NRAmc::getIMcsPerCqi(): Unrecognized direction");
    }
    CQIelem entry = mcsTable->getCqiElem(cqi);
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the McsTable (extended reporting supported)
    unsigned int min = mcsTable->getMinIndex(mod);
    unsigned int max = mcsTable->getMaxIndex(mod);

    // Initialize the working variables at the minimum value.
    //NRMCSelem ret = mcsTable->at(min);
    unsigned int index = 0;

    // Search in the McsTable from min to max until the rate exceeds
    // the coderate in an entry of the table.
    for (unsigned int i = min; i <= max; i++) {
        NRMCSelem elem = mcsTable->at(i);
        if (elem.coderate_ <= rate)
            index = i;
        else
            break;
    }

    return index;
}
