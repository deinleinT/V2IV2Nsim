//
// Part of 5G-Sim-V2I/N
//

package nr.stack.sdap.layer;


moduleinterface INRsdap
{
    parameters:
        @signal[fromUpperLayer];
        @statistic[fromUpperLayer](title="fromUpperLayer"; source="fromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);

        @signal[toUpperLayer];
        @statistic[toUpperLayer](title="toUpperLayer"; source="toUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);

        @signal[fromLowerLayer];
        @statistic[fromLowerLayer](title="fromLowerLayer"; source="fromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);

        @signal[toLowerLayer];
        @statistic[toLowerLayer](title="toLowerLayer"; source="toLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);

        @signal[pkdrop];
        @statistic[droppedBytes](source=sum(packetBytes(pkdrop)); record=last,vector?);
        
    gates:
        inout upperLayer;
        inout lowerLayer;
}

simple NRsdap like INRsdap
{
    parameters:
        int conversationalRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default(1);
        int streamingRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default(1);
        int interactiveRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default(1);
        int backgroundRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default(1);
        @class("NRsdap");
        @display("i=block/filter");

    gates:
        inout upperLayer;
        inout lowerLayer;
}

simple NRsdapUE extends NRsdap
{
    parameters:
        @class("NRsdapUE");
       
}

simple NRsdapGNB extends NRsdap
{
    parameters:
        @class("NRsdapGNB");
        
}
