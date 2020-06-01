/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/stack/sdap/entity/NRSdapEntity.h"

NRSdapEntity::NRSdapEntity()
{
    sequenceNumber_ = 0;
}

NRSdapEntity::~NRSdapEntity()
{
}

unsigned int NRSdapEntity::nextSequenceNumber()
{
    return sequenceNumber_++;
}
