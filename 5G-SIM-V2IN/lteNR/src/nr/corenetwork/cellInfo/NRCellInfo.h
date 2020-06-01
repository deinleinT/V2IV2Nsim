/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "corenetwork/lteCellInfo/LteCellInfo.h"

class NRCellInfo : public LteCellInfo
{
protected:
    int ulSymbolsOneMS;
    int dlSymbolsOneMS;

protected:
    virtual void initialize();
    virtual void calculateMCSScale(double *mcsUl, double *mcsDl);
public:
    virtual int getUlSymbolsOneMS() {
        return ulSymbolsOneMS;
    }
    ;
    virtual int getDlSymbolsOneMS() {
        return dlSymbolsOneMS;
    }
    ;
//    virtual void updateMCSScale(double *mcs, double signalRe, double signalCarriers = 0, Direction dir = DL);
};

