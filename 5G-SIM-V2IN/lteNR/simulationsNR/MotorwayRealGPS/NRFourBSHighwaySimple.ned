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

package lteNR.simulationsNR.MotorwayRealGPS;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.AdhocHost;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.ethernet.Eth10G;

import lteNR.nr.world.radio.NRChannelControl;
import lteNR.nr.common.binder.NRBinder;

import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;
import org.car2x.veins.modules.obstacle.ObstacleControl;
import org.car2x.veins.modules.world.annotations.AnnotationManager;
import org.car2x.veins.base.modules.BaseWorldUtility;
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;

import lteNR.nr.common.nodes.UPF;
import lteNR.nr.common.nodes.gnodeB;
import lteNR.nr.common.nodes.GeneralParameters;

import lteNR.common.carrierAggregation.CarrierAggregation;


network NRFourBSHighwaySimpleRealGPS extends GeneralParameters
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=8065.336,5066.8423");

    submodules:

        carrierAggregation: CarrierAggregation {
            @display("p=6976.3203,182.52;is=s");
        }

        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=6966.18,4633.98;i=misc/globe");
        }

		//INET
        routingRecorder: RoutingTableRecorder {
            @display("p=6966.18,1368.9;is=s");
        }

        //INET
        configurator: Ipv4NetworkConfigurator {
            @display("p=6215.8203,4633.98");
            config = xmldoc("demo.xml");
        }

        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=6966.18,801.06");
        }

        //# Veins manager module
        veinsManager: VeinsInetManager {
            @display("p=6966.18,1946.88;is=s");
        }

        obstacles: ObstacleControl {
            @display("p=6966.18,3985.02");
        }

        annotations: AnnotationManager {
            @display("p=6966.18,3346.2002");
        }

        //# NR modules
        //changed
        channelControl: NRChannelControl {
            @display("p=6966.18,2605.98;is=s");
        }

        //changed
        binder: NRBinder {
            @display("p=6215.8203,3985.02;is=s");
        }

        //INET
        server1: StandardHost {
            @display("p=3873.48,3985.02;is=n;i=device/server");
        }
//        server2: StandardHost {
//            @display("p=3032.424,4473.576;is=n;i=device/server");
//        }
//        server3: StandardHost {
//            @display("p=3943.152,3993.192;is=n;i=device/server");
//        }

        //INET
        router: Router {
            @display("p=2021.616,3983.184;i=device/smallrouter");
        }

        upf: UPF {
            @display("p=510.408,3993.192;is=l");
        }

//        gNodeB1: gNodeB {
//            @display("p=324.48,294.06;is=vl");
//        }
        gNodeB1: gnodeB {
            @display("p=-516.20975774,250.778876878;is=vl");
        }

        gNodeB2: gnodeB {
            @display("p=3523.68414986,-37.9199275908;is=vl");
        }

        gNodeB3: gnodeB {
            @display("p=5610.11430225,1500.03599192;is=vl");
        }

        gNodeB4: gnodeB {
            @display("p=6515.96879092,109.479511106;is=vl");
        }

    connections allowunconnected:
        server1.pppg++ <--> Eth10G <--> router.pppg++;
//        server2.pppg++ <--> Eth10G <--> router.pppg++;
//        server3.pppg++ <--> Eth10G <--> router.pppg++;

        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB2.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB3.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB4.ppp;

        //# X2 connections
        gNodeB1.x2++ <--> Eth10G <--> gNodeB2.x2++;//1 0 | 2 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB3.x2++;//1 1 | 3 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB4.x2++;//1 2 | 4 0
        gNodeB2.x2++ <--> Eth10G <--> gNodeB3.x2++;//2 1 | 3 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB4.x2++;//2 2 | 4 1
        gNodeB3.x2++ <--> Eth10G <--> gNodeB4.x2++;//3 2 | 4 2

}
