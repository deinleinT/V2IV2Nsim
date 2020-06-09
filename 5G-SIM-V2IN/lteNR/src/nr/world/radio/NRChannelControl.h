//
// SPDX-FileCopyrightText: 2020 Thomas Deinlein <thomas.deinlein@fau.de>
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

/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
*/

#pragma once

#include "world/radio/LteChannelControl.h"


class NRChannelControl : public LteChannelControl
{
  protected:

    /** Calculate interference distance*/
    virtual double calcInterfDist();

    /** Reads init parameters and calculates a maximal interference distance*/
    virtual void initialize();

  public:
    NRChannelControl();
    virtual ~NRChannelControl();

    /** Called from ChannelAccess, to transmit a frame to all the radios in range, on the frame's channel */
    virtual void sendToChannel(RadioRef srcRadio, AirFrame *airFrame);
};

