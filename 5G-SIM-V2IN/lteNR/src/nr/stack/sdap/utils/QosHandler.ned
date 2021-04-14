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
// This file is part of 5G-Sim-V2I/N
//

package nr.stack.sdap.utils;


moduleinterface IQosHandler
{
}

simple QosHandler like IQosHandler
{
    parameters:
        int v2xQfi = default(1);
        int v2x5Qi = default(3);//GBR
        
        int voipQfi = default(2);
        int voip5Qi = default(7);//NGBR
        
        int videoQfi = default(3);
        int video5Qi = default(6);//NGBR
        
        int dataQfi = default(4);
        int data5Qi = default(79);//NGBR
        
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
