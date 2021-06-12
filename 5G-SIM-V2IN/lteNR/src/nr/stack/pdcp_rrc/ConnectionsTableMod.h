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


#include "common/LteCommon.h"
#include "stack/pdcp_rrc/ConnectionsTable.h"

/**
 * @class ConnectionsTable
 * @brief For keeping track of connections
 *
 * changed the original hashtable to a vector
 * in the vector the connections are recorded via a
 * struct with the variables
 * srcAddr, dstAddr, srcPort, dstPort, dir, msgCat, lcid
 *
 */
class ConnectionsTableMod : public ConnectionsTable
{
  public:
    ConnectionsTableMod();
    virtual ~ConnectionsTableMod();


    /**
     * find_entry() checks if an entry is in the
     * table and returns a proper number.
     *
     * @param srcAddr part of 4-tuple
     * @param dstAddr part of 4-tuple
     * @param srcPort part of 4-tuple
     * @param dstPort part of 4-tuple
     * @param dir flow direction (DL/UL/D2D)
     * @return value of LCID field in hash table:
     *             - 0xFFFF if no entry was found
     *             - LCID if it was found
     */
    virtual LogicalCid find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir);


    /**
     * create_entry() adds a new entry to the table
     *
     * @param srcAddr part of 4-tuple
     * @param dstAddr part of 4-tuple
     * @param srcPort part of 4-tuple
     * @param dstPort part of 4-tuple
     * @param dir flow direction (DL/UL/D2D)
     * @param LCID connection id to insert
     */
    virtual void create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir,
            LogicalCid lcid);

    virtual void erase_entry(MacNodeId id);

    std::vector<entry_>& getEntries() {
        return entries_;
    }
  protected:

    std::vector<entry_> entries_;
};

