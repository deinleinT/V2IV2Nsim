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

#ifndef _NRMCS_H_
#define _NRMCS_H_

#include "common/LteCommon.h"
#include "stack/mac/amc/LteMcs.h"

// This file contains MCS types and constants; MCS and ITBS tables;
// and functions related to MCS and Tx-Modes.

struct NRMCSelem
{
    LteMod mod_;       /// modulation (Qm)
    double coderate_;  /// coderate (R)

    NRMCSelem(LteMod mod = _QPSK, double coderate = 0.0)
    {
        mod_ = mod;
        coderate_ = coderate;
    }
};

/**
 * <MCS Index> , <Modulation> , <coderate>
 * This table contains value taken from table 5.1.3.1-1 and 5.1.3.1-2 (TS 38.214)
 */
class NRMcsTable
{
    bool extended_;

  public:

    /**
     * <CQI Index [0-15]> , <Modulation> , <Code Rate x 1024>
     * This table contains value taken from the table 7.2.3-1 (TS 38.214)
     */
    CQIelem cqiTable[MAXCQI+1];


    NRMCSelem table[CQI2ITBSSIZE];


    NRMcsTable(bool extended=true);
    ~NRMcsTable()
    {
    }

    CQIelem getCqiElem(int i)
    {
        return cqiTable[i];
    }

    unsigned int getMinIndex(LteMod mod);
    unsigned int getMaxIndex(LteMod mod);

    /// MCS table seek operator
    NRMCSelem& at(Tbs tbs)
    {
        return table[tbs];
    }

    void loadTable(bool extended);

//    /// MCS Table re-scaling function
//    void rescale(const double scale);
};

const unsigned int TBSTABLESIZE = 94;
extern const unsigned int nInfoToTbs[TBSTABLESIZE];


#endif
