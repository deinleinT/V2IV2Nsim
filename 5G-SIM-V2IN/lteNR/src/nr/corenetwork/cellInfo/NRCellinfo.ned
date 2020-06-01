//
// Part of 5G-Sim-V2I/N
//

package nr.corenetwork.cellInfo;

import lte.corenetwork.lteCellInfo.LteCellInfo;

simple NRCellInfo extends LteCellInfo
{
    parameters:
        @class(NRCellInfo);
        @display("i=block/control");
        int numerology = default(15);
}
