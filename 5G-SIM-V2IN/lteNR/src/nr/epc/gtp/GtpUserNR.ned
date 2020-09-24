//
// Part of 5G-Sim-V2I/N
//

package nr.epc.gtp;


//
// Implements a simplified version of the GTP-U protocol
//
simple GtpUserNR
{
    parameters:
        int localPort = default(31);
        int tunnelPeerPort = default(31);
        bool filter = default(true);
        
        @display("i=block/tunnel");

    gates:
        output udpOut;
        input udpIn;

        input trafficFlowFilterGate;
        output pppGate;
        

}

