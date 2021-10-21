//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

/*
 * This file has been modified for 5G-SIM-V2I/N
 */

#ifndef _LTE_TFTCONTROLINFO_H_
#define _LTE_TFTCONTROLINFO_H_

#include <omnetpp.h>

using namespace omnetpp;

class TftControlInfo: public cObject {
protected:
    unsigned int tft_;
    unsigned short msgCategory;
    unsigned short qfi;
    unsigned short _5Qi;
    unsigned short radioBearerId;

public:
    TftControlInfo();
    virtual ~TftControlInfo();

    void setTft(unsigned int tft) {
        tft_ = tft;
    }
    unsigned int getTft() {
        return tft_;
    }

    void setMsgCategory(unsigned short msgCat) {
        msgCategory = msgCat;
    }
    unsigned short getMsgCategory() {
        return msgCategory;
    }

	void setQfi(unsigned short qfi) {
		this->qfi = qfi;
	}

	unsigned short getQfi() {
		return qfi;
	}

	void set5Qi(unsigned short value) {
		this->_5Qi = value;
	}

	unsigned short get5Qi() {
		return _5Qi;
	}

    void setRadioBearerId(unsigned short rbId) {
        this->radioBearerId = rbId;
    }
    unsigned short getRadioBearerId() {
        return radioBearerId;
    }
};

#endif

