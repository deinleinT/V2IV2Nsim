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

#ifndef _LTE_LTEMAXCICOMP_H_
#define _LTE_LTEMAXCICOMP_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "common/LteCommon.h"

class LteMaxCiComp : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    // get the bandLimit for the given user
    // return false if ALL bands are usable
    bool getBandLimit(std::vector<BandLimit>* bandLimit, MacNodeId ueId);

  public:

    virtual void prepareSchedule();

    virtual void commitSchedule();

};

#endif // _LTE_LTEMAXCICOMP_H_
