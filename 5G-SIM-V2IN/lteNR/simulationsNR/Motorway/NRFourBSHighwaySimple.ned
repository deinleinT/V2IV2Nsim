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

package nr.simulationsNR.Motorway;

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


network NRFourBSHighwaySimple extends v2vScenario
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=8065.336,5066.8423");

    submodules:

        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=6853.6997,4467.1436;i=misc/globe");
        }

		//INET
        routingRecorder: RoutingTableRecorder {
            @display("p=6853.6997,2019.3937;is=s");
        }
        //INET
        configurator: IPv4NetworkConfigurator {
            @display("p=6853.6997,3353.4175");
            config = xmldoc("demo.xml");
        }

        //# Veins manager module
        veinsManager: VeinsInetManager {
            @display("p=6853.6997,2582.3762;is=s");
        }

        obstacles: ObstacleControl {
            @display("p=6853.6997,4087.7424");
        }

        annotations: AnnotationManager {
            @display("p=6853.6997,3720.5798");
        }

        //# NR modules
        //changed
        channelControl: NRChannelControl {
            @display("p=6853.6997,3096.4036;is=s");
        }

        //changed
        binder: NRBinder {
            @display("p=6853.6997,2827.1511;is=s");
        }

        //INET
        server1: StandardHost {
            @display("p=3182.544,3432.744;is=n;i=device/server");
        }
        server2: StandardHost {
            @display("p=3032.424,4473.576;is=n;i=device/server");
        }
        server3: StandardHost {
            @display("p=3943.152,3993.192;is=n;i=device/server");
        }

        //INET
        router: Router {
            @display("p=2021.616,3983.184;i=device/smallrouter");
        }

        upf: UPF {
            @display("p=510.408,3993.192;is=l");
        }

        gNodeB1: gNodeB {
            @display("p=5911.62,182.52;is=vl");//hochhaus
        }

        gNodeB2: gNodeB {
            @display("p=4279.08,182.52;is=vl");
        }

        gNodeB3: gNodeB {
            @display("p=2555.28,182.52;is=vl");
        }

        gNodeB4: gNodeB {
            @display("p=902.46,577.98004;is=vl");
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

        //# X2 connections
        gNodeB1.x2++ <--> Eth10G <--> gNodeB2.x2++;//1 0 | 2 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB3.x2++;//1 1 | 3 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB4.x2++;//1 2 | 4 0
        gNodeB2.x2++ <--> Eth10G <--> gNodeB3.x2++;//2 1 | 3 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB4.x2++;//2 2 | 4 1
        gNodeB3.x2++ <--> Eth10G <--> gNodeB4.x2++;//3 2 | 4 2

}
