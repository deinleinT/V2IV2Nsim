/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "nr/common/NRCommon.h"
#include "common/LteControlInfo.h"

class NRSdapEntity
{
    // next sequence number to be assigned
    unsigned int sequenceNumber_;

  public:

    NRSdapEntity();
    virtual ~NRSdapEntity();
    NRSdapEntity(const NRSdapEntity & other){
        this->sequenceNumber_ = other.sequenceNumber_;
    }

    // returns the current sno and increment the next sno
    unsigned int nextSequenceNumber();

    // set the value of the next sequence number
    void setNextSequenceNumber(unsigned int nextSno) { sequenceNumber_ = nextSno; }

    // reset numbering
    void resetSequenceNumber() { sequenceNumber_ = 0; }
};
