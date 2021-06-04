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


#include "stack/mac/amc/LteMcs.h"

#define round(x) floor((x) + 0.5)


std::vector<unsigned char> cwMapping(const TxMode& txMode, const Rank& ri,
        const unsigned int antennaPorts) {
    std::vector<unsigned char> res;

    if (ri <= 1) {
        res.push_back(1);
    } else {
        switch (txMode) {
        // SISO and MU-MIMO supports only rank 1 transmission (1 layer)
        case SINGLE_ANTENNA_PORT0:
        case SINGLE_ANTENNA_PORT5:
        case MULTI_USER: {
            res.push_back(1);
            break;
        }

            // TX Diversity uses a number of layers equal to antennaPorts
        case TRANSMIT_DIVERSITY: {
            res.push_back(antennaPorts);
            break;
        }

            // Spatial MUX uses MIN(RI, antennaPorts) layers
        case OL_SPATIAL_MULTIPLEXING:
        case CL_SPATIAL_MULTIPLEXING: {
            int usedRi = (antennaPorts < ri) ? antennaPorts : ri;
            if (usedRi == 2) {
                res.push_back(1);
                res.push_back(1);
            }
            if (usedRi == 3) {
                res.push_back(1);
                res.push_back(2);
            }
            if (usedRi == 4) {
                res.push_back(2);
                res.push_back(2);
            }
            if (usedRi == 8) {
                res.push_back(4);
                res.push_back(4);
            }
            break;
        }

        default: {
            res.push_back(1);
            break;
        }
        }
    }
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//see 3GPP 38.214, Table 5.2.2.1-2
const CQIelem cqiTable[] = {
        CQIelem(_QPSK, 0.0),
        CQIelem(_QPSK, 78.0),
        CQIelem(_QPSK, 120.0),
        CQIelem(_QPSK, 193.0),
        CQIelem(_QPSK, 308.0),
        CQIelem(_QPSK, 449.0),
        CQIelem(_QPSK, 602.0),
        CQIelem(_16QAM, 378.0),
        CQIelem(_16QAM, 490.0),
        CQIelem(_16QAM, 616.0),
        CQIelem(_64QAM, 466.0),
        CQIelem(_64QAM, 567.0),
        CQIelem(_64QAM, 666.0),
        CQIelem(_64QAM, 772.0),
        CQIelem(_64QAM, 873.0),
        CQIelem(_64QAM, 948.0),
};

//Table from 38.214, 5.1.3.1-1
//used as default table
McsTableNROne::McsTableNROne() {
    table[0] = MCSelemNR(_QPSK, 120.0);
    table[1] = MCSelemNR(_QPSK, 157.0);
    table[2] = MCSelemNR(_QPSK, 193.0);
    table[3] = MCSelemNR(_QPSK, 251.0);
    table[4] = MCSelemNR(_QPSK, 308.0);
    table[5] = MCSelemNR(_QPSK, 379.0);
    table[6] = MCSelemNR(_QPSK, 449.0);
    table[7] = MCSelemNR(_QPSK, 526.0);
    table[8] = MCSelemNR(_QPSK, 602.0);
    table[9] = MCSelemNR(_QPSK, 679.0);
    table[10] = MCSelemNR(_16QAM, 340.0);
    table[11] = MCSelemNR(_16QAM, 378.0);
    table[12] = MCSelemNR(_16QAM, 434.0);
    table[13] = MCSelemNR(_16QAM, 490.0);
    table[14] = MCSelemNR(_16QAM, 553.0);
    table[15] = MCSelemNR(_16QAM, 616.0);
    table[16] = MCSelemNR(_16QAM, 658.0);
    table[17] = MCSelemNR(_64QAM, 438.0);
    table[18] = MCSelemNR(_64QAM, 466.0);
    table[19] = MCSelemNR(_64QAM, 517.0);
    table[20] = MCSelemNR(_64QAM, 567.0);
    table[21] = MCSelemNR(_64QAM, 616.0);
    table[22] = MCSelemNR(_64QAM, 666.0);
    table[23] = MCSelemNR(_64QAM, 719.0);
    table[24] = MCSelemNR(_64QAM, 772.0);
    table[25] = MCSelemNR(_64QAM, 822.0);
    table[26] = MCSelemNR(_64QAM, 873.0);
    table[27] = MCSelemNR(_64QAM, 910.0);
    table[28] = MCSelemNR(_64QAM, 948.0);
}

//Table from 38.214, 6.1.
McsTableNRTwo::McsTableNRTwo() {
    table[0] = MCSelemNR(_QPSK, 120.0 );
    table[1] = MCSelemNR(_QPSK, 157.0);
    table[2] = MCSelemNR(_QPSK, 193.0);
    table[3] = MCSelemNR(_QPSK, 251.0);
    table[4] = MCSelemNR(_QPSK, 308.0);
    table[5] = MCSelemNR(_QPSK, 379.0);
    table[6] = MCSelemNR(_QPSK, 449.0);
    table[7] = MCSelemNR(_QPSK, 526.0);
    table[8] = MCSelemNR(_QPSK, 602.0);
    table[9] = MCSelemNR(_QPSK, 679.0);
    table[10] = MCSelemNR(_16QAM, 340.0);
    table[11] = MCSelemNR(_16QAM, 378.0);
    table[12] = MCSelemNR(_16QAM, 434.0);
    table[13] = MCSelemNR(_16QAM, 490.0);
    table[14] = MCSelemNR(_16QAM, 553.0);
    table[15] = MCSelemNR(_16QAM, 616.0);
    table[16] = MCSelemNR(_16QAM, 658.0);
    table[17] = MCSelemNR(_16QAM, 466.0);
    table[18] = MCSelemNR(_64QAM, 517.0);
    table[19] = MCSelemNR(_64QAM, 567.0);
    table[20] = MCSelemNR(_64QAM, 616.5);
    table[21] = MCSelemNR(_64QAM, 666.0);
    table[22] = MCSelemNR(_64QAM, 719.0);
    table[23] = MCSelemNR(_64QAM, 772.0);
    table[24] = MCSelemNR(_64QAM, 822.0);
    table[25] = MCSelemNR(_64QAM, 873.0);
    table[26] = MCSelemNR(_64QAM, 910.0);
    table[27] = MCSelemNR(_64QAM, 948.0);
    table[28] = MCSelemNR(_64QAM, 948.0);//changed
}

//TABLE 5.1.3.2-1, 38.214
extern const std::vector<Tbs> tbsNRTable = {
        24,
        32,
        40,
        48,
        56,
        64,
        72,
        80,
        88,
        96,
        104,
        112,
        120,
        128,
        136,
        144,
        152,
        160,
        168,
        176,
        184,
        192,
        208,
        224,
        240,
        256,
        272,
        288,
        304,
        320,
        336,
        352,
        368,
        384,
        408,
        432,
        456,
        480,
        504,
        528,
        552,
        576,
        608,
        640,
        672,
        704,
        736,
        768,
        808,
        848,
        888,
        928,
        984,
        1032,
        1064,
        1128,
        1160,
        1192,
        1224,
        1256,
        1288,
        1320,
        1352,
        1416,
        1480,
        1544,
        1608,
        1672,
        1736,
        1800,
        1864,
        1928,
        2024,
        2088,
        2152,
        2216,
        2280,
        2408,
        2472,
        2536,
        2600,
        2664,
        2728,
        2792,
        2856,
        2976,
        3104,
        3240,
        3368,
        3496,
        3624,
        3752,
        3824
};


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

//38.214, 5.1.3.2, UL 6.1.4.2
//table 38.214-5.1.3.1-1 is used
//calculates the TBS in bits for 5G
//tested with https://5g-tools.com/5g-nr-tbs-transport-block-size-calculator/
unsigned int calcTBS(MacNodeId nodeId, unsigned int numPRB, unsigned short mcsIndex,
        unsigned short numLayers) {

    //std::cout << "LteMcs::calcTBS start at " << simTime().dbl() << std::endl;

    Tbs tbs = 0.0;
    McsTableNROne table;

    double coderate = table.at(mcsIndex).coderate_ / 1024; // coderate in table is multiplied with 1024
    LteMod mod = table.at(mcsIndex).mod_;

    //Qm, Modulation order
    unsigned short qM = getQm(mod);

    //Number of SC
    //Step 1: --> 12
    const unsigned short numSC = check_and_cast<NRCellInfo*>(getCellInfo(nodeId))->getRbyDl();

    //--> nSym is the number of symbols per SC and --> 14
    const unsigned short nSym = check_and_cast<NRCellInfo*>(getCellInfo(nodeId))->getDlSymbolsOneMS();//slot symbols
    //

    //const unsigned short nPrbDMRS = check_and_cast<NRCellInfo*>(getCellInfo(nodeId))->getSignalDl();
    const unsigned short nPrbDMRS = 0;
    const unsigned short nPRBoh = 0;

    //N'RE = N RBsc * N shsymb - N PRBDMRS - N PRBoh
    int nRE = numSC * nSym - nPrbDMRS - nPRBoh;
    //N RE
    int NRE = std::min(156, nRE) * numPRB; //numPRB is total number of allocated PRBs for UE

    //step 2: Ninfo
    //N info
    unsigned int nInfo = NRE * coderate * qM * numLayers;
    //N' info
    double NINFO = 0.0;

    if (nInfo <= 3824.0) {
        //step 3;
        double n = std::max(3.0, std::floor(log2(nInfo)) - 6.0);
        NINFO = std::max(24.0, std::pow(2.0, n) * std::floor(nInfo / std::pow(2.0, n)));
        //find closest TBS to N'info
        for (const auto & var : tbsNRTable) {
            if (NINFO <= var) {
                tbs = var;
                break;
            }
        }

    } else { // N info > 3824
        //step 4;
        double n = std::floor(log2(nInfo - 24.0)) - 5.0;
        //N'info
        NINFO = std::max(3840.0, std::pow(2.0, n) * std::round((nInfo - 24.0) / std::pow(2.0, n)));

        if (coderate <= (1.0 / 4.0)) { // R <= 1/4
            double c = std::ceil((NINFO + 24.0) / 3816.0);
            tbs = 8.0 * c * std::ceil((NINFO + 24.0) / (8.0 * c)) - 24.0;
        } else {
            if (NINFO > 8424.0) { //N'info > 8424
                double c = std::ceil((NINFO + 24.0) / 8424.0);
                tbs = 8.0 * c * std::ceil((NINFO + 24.0) / (8.0 * c)) - 24.0;
            } else {
                tbs = 8.0 * std::ceil((NINFO + 24.0) / 8.0) - 24.0;
            }
        }
    }
    //Table 5.1.3.1-2 is used and 28 <= Imcs <= 31 --> not implemented, cause we only use Table 5.1.3.1-1
    //else not considered
    //scaling of Ninfo not implemented

    //std::cout << "LteMcs::calcTBS start at " << simTime().dbl() << std::endl;

    return tbs;
}

//
