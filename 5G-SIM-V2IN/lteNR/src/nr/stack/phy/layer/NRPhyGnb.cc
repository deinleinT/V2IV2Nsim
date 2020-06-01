/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#include "nr/stack/phy/layer/NRPhyGnb.h"

Define_Module(NRPhyGnb);

NRPhyGnb::NRPhyGnb() :
        LtePhyEnb() {

}

NRPhyGnb::~NRPhyGnb() {

}

void NRPhyGnb::initialize(int stage) {
    LtePhyEnb::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {

        averageTxPower = registerSignal("averageTxPower");
        attenuation = registerSignal("attenuation");
        snir = registerSignal("snir");
        d2d = registerSignal("d2d");
        d3d = registerSignal("d3d");
        totalPer = registerSignal("totalPer");
        bler = registerSignal("bler");
        speed = registerSignal("speed");


        emit(averageTxPower, txPower_);
        errorCount = 0;

        qosHandler = check_and_cast<QosHandlerGNB*>(getParentModule()->getSubmodule("qosHandler"));

    }
}

void NRPhyGnb::recordAttenuation(const double & att) {
    emit(attenuation, att);
}

void NRPhyGnb::recordSNIR(const double & snirVal) {
    emit(snir, snirVal);
}

void NRPhyGnb::recordDistance3d(const double & d3dVal) {
    emit(d3d, d3dVal);
}

void NRPhyGnb::recordDistance2d(const double & d2dVal) {
    emit(d2d, d2dVal);
}

void NRPhyGnb::handleMessage(cMessage *msg) {

////RRC Handling
//    if (strcmp(msg->getName(), "RRC") == 0) {
//        if (msg->getArrivalGate()->getId() == radioInGate_) {
//            //RRC Message from a UE
//            //TODO
//        }
//
//        // message from stack
//        else if (msg->getArrivalGate()->getId() == upperGateIn_) {
//            //send RRC Message to a UE
//            //TODO
//        }
//    }
    LtePhyEnb::handleMessage(msg);
}

void NRPhyGnb::recordTotalPer(const double & totalPerVal) {
    emit(totalPer, totalPerVal);
}

void NRPhyGnb::recordBler(const double & blerVal) {
    emit(bler, blerVal);
}

void NRPhyGnb::recordSpeed(const double & speedVal) {
    emit(speed, speedVal);
}

void NRPhyGnb::errorDetected() {
    ++errorCount;
}

void NRPhyGnb::finish() {
    recordScalar("#errorCounts", errorCount);
}
