/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "stack/phy/feedback/LteDlFeedbackGenerator.h"

class NRDlFeedbackGenerator : public LteDlFeedbackGenerator
{
protected:

    virtual void handleMessage(cMessage *msg);
    virtual void initialize(int stage);

};
