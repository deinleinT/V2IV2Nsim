//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU), Computer Science 7 - Computer Networks and Communication Systems
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
 */

#pragma once

#include <omnetpp.h>
#include "nr/stack/sdap/layer/NRsdap.h"
#include "nr/common/NRCommon.h"
#include "common/LteControlInfo.h"
#include "nr/stack/sdap/packet/SdapPdu_m.h"
#include "nr/stack/sdap/utils/QosHandler.h"

using namespace omnetpp;

class NRsdapUE: public NRsdap {
public:
	void recordThroughputDL(unsigned int length){
		throughputDL = throughputDL + length * 8;
	}

	void recordThroughputUL(unsigned int  length){
		throughputUL = throughputUL + length * 8;
	}

protected:
	virtual void initialize(int stage);
	virtual void handleSelfMessage(cMessage *msg);
	virtual void finish() {
		if (throughputTimer) {
			cancelEvent(throughputTimer);
			delete throughputTimer;
			throughputTimer = nullptr;
		}
	}
	virtual void fromLowerToUpper(cMessage *msg);
	virtual void fromUpperToLower(cMessage *msg);
	unsigned int  throughputDL;//in Bits
	unsigned int  throughputUL;
	cOutVector throughputDLvec;
	cOutVector throughputULvec;
    cMessage * throughputTimer;

};
