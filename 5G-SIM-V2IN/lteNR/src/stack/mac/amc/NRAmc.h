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

#ifndef _NRAMC_H_
#define _NRAMC_H_

#include <omnetpp.h>

#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/NRMcs.h"

/**
 * @class NRAMC
 * @brief NR AMC module for Omnet++ simulator
 *
 * TBS determination based on 3GPP TS 38.214 v15.6.0 (June 2019)
 */
class NRAmc : public LteAmc
{
    unsigned int getSymbolsPerSlot(double carrierFrequency, Direction dir);
    unsigned int getResourceElementsPerBlock(unsigned int symbolsPerSlot);
    unsigned int getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot);
    unsigned int computeTbsFromNinfo(double nInfo, double coderate);

    unsigned int computeCodewordTbs(UserTxParams *info, Codeword cw, Direction dir, unsigned int numRe);

    //added
    unsigned short getQm(LteMod mod){
        unsigned short qM = 0;
        switch (mod) {
            case _QPSK:
                qM = 2;
                break;
            case _16QAM:
                qM = 4;
                break;
            case _64QAM:
                qM = 6;
                break;
            case _256QAM:
                qM = 8;
                break;
            }
        return qM;
    }

public:

    //set useExtendedMcsTable to true to use 38.214-5.1.3.1-2
    //calculates the TBS in bits for 5G
    //tested with https://5g-tools.com/5g-nr-tbs-transport-block-size-calculator/
    virtual void printTBS()
    {
        if (omnetpp::getSimulation()->getSystemModule()->hasPar("printTBS")) {
            if (omnetpp::getSimulation()->getSystemModule()->par("printTBS").boolValue()) {
                if (omnetpp::getSimulation()->getSystemModule()->hasPar("useExtendedMcsTable")) {
                    unsigned int max = 28;
                    if (omnetpp::getSimulation()->getSystemModule()->par("useExtendedMcsTable").boolValue()) {
                        std::cout << "Print the calculated TBS for Table 38.214-5.1.3.1-2" << std::endl;
                        for (unsigned int k = 0; k <= 27; ++k) {
                            for (unsigned int i = 1; i <= 273; ++i) {

                                NRMcsTable *mcsTable = &dlNrMcsTable_;
                                NRMCSelem elem = mcsTable->at(k);
                                std::vector<unsigned char> layers;
                                layers.push_back(1);
                                unsigned int numRe = getResourceElements(i, getSymbolsPerSlot(2, DL));
//                                unsigned int modFactor = 2 << elem.mod_;
                                unsigned int modFactor = getQm(elem.mod_);
                                double coderate = elem.coderate_ / 1024;
                                double nInfo = numRe * coderate * modFactor * layers.at(0);
                                unsigned int tbs = computeTbsFromNinfo(std::floor(nInfo), coderate);

                                std::cout << "Number of Resource Blocks: " << i << " | MCS Index Table 2: " << k
                                        << " | Layer: " << 1 << " || Calculated TBS: " << tbs << std::endl;
                            }
                        }
                    }
                    else {
                        std::cout << "Print the calculated TBS for Table 38.214-5.1.3.1-1" << std::endl;
                        for (unsigned int k = 0; k <= max; ++k) {
                            for (unsigned int i = 1; i <= 273; ++i) {
                                NRMcsTable *mcsTable = &dlNrMcsTable_;
                                NRMCSelem elem = mcsTable->at(k);
                                std::vector<unsigned char> layers;
                                layers.push_back(1);
                                unsigned int numRe = getResourceElements(i, getSymbolsPerSlot(2, DL));
//                                unsigned int modFactor = 2 << elem.mod_;
                                unsigned int modFactor = getQm(elem.mod_);
                                double coderate = elem.coderate_ / 1024;
                                double nInfo = numRe * coderate * modFactor * layers.at(0);
                                unsigned int tbs = computeTbsFromNinfo(floor(nInfo), coderate);

                                std::cout << "Number of Resource Blocks: " << i << " | MCS Index Table 1: " << k
                                        << " | Layer: " << 1 << " || Calculated TBS: " << tbs << std::endl;
                            }
                        }
                    }
                }
                throw omnetpp::cRuntimeError("End Simulation after printing all TBS results!");
            }
        }
    }

    NRMcsTable dlNrMcsTable_;    // TODO tables for UL and DL should be different
    NRMcsTable ulNrMcsTable_;
    NRMcsTable d2dNrMcsTable_;

    NRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas, bool extendedMcsTable);
    virtual ~NRAmc();

    NRMCSelem getMcsElemPerCqi(Cqi cqi, const Direction dir);
    unsigned int getMcsIndexCqi(Cqi cqi, const Direction dir);

    virtual unsigned int computeReqRbs(MacNodeId id, Band b, Codeword cw, unsigned int bytes, const Direction dir,
            double carrierFrequency);
    virtual unsigned int computeReqRbs(MacNodeId id, Codeword cw, unsigned int bytes, const Direction dir,
            unsigned int blocks, double carrierFrequency);

    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir,
            double carrierFrequency);
    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir,
            double carrierFrequency);
//    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
//    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency);

//    // multiband version of the above function. It returns the number of bytes that can fit in the given "blocks" of the given "band"
//    virtual unsigned int computeBytesOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
//    virtual unsigned int computeBitsOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
};

#endif
