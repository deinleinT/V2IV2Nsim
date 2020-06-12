//
// SPDX-FileCopyrightText: 2020 Friedrich-Alexander University Erlangen-Nürnberg (FAU), 
// Computer Science 7 - Computer Networks and Communication Systems
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


package nr.corenetwork.binder;

import lte.corenetwork.binder.LteBinder;


simple NRBinder extends LteBinder
{
    parameters:
        @class(NRBinder);
        //Qos Characteristics, TS 23.501, V 16.0.2
        string qiValue = "1 2 3 4 65 66 67 75 71 72 73 74 75 5 6 7 8 9 69 70 79 80 82 83 84 85"; //
        string resourceType = "GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR NGBR NGBR NGBR NGBR NGBR NGBR NGBR NGBR NGBR DCGBR DCGBR DCGBR DCGBR ";
        string priorityLevel = "20 40 30 50 7 20 15 0 56 56 56 56 56 10 60 70 80 90 5 55 65 68 19 22 24 21";
        string packetDelayBudgetNR = "0.1 0.15 0.05 0.3 0.075 0.1 0.1 0 0.15 0.3 0.3 0.5 0.5 0.1 0.3 0.1 0.3 0.3 0.06 0.2 0.05 0.01 0.01 0.01 0.03 0.005";//@unit(s)
        string packetErrorRate = "1e-2 1e-3 1e-3 1e-6 1e-2 1e-2 1e-3 0 1e-6 1e-4 1e-8 1e-8 1e-4 1e-6 1e-6 1e-3 1e-6 1e-6 1e-6 1e-6 1e-2 1e-6 1e-4 1e-4 1e-5 1e-5";
        string maxDataBurstVolume = "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 255 1354 1354 255";
        string defAveragingWindow = "2 2 2 2 2 2 2 0 2 2 2 2 2 0 0 0 0 0 0 0 0 0 2 2 2 2";//@unit(s)
        bool exchangeBuffersOnHandover = default(false);
}
