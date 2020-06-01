//
// This file is part of 5G-Sim-V2I/N
//

package nr.stack.sdap.utils;


moduleinterface IQosHandler
{
}

simple QosHandler like IQosHandler
{
    parameters:
        int v2xQfi = default(4);
        int voipQfi = default(3);
        int videoQfi = default(2);
        int dataQfi = default(1);
        int v2xQfiToRadioBearer = default(1);
        int videoQfiToRadioBearer = default(1);
        int voipQfiToRadioBearer = default(1);
        int dataQfiToRadioBearer = default(1);
        @display("i=block/classifier");
}

simple QosHandlerUE extends QosHandler
{
    parameters:
        @class("QosHandlerUE");
        @display("i=block/classifier");

}

simple QosHandlerGNB extends QosHandler
{
    parameters:
        @class("QosHandlerGNB");
        @display("i=block/classifier");

}

simple QosHandlerUPF extends QosHandler
{
    parameters:
        @class("QosHandlerUPF");
        @display("i=block/classifier");

}
