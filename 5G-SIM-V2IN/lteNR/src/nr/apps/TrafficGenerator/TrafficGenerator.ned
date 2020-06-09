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

//
// This file is part of 5G-Sim-V2I/N
//

package nr.apps.TrafficGenerator;

import inet.applications.udpapp.UDPBasicApp;

simple TrafficGenerator extends UDPBasicApp
{
    parameters:
        string type = default("");
        stopTime = default(10000s);
        bool autoReply = default(false);  
        int videoSize = default(intuniform(5000000,10000000)); // in bytes
        
        int dataSize = default(2000000); // in bytes
        
//        volatile int messageLengthCar50 @unit(B) = default(1000B);
//        int dataSizeCar50 = default(500000000);
        bool considerDatasizeAndMessages = default(false);
        int messages = default(10);
        
        //delay for following packets
        double resendingDelay @unit(s)= default(0.001s);
        double startTimeDL @unit(s)= default(15.0s);
        
        double v2xDelayBudget = default(0.1);
        double voipDelayBudget = default(0.02);
        double videoDelayBudget = default(0.07);
        double dataDelayBudget = default(0.06);
        
        double delayBudget10ms = default(0.01);
        double delayBudget20ms = default(0.02);
        double delayBudget50ms = default(0.05);
        double delayBudget100ms = default(0.1);
        double delayBudget200ms = default(0.2);
        double delayBudget500ms = default(0.5);
        double delayBudget1s = default(1.0);
        
        
        @signal[delayVoip];
        @statistic[delayVoip](title="delayVoip"; unit=""; source="delayVoip"; record=vector);
        
        @signal[delayV2X];
        @statistic[delayV2X](title="delayV2X"; unit=""; source="delayV2X"; record=vector);
        
        @signal[delayVideo];
        @statistic[delayVideo](title="delayVideo"; unit=""; source="delayVideo"; record=vector);
        
        @signal[delayData];
        @statistic[delayData](title="delayData"; unit=""; source="delayData"; record=vector);
        
        
        @signal[delayVoipVariation];
        @statistic[delayVoipVariation](title="delayVoipVariation"; unit=""; source="delayVoipVariation"; record=vector);
        
        @signal[delayV2XVariation];
        @statistic[delayV2XVariation](title="delayV2XVariation"; unit=""; source="delayV2XVariation"; record=vector);
        
        @signal[delayVideoVariation];
        @statistic[delayVideoVariation](title="delayVideoVariation"; unit=""; source="delayVideoVariation"; record=vector);
        
        @signal[delayDataVariation];
        @statistic[delayDataVariation](title="delayDataVariation"; unit=""; source="delayDataVariation"; record=vector);
        
        @signal[delayVoipVariationReal];
        @statistic[delayVoipVariationReal](title="delayVoipVariationReal"; unit=""; source="delayVoipVariationReal"; record=vector);
        
        @signal[delayV2XVariationReal];
        @statistic[delayV2XVariationReal](title="delayV2XVariationReal"; unit=""; source="delayV2XVariationReal"; record=vector);
        
        @signal[delayVideoVariationReal];
        @statistic[delayVideoVariationReal](title="delayVideoVariationReal"; unit=""; source="delayVideoVariationReal"; record=vector);
        
        @signal[delayDataVariationReal];
        @statistic[delayDataVariationReal](title="delayDataVariationReal"; unit=""; source="delayDataVariationReal"; record=vector);
        
        @signal[v2vExchangeDelay];
        @statistic[v2vExchangeDelay](title="v2vExchangeDelay"; unit=""; source="v2vExchangeDelay"; record=vector);
        
        @signal[v2vExchangeDelayReal];
        @statistic[v2vExchangeDelayReal](title="v2vExchangeDelayReal"; unit=""; source="v2vExchangeDelayReal"; record=vector);
}



simple TrafficGeneratorServerUL extends TrafficGenerator{
    parameters:
        @class("TrafficGeneratorServerUL");
        type = "server";    
}

simple TrafficGeneratorCarUL extends TrafficGenerator{
    parameters:
        @class("TrafficGeneratorCarUL");
        type = "car";    
}
///////////////////////////////////////////////////////////////////////
simple TrafficGeneratorServerDL extends TrafficGenerator{
    parameters:
        @class("TrafficGeneratorServerDL");
        type = "server";    
}

simple TrafficGeneratorCarDL extends TrafficGenerator{
    parameters:
        @class("TrafficGeneratorCarDL");
        type = "car"; 
        @signal[carName](type=string);  
}