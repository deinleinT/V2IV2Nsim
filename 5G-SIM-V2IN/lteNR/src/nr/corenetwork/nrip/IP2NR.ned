//
// Part of 5G-Sim-V2I/N
//

package nr.corenetwork.nrip;

import lte.corenetwork.lteip.IP2lte;

simple IP2NR extends IP2lte
{
    parameters:
        @class("IP2NR");
    gates:
        inout stackNR @labels(IPDatagram);
}
