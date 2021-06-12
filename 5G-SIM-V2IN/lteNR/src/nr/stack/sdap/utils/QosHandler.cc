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

void QosHandlerUE::handleMessage(cMessage * msg) {
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

void QosHandlerGNB::handleMessage(cMessage * msg) {
	// TODO - Generated method body

	//std::cout << "QosHandlerGNB::handleMessage start at " << simTime().dbl() << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UPF
void QosHandlerUPF::initialize(int stage) {
	if (stage == 0) {
		nodeType = USER_PLANE_FUNCTION;
		initQfiParams();
	}
}

void QosHandlerUPF::handleMessage(cMessage * msg) {
	// TODO - Generated method body

	//std::cout << "QosHandlerUPF::handleMessage start at " << simTime().dbl() << std::endl;
}
