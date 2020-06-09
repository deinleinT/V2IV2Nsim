//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2020
// Author: Thomas Deinlein
//

#ifndef _LTE_CONNECTIONSTABLE_H_
#define _LTE_CONNECTIONSTABLE_H_


#include "common/LteCommon.h"

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
class ConnectionsTable
{
  public:
    ConnectionsTable();
    virtual ~ConnectionsTable();


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
    LogicalCid find_entry(uint32_t srcAddr, uint32_t dstAddr,
        uint16_t srcPort, uint16_t dstPort, uint16_t dir, unsigned char msgCat);


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
    void create_entry(uint32_t srcAddr, uint32_t dstAddr,
        uint16_t srcPort, uint16_t dstPort, uint16_t dir, LogicalCid lcid, unsigned char msgCat);

    void erase_entry(uint32_t nodeAddress);

    struct entry_ {
        uint32_t srcAddr_;
        uint32_t dstAddr_;
        uint16_t srcPort_;
        uint16_t dstPort_;
        uint16_t dir_;
        unsigned char msgCat; //messageCategory
        LogicalCid lcid_;
    };

    std::vector<entry_>& getEntries() {
        return entries_;
    }
  private:

    std::vector<entry_> entries_;
};

#endif
