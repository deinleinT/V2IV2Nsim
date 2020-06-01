/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/stack/phy/feedback/NRDlFeedbackGenerator.h"

Define_Module(NRDlFeedbackGenerator);

void NRDlFeedbackGenerator::handleMessage(cMessage *msg){
    LteDlFeedbackGenerator::handleMessage(msg);
}

void NRDlFeedbackGenerator::initialize(int stage){
    LteDlFeedbackGenerator::initialize(stage);
}
