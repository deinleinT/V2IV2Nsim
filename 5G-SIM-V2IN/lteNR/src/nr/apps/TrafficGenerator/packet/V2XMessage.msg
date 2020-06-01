//
// This is a part of 5G-Sim-V2I/N
//

packet V2XMessage {
    int senderNodeId;
    string senderName;
    string senderIpAddress;
    int senderPort;
    string destIpAddress;
    int destPort;
    unsigned int sequenceNumber;
    
    simtime_t startMeasuringExchangeDelay;
    
}

