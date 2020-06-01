//
// Part of 5G-Sim-V2I/N
//

package nr.stack.rrc;

simple Rrc
{
    parameters:
        @class("Rrc");
        bool on = default(false);
        string nodeType;

    gates:
        inout gateRRC;
}
