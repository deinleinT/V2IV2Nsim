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

#include "stack/mac/buffer/harq_d2d/LteHarqProcessMirrorD2D.h"

LteHarqProcessMirrorD2D::LteHarqProcessMirrorD2D(unsigned int numUnits, unsigned char maxTransmissions)
{
    numUnits_ = numUnits;
    maxTransmissions_ = maxTransmissions;
    status_.resize(numUnits, TXHARQ_PDU_EMPTY);
    pduLength_.resize(numUnits, 0);
    transmissions_.resize(numUnits, 0);
}

LteHarqProcessMirrorD2D::~LteHarqProcessMirrorD2D()
{
}

void LteHarqProcessMirrorD2D::storeFeedback(HarqAcknowledgment harqAck, int64_t pduLength, Codeword cw)
{
    pduLength_[cw] = pduLength;
    transmissions_[cw]++;

    if (harqAck == HARQACK)
    {
        // pdu has been sent and received correctly
        transmissions_[cw] = 0;
        status_[cw] = TXHARQ_PDU_EMPTY;
    }
    else if (harqAck == HARQNACK)
    {
        if (transmissions_[cw] == (maxTransmissions_ + 1))
        {
            transmissions_[cw] = 0;
            status_[cw] = TXHARQ_PDU_EMPTY;
        }
        else
        {
            // pdu_ ready for next transmission
            status_[cw] = TXHARQ_PDU_BUFFERED;
        }
    }
}
