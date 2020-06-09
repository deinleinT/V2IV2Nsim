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

#include "stack/pdcp_rrc/ConnectionsTable.h"

ConnectionsTable::ConnectionsTable() {
    // Table is resetted by putting all fields equal to 0xFF
    //memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
    entries_.clear();
}


LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr,
        uint16_t srcPort, uint16_t dstPort, uint16_t dir, unsigned char msgCat) {

    for (auto & var : entries_) {
        if (var.srcAddr_ == srcAddr)
            if (var.dstAddr_ == dstAddr)
                if (var.dir_ == dir)
                    if (var.srcPort_ == srcPort)
                        if (var.dstPort_ == dstPort)
                            if (var.msgCat == msgCat)
                                return var.lcid_;
    }
    return 0xFFFF;
}



void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr,
        uint16_t srcPort, uint16_t dstPort, uint16_t dir, LogicalCid lcid, unsigned char msgCat) {

    bool alreadyCreated = (find_entry(srcAddr, dstAddr, srcPort, dstPort, dir, msgCat) != 0xFFFF);


    if (alreadyCreated) {
        return;
    } else {
        entry_ tmp;
        tmp.srcAddr_ = srcAddr;
        tmp.dstAddr_ = dstAddr;
        tmp.srcPort_ = srcPort;
        tmp.dstPort_ = dstPort;
        tmp.lcid_ = lcid;
        tmp.msgCat = msgCat;
        tmp.dir_ = dir;
        entries_.push_back(tmp);
    }
}

void ConnectionsTable::erase_entry(uint32_t nodeAddress) {

    //std::cout << "ConnectionsTable::erase_entry start at " << simTime().dbl() << std::endl;

    int position = -1;
    while (position == -1) {

        for (unsigned int i = 0; i < entries_.size(); i++) {
            if (entries_[i].dstAddr_ == nodeAddress
                    || entries_[i].srcAddr_ == nodeAddress) {
                position = i;
                break;
            }
        }

        if (position != -1) {
            entries_.erase(entries_.begin() + position);
            position = -1;
        } else {
            break;
        }
    }
    //std::cout << "ConnectionsTable::erase_entry end at " << simTime().dbl() << std::endl;
}

ConnectionsTable::~ConnectionsTable() {
//    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
    entries_.clear();
}
