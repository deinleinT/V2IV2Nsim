//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nuernberg (FAU), Computer Science 7 - Computer Networks and Communication Systems
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

//
// Part of 5G-Sim-V2I/N
//

package lteNR.nr.stack.sdap.layer;


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
