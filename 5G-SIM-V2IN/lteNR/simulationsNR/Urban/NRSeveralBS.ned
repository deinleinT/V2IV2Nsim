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

package nr.simulationsNR.Urban;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.AdhocHost;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.ethernet.Eth10G;

import nr.world.radio.NRChannelControl;
import nr.corenetwork.binder.NRBinder;

import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;
import org.car2x.veins.modules.obstacle.ObstacleControl;
import org.car2x.veins.modules.world.annotations.AnnotationManager;
import org.car2x.veins.base.modules.BaseWorldUtility;

import nr.corenetwork.nodes.UPF;
import nr.corenetwork.nodes.gNodeB;
import nr.corenetwork.nodes.v2vScenario;


network NRSeveralBS extends v2vScenario
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=5000,5000");
        
    submodules:

        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=3941,2297.04;i=misc/globe");
        }

		//INET
        routingRecorder: RoutingTableRecorder {
            @display("p=3941.2021,167.488;is=s");
        }
        //INET
        configurator: IPv4NetworkConfigurator {
            @display("p=3941.2021,1188.118");
            config = xmldoc("demo.xml");
        }

        //# Veins manager module
        veinsManager: VeinsInetManager {
            @display("p=3941.2021,418.72;is=s");
        }

        obstacles: ObstacleControl {
            @display("p=3941.2021,1915.644");
        }

        annotations: AnnotationManager {
            @display("p=3941.2021,1559.732");
        }

        //# NR modules
        //changed
        channelControl: NRChannelControl {
            @display("p=3935.9683,926.418;is=s");
        }

        //changed
        binder: NRBinder {
            @display("p=3941.2021,669.952;is=s");
        }

        //INET
        server1: StandardHost {
            @display("p=4273.416,3442.752;is=n;i=device/server");
        }
        server2: StandardHost {
            @display("p=3572.856,4563.648;is=n;i=device/server");
        }
        server3: StandardHost {
            @display("p=3863.088,3973.176;is=n;i=device/server");
        }

        //INET
        router: Router {
            @display("p=1781.424,4443.552;i=device/smallrouter");
        }

        upf: UPF {
            @display("p=630.504,4453.56;is=l");
        }

        gNodeB1: gNodeB {
            @display("p=2304.19,1555.01;is=vl");//hochhaus
        }

        gNodeB2: gNodeB {
            @display("p=1371.06,1573.35;is=vl");
        }

        gNodeB3: gNodeB {
            @display("p=1493.88,1091.91;is=vl");
        }

        gNodeB4: gNodeB {
            @display("p=936.665,842.739;is=vl");
        }

        gNodeB5: gNodeB {
            @display("p=452.88,1521.78;is=vl");
        }

        gNodeB6: gNodeB {
            @display("p=377.858,2766.73;is=vl");
        }

        gNodeB7: gNodeB {
            @display("p=300.669,437.871;is=vl");
        }

        gNodeB8: gNodeB {
            @display("p=1645.37,163.92;is=vl");
        }

    connections allowunconnected:
        server1.pppg++ <--> Eth10G <--> router.pppg++;
        server2.pppg++ <--> Eth10G <--> router.pppg++;
        server3.pppg++ <--> Eth10G <--> router.pppg++;

        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB2.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB3.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB4.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB5.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB6.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB7.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB8.ppp;

        //# X2 connections
        gNodeB1.x2++ <--> Eth10G <--> gNodeB2.x2++;//1 0 | 2 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB3.x2++;//1 1 | 3 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB4.x2++;//1 2 | 4 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB5.x2++;//1 3 | 5 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB6.x2++;//1 4 | 6 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB7.x2++;//1 5 | 7 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB8.x2++;//1 6 | 8 0

        gNodeB2.x2++ <--> Eth10G <--> gNodeB3.x2++;//2 1 | 3 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB4.x2++;//2 2 | 4 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB5.x2++;//2 3 | 5 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB6.x2++;//2 4 | 6 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB7.x2++;//2 5 | 7 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB8.x2++;//2 6 | 8 1

        gNodeB3.x2++ <--> Eth10G <--> gNodeB4.x2++;//3 2 | 4 2
        gNodeB3.x2++ <--> Eth10G <--> gNodeB5.x2++;//3 3 | 5 2
        gNodeB3.x2++ <--> Eth10G <--> gNodeB6.x2++;//3 4 | 6 2
        gNodeB3.x2++ <--> Eth10G <--> gNodeB7.x2++;//3 5 | 7 2
        gNodeB3.x2++ <--> Eth10G <--> gNodeB8.x2++;//3 6 | 8 2

        gNodeB4.x2++ <--> Eth10G <--> gNodeB5.x2++;//4 3 | 5 3
        gNodeB4.x2++ <--> Eth10G <--> gNodeB6.x2++;//4 4 | 6 3
        gNodeB4.x2++ <--> Eth10G <--> gNodeB7.x2++;//4 5 | 7 3
        gNodeB4.x2++ <--> Eth10G <--> gNodeB8.x2++;//4 6 | 8 3

        gNodeB5.x2++ <--> Eth10G <--> gNodeB6.x2++;//5 4 | 6 4
        gNodeB5.x2++ <--> Eth10G <--> gNodeB7.x2++;//5 5 | 7 4
        gNodeB5.x2++ <--> Eth10G <--> gNodeB8.x2++;//5 6 | 8 4

        gNodeB6.x2++ <--> Eth10G <--> gNodeB7.x2++;//6 5 | 7 5
        gNodeB6.x2++ <--> Eth10G <--> gNodeB8.x2++;//6 6 | 8 5

        gNodeB7.x2++ <--> Eth10G <--> gNodeB8.x2++;//7 6 | 8 6        
}
