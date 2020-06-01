//
// This file is part of 5G-Sim-V2I/N
//


packet VoIPMessage {
    int senderNodeId;
    string senderName;
    string senderIpAddress;
    int senderPort;
    string destIpAddress;
    int destPort;
    int sequenceNumber;
}
