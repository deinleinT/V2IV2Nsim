//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2021
// Author: Thomas Deinlein
//

#ifndef _LTE_CONNECTIONSTABLE_H_
#define _LTE_CONNECTIONSTABLE_H_

/// This is the maximum number of allowed connections * 2
#define TABLE_SIZE 2048

#include "common/LteCommon.h"

/**
 * @class ConnectionsTable
 * @brief Hash table to keep track of connections
 *
 * This is an hash table used by the RRC layer
 * to assign CIDs to different connections.
 * The table is in the format:
 *  ______________________________________________________
 * | srcAddr | dstAddr | typeOfService | Direction | LCID |
 *
 * A 4-tuple (plus direction) is used to check if connection was already
 * established and return the proper LCID, otherwise a
 * new entry is added to the table
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
     * @param typeOfService part of 4-tuple
     * @return value of LCID field in hash table:
     *             - 0xFFFF if no entry was found
     *             - LCID if it was found
     */
    virtual LogicalCid find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService);

    /**
     * find_entry() checks if an entry is in the
     * table and returns a proper number.
     *
     * @param srcAddr part of 4-tuple
     * @param dstAddr part of 4-tuple
     * @param typeOfService part of 4-tuple
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
     * @param typeOfService part of 4-tuple
     * @param LCID connection id to insert
     */
    virtual void create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, LogicalCid lcid);

    /**
     * create_entry() adds a new entry to the table
     *
     * @param srcAddr part of 4-tuple
     * @param dstAddr part of 4-tuple
     * @param typeOfService part of 4-tuple
     * @param dir flow direction (DL/UL/D2D)
     * @param LCID connection id to insert
     */
    virtual void create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir, LogicalCid lcid);

    virtual void erase_entry(MacNodeId id){};

  protected:
    /**
     * hash_func() calculates the hash function used
     * by this structure. At the moment it's simply an OR
     * operation between all fields of the 4-tuple
     *
     * @param srcAddr part of 4-tuple
     * @param dstAddr part of 4-tuple
     * @param typeOfService part of 4-tuple
     */
    virtual unsigned int hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService);

    /**
     * hash_func() calculates the hash function used
     * by this structure. At the moment it's simply an OR
     * operation between all fields of the 4-tuple
     *
     * @param srcAddr part of 4-tuple
     * @param dstAddr part of 4-tuple
     * @param typeOfService part of 4-tuple
     * @param dir flow direction (DL/UL/D2D)
     */
    virtual unsigned int hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir);

    /*
     * Data Structures
     */

    /**
     * \struct entry
     * \brief hash table entry
     *
     * This structure contains an entry of the
     * connections hash table. It contains
     * all fields of the 4-tuple and the
     * associated LCID (Logical Connection ID).
     */
    struct entry_
    {
        uint32_t srcAddr_;
        uint32_t dstAddr_;
        uint16_t typeOfService_;
        uint16_t dir_;
        LogicalCid lcid_;
    };
    /// Hash table of size TABLE_SIZE
    entry_ ht_[TABLE_SIZE];
};

#endif
