//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified for 5G-Sim-V2I/N
//

#ifndef _LTE_LTEMCS_H_
#define _LTE_LTEMCS_H_

#include <omnetpp.h>

#include "nr/corenetwork/cellInfo/NRCellInfo.h"
#include "common/LteCommon.h"
using namespace std;


// This file contains MCS types and constants;
// and functions related to MCS and Tx-Modes.

struct CQIelem {
    /// Modulation
    LteMod mod_;

    /// Code rate x 1024
    double rate_;

    /// Constructor, with default set to "out of range CQI"
    CQIelem(LteMod mod, double rate) {
        mod_ = mod;
        rate_ = rate;
    }
};

/**
 * Gives the number of layers for each codeword.
 * @param txMode The transmission mode.
 * @param ri The Rank Indication.
 * @param antennaPorts Number of antenna ports
 * @return A vector containing the number of layers per codeword.
 */
std::vector<unsigned char> cwMapping(const TxMode& txMode, const Rank& ri, const unsigned int antennaPorts);

//////////////////////////////////////////////////////////////

//
struct MCSelemNR {
    LteMod mod_;       /// modulation
    double coderate_; /// coderate

    MCSelemNR(LteMod mod = _QPSK, double coderate = 0.0) {
        mod_ = mod;
        coderate_ = coderate;
    }
};

class McsTableNR {
public:

    MCSelemNR table[CQI2ITBSSIZE];

    McsTableNR() {
    }
    ;
    ~McsTableNR() {
    }

    /// MCS table seek operator
    MCSelemNR& at(Tbs tbs) {
        return table[tbs];
    }


    /// MCS Table re-scaling function
    void rescale(const double scale) {
        if (scale <= 0)
            throw cRuntimeError("Bad Rescaling value: %f", scale);

//        for (Tbs i = 0; i < CQI2ITBSSIZE; ++i) {
//            table[i].threshold_ *= (168.0 / scale);
//        }
    }
    ;
};
class McsTableNROne: public McsTableNR {
public:
    McsTableNROne();
    ~McsTableNROne(){
    };
};

class McsTableNRTwo: public McsTableNR {
public:
    McsTableNRTwo();
    ~McsTableNRTwo(){
    };
};

extern const std::vector<Tbs> tbsNRTable;
extern const CQIelem cqiTable[];

unsigned int calcTBS(MacNodeId nodeId, unsigned int numPRB, unsigned short mcsIndex,
        unsigned short numLayers);

unsigned short getQm(LteMod mod);
//

#endif
