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

#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

Define_Module(LtePdcpRrcUe);
Define_Module(LtePdcpRrcEnb);
Define_Module(LtePdcpRrcRelayUe);
Define_Module(LtePdcpRrcRelayEnb);

LtePdcpRrcBase::LtePdcpRrcBase() {
    ht_ = new ConnectionsTable();
    lcid_ = 1;
}

LtePdcpRrcBase::~LtePdcpRrcBase() {
    delete ht_;

    PdcpEntities::iterator it = entities_.begin();
    for (; it != entities_.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }
    entities_.clear();
}

void LtePdcpRrcBase::headerCompress(cPacket* pkt, int headerSize) {
    //std::cout << "LtePdcpRrcBase::headerCompress start at " << simTime().dbl() << std::endl;

    if (headerCompressedSize_ != -1) {
        pkt->setByteLength(
                pkt->getByteLength() - headerSize + headerCompressedSize_);
        //EV << "LtePdcp : Header compression performed\n";
    }

    //std::cout << "LtePdcpRrcBase::headerCompress end at " << simTime().dbl() << std::endl;
}

void LtePdcpRrcBase::headerDecompress(cPacket* pkt, int headerSize) {
    //std::cout << "LtePdcpRrcBase::headerDecompress start at " << simTime().dbl() << std::endl;

    if (headerCompressedSize_ != -1) {
        pkt->setByteLength(
                pkt->getByteLength() + headerSize - headerCompressedSize_);
        //EV << "LtePdcp : Header decompression performed\n";
    }

    //std::cout << "LtePdcpRrcBase::headerDecompress end at " << simTime().dbl() << std::endl;
}


void LtePdcpRrcBase::setTrafficInformation(cPacket* pkt,
        FlowControlInfo* lteInfo) {
    //std::cout << "LtePdcpRrcBase::setTrafficInformation start at " << simTime().dbl() << std::endl;

	std::string applName = std::string(pkt->getName());
    if ((strcmp(pkt->getName(), "VoIP") == 0)
    		|| (applName.find("VoIP") != std::string::npos)
			|| (applName.find("voip") != std::string::npos))   {
        lteInfo->setApplication(VOIP);
        lteInfo->setTraffic(CONVERSATIONAL);
        lteInfo->setRlcType((int) par("conversationalRlc"));
    } else if ((strcmp(pkt->getName(), "gaming")) == 0) {
        lteInfo->setApplication(GAMING);
        lteInfo->setTraffic(INTERACTIVE);
        lteInfo->setRlcType((int) par("interactiveRlc"));
    } else if ((strcmp(pkt->getName(), "VoDPacket") == 0)
            || (strcmp(pkt->getName(), "VoDFinishPacket") == 0)
            || (strcmp(pkt->getName(), "Video") == 0)
			|| (applName.find("Video") != std::string::npos)
			|| (applName.find("video") != std::string::npos)) {
        lteInfo->setApplication(VOD);
        lteInfo->setTraffic(STREAMING);
        lteInfo->setRlcType((int) par("streamingRlc"));
    } else if (strcmp(pkt->getName(), "V2X") == 0
    		|| (applName.find("V2X") != std::string::npos)
			|| (applName.find("v2x") != std::string::npos)) {
        lteInfo->setApplication(V2X);
        lteInfo->setTraffic(V2X_TRAFFIC);
        lteInfo->setRlcType((int) par("conversationalRlc"));
    } else if (strcmp(pkt->getName(), "Data") == 0
    		|| strcmp(pkt->getName(), "Data-frag") == 0
			|| (applName.find("Data") != std::string::npos)
			|| (applName.find("data") != std::string::npos)) {
        lteInfo->setApplication(DATA_FLOW);
        lteInfo->setTraffic(BACKGROUND);
        lteInfo->setRlcType((int) par("backgroundRlc"));
    } else {
        lteInfo->setApplication(CBR);
        lteInfo->setTraffic(BACKGROUND);
        lteInfo->setRlcType((int) par("backgroundRlc"));
    }

    lteInfo->setDirection(getDirection());

    //std::cout << "LtePdcpRrcBase::setTrafficInformation end at " << simTime().dbl() << std::endl;
}

/*
 * Upper Layer handlers
 */

void LtePdcpRrcBase::fromDataPort(cPacket *pkt) {
    //std::cout << "LtePdcpRrcBase::fromDataPort start at " << simTime().dbl() << std::endl;

    emit(receivedPacketFromUpperLayer, pkt);

    // Control Informations
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            pkt->removeControlInfo());

    setTrafficInformation(pkt, lteInfo);
    lteInfo->setDestId(getDestId(lteInfo));
    headerCompress(pkt, lteInfo->getHeaderSize()); // header compression

    // Cid Request
    //EV << "LteRrc : Received CID request for Traffic [ " << "Source: " << IPv4Address(lteInfo->getSrcAddr()) << "@" << lteInfo->getSrcPort() << " Destination: " << IPv4Address(lteInfo->getDstAddr()) << "@" << lteInfo->getDstPort() << " ]\n";


    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
            lteInfo->getSrcPort(), lteInfo->getDstPort(),
            lteInfo->getDirection(), lteInfo->getApplication())) == 0xFFFF) {
        // LCID not found
        mylcid = lcid_++;

        //EV << "LteRrc : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
                lteInfo->getSrcPort(), lteInfo->getDstPort(),
                lteInfo->getDirection(), mylcid, lteInfo->getApplication());
    }

    //EV << "LteRrc : Assigned Lcid: " << mylcid << "\n";
    //EV << "LteRrc : Assigned Node ID: " << nodeId_ << "\n";

    // get the PDCP entity for this LCID

    LtePdcpEntity* entity;
    if(lteInfo->getDirection() == DL)
        entity = getEntity(mylcid, lteInfo->getDirection());
    else
        entity = getEntity(mylcid, nodeId_);

    // get the sequence number for this PDCP SDU.
    // Note that the numbering depends on the entity the packet is associated to.
    unsigned int sno = entity->nextSequenceNumber();

    // set sequence number
    lteInfo->setSequenceNumber(sno);
    // NOTE setLcid and setSourceId have been anticipated for using in "ctrlInfoToMacCid" function
    lteInfo->setLcid(mylcid);
    lteInfo->setSourceId(nodeId_);
    lteInfo->setDestId(getDestId(lteInfo));

    // PDCP Packet creation
    LtePdcpPdu* pdcpPkt = new LtePdcpPdu("LtePdcpPdu");
    pdcpPkt->setByteLength(
        lteInfo->getRlcType() == UM ? PDCP_HEADER_UM : PDCP_HEADER_AM);
    pdcpPkt->encapsulate(pkt);

    //EV << "LtePdcp : Preparing to send " << lteTrafficClassToA((LteTrafficClass) lteInfo->getTraffic()) << " traffic\n";
    //EV << "LtePdcp : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    lteInfo->setSourceId(nodeId_);
    lteInfo->setLcid(mylcid);
    pdcpPkt->setControlInfo(lteInfo);

    //EV << "LtePdcp : Sending packet " << pdcpPkt->getName() << " on port " << (lteInfo->getRlcType() == UM ? "UM_Sap$o\n" : "AM_Sap$o\n");

    // Send message
    send(pdcpPkt, (lteInfo->getRlcType() == UM ? umSap_[OUT] : amSap_[OUT]));
    emit(sentPacketToLowerLayer, pdcpPkt);

    //std::cout << "LtePdcpRrcBase::fromDataPort start at " << simTime().dbl() << std::endl;
}

//void LtePdcpRrcBase::fromEutranRrcSap(cPacket *pkt)
//{
//    // TODO For now use LCID 1000 for Control Traffic coming from RRC
//    FlowControlInfo* lteInfo = new FlowControlInfo();
//    lteInfo->setSourceId(nodeId_);
//    lteInfo->setLcid(1000);
//    lteInfo->setRlcType(TM);
//    pkt->setControlInfo(lteInfo);
//    //EV << "LteRrc : Sending packet " << pkt->getName() << " on port TM_Sap$o\n";
//    send(pkt, tmSap_[OUT]);
//}

/*
 * Lower layer handlers
 */

void LtePdcpRrcBase::toDataPort(cPacket *pkt) {
    //std::cout << "LtePdcpRrcBase::toDataPort start at " << simTime().dbl() << std::endl;

    emit(receivedPacketFromLowerLayer, pkt);
    LtePdcpPdu* pdcpPkt = check_and_cast<LtePdcpPdu*>(pkt);
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
            pdcpPkt->removeControlInfo());

    //EV << "LtePdcp : Received packet with CID " << lteInfo->getLcid() << "\n";
    //EV << "LtePdcp : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    cPacket* upPkt = pdcpPkt->decapsulate(); // Decapsulate packet
    delete pdcpPkt;

    headerDecompress(upPkt, lteInfo->getHeaderSize()); // Decompress packet header
    handleControlInfo(upPkt, lteInfo);

    //EV << "LtePdcp : Sending packet " << upPkt->getName() << " on port DataPort$o\n";
    // Send message
    send(upPkt, dataPort_[OUT]);
    emit(sentPacketToUpperLayer, upPkt);

    //std::cout << "LtePdcpRrcBase::toDataPort end at " << simTime().dbl() << std::endl;
}

/*
 * Main functions
 */

void LtePdcpRrcBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        dataPort_[IN] = gate("DataPort$i");
        dataPort_[OUT] = gate("DataPort$o");
        eutranRrcSap_[IN] = gate("EUTRAN_RRC_Sap$i");
        eutranRrcSap_[OUT] = gate("EUTRAN_RRC_Sap$o");
        tmSap_[IN] = gate("TM_Sap$i");
        tmSap_[OUT] = gate("TM_Sap$o");
        umSap_[IN] = gate("UM_Sap$i");
        umSap_[OUT] = gate("UM_Sap$o");
        amSap_[IN] = gate("AM_Sap$i");
        amSap_[OUT] = gate("AM_Sap$o");

        binder_ = getBinder();
        headerCompressedSize_ = par("headerCompressedSize"); // Compressed size
        nodeId_ = getAncestorPar("macNodeId");

        // statistics
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

        // TODO WATCH_MAP(gatemap_);
        WATCH(headerCompressedSize_);
        WATCH(nodeId_);
        WATCH(lcid_);
    }
}

void LtePdcpRrcBase::handleMessage(cMessage* msg) {
    //std::cout << "LtePdcpRrcBase::handleMessage start at " << simTime().dbl() << std::endl;

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    //EV << "LtePdcp : Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == dataPort_[IN]) {
        fromDataPort(pkt);
    }
//    else if (incoming == eutranRrcSap_[IN])
//    {
//        fromEutranRrcSap(pkt);
//    }
//    else if (incoming == tmSap_[IN])
//    {
//        toEutranRrcSap(pkt);
//    }
    else {
        toDataPort(pkt);
    }

    //std::cout << "LtePdcpRrcBase::handleMessage end at " << simTime().dbl() << std::endl;

}

LtePdcpEntity* LtePdcpRrcBase::getEntity(LogicalCid lcid, MacNodeId ueId) {
    //std::cout << "LtePdcpRrcBase::getEntity start at " << simTime().dbl() << std::endl;

    // Find entity for this LCID
    PdcpEntities::iterator it = entities_.find(lcid);
    if (it == entities_.end()) {
        // Not found: create
        LtePdcpEntity* ent = new LtePdcpEntity(ueId);
        entities_.insert(std::make_pair(lcid, ent));    // Add to entities map

        //EV << "LtePdcpRrcBase::getEntity - Added new PdcpEntity for Lcid: " << lcid << "\n";

        //std::cout << "LtePdcpRrcBase::getEntity end at " << simTime().dbl() << std::endl;

        return ent;
    } else {
        // Found
        //EV << "LtePdcpRrcBase::getEntity - Using old PdcpEntity for Lcid: " << lcid << "\n";

        //std::cout << "LtePdcpRrcBase::getEntity end at " << simTime().dbl() << std::endl;

        for(auto & var : entities_){
            if(var.first == lcid && var.second->getUeId() == ueId){
                return it->second;
            }
        }
    }
}

void LtePdcpRrcBase::finish() {
    // TODO make-finish
}

void LtePdcpRrcEnb::initialize(int stage) {
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        nodeId_ = getAncestorPar("macNodeId");
}

void LtePdcpRrcUe::initialize(int stage) {
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_NETWORK_LAYER_3) {
        nodeId_ = getAncestorPar("macNodeId");
    }
}
