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

#include "nr/stack/pdcp_rrc/ConnectionsTableMod.h"

ConnectionsTableMod::ConnectionsTableMod() {
	// Table is resetted by putting all fields equal to 0xFF
	//memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
	entries_.clear();
}

LogicalCid ConnectionsTableMod::find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir) {

	for (auto &var : entries_) {
		if (var.srcAddr_ == srcAddr)
			if (var.dstAddr_ == dstAddr)
				if (var.dir_ == dir)
					if (var.typeOfService_ == typeOfService)
						return var.lcid_;
	}
	return 0xFFFF;
}

void ConnectionsTableMod::create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir, LogicalCid lcid) {

	bool alreadyCreated = (find_entry(srcAddr, dstAddr, typeOfService, dir) != 0xFFFF);

	if (alreadyCreated) {
		return;
	}
	else {
		entry_ tmp;
		tmp.srcAddr_ = srcAddr;
		tmp.dstAddr_ = dstAddr;
		tmp.lcid_ = lcid;
		tmp.typeOfService_ = typeOfService;
		tmp.dir_ = dir;
		entries_.push_back(tmp);
	}
}

void ConnectionsTableMod::erase_entry(MacNodeId id) {

	//std::cout << "ConnectionsTable::erase_entry start at " << simTime().dbl() << std::endl;

	std::vector<entry_>::const_iterator it;
	for (it = entries_.begin(); it != entries_.end();) {
		if (MacCidToNodeId((*it).lcid_) == id) {
			it = entries_.erase(it);
		}
		else {
			++it;
		}
	}

	//std::cout << "ConnectionsTable::erase_entry end at " << simTime().dbl() << std::endl;
}

ConnectionsTableMod::~ConnectionsTableMod() {
//    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
	entries_.clear();
}
