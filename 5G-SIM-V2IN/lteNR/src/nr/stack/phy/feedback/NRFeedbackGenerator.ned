//
// Part of 5G-Sim-V2I/N
//

package nr.stack.phy.feedback;

import lte.stack.phy.feedback.*;

simple NRDlFeedbackGenerator extends LteDlFeedbackGenerator
{
    parameters:
        @display("i=block/cogwheel");
        @class("NRDlFeedbackGenerator");

        initialTxMode = default("SINGLE_ANTENNA_PORT0");
		//initialTxMode = default("TRANSMIT_DIVERSITY");
        feedbackGeneratorType = default("IDEAL");
}



