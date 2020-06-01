/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/world/radio/NRChannelControl.h"

Define_Module(NRChannelControl);

NRChannelControl::NRChannelControl() :
        LteChannelControl() {
}

NRChannelControl::~NRChannelControl() {
}

void NRChannelControl::initialize() {
    LteChannelControl::initialize();
}

double NRChannelControl::calcInterfDist() {
    return LteChannelControl::calcInterfDist();
}

void NRChannelControl::sendToChannel(RadioRef srcRadio, AirFrame *airFrame) {
    //std::cout << "NRChannelControl::sendToChannel start at " << simTime().dbl() << std::endl;

    LteChannelControl::sendToChannel(srcRadio, airFrame);

    //std::cout << "NRChannelControl::sendToChannel end at " << simTime().dbl() << std::endl;

}
