//
// Part of 5G-Sim-V2I/N
//


package nr.stack.mac;

import lte.stack.mac.*;




//
// User Equipment MAC layer of LTE stack (Realistic)
//
simple NRMacUe extends LteMacUe like LteMac
{
    @class("NRMacUe");
}



//
// eNodeB MAC layer of LTE stack (Realistic)
//
simple NRMacGnb extends LteMacEnb like LteMac
{
    @class("NRMacGnb");
    int gNodeBCount = default(0);
}

