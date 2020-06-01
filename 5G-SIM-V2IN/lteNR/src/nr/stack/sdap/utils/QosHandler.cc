//
// This file is a part of 5G-Sim-V2I/N
//

#include "nr/stack/sdap/utils/QosHandler.h"

Define_Module(QosHandlerUE);
Define_Module(QosHandlerGNB);
Define_Module(QosHandlerUPF);

void QosHandlerUE::initialize(int stage) {

    if (stage == 0) {
        nodeType = UE;
        initQfiParams();
    }

}

void QosHandlerUE::handleMessage(cMessage *msg) {
    // TODO - Generated method body

    //std::cout << "QosHandlerUE::handleMessage start at " << simTime().dbl() << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//GNB
void QosHandlerGNB::initialize(int stage) {

    if (stage == 0) {
        nodeType = GNODEB;
        initQfiParams();
    }
}

void QosHandlerGNB::handleMessage(cMessage *msg) {
    // TODO - Generated method body

    //std::cout << "QosHandlerGNB::handleMessage start at " << simTime().dbl() << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UPF
void QosHandlerUPF::initialize(int stage) {
    if (stage == 0) {
        nodeType = UPF;
        initQfiParams();
    }
}

void QosHandlerUPF::handleMessage(cMessage *msg) {
    // TODO - Generated method body

    //std::cout << "QosHandlerUPF::handleMessage start at " << simTime().dbl() << std::endl;
}
