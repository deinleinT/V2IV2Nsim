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
import nr.corenetwork.nodes.GeneralParameters;


network NRFourBSHighwaySimple2 extends GeneralParameters
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
            @display("p=4765.8003,4441.3203;is=n;i=device/server");
        }

        //INET
        router: Router {
            @display("p=2017.8601,4451.46;i=device/smallrouter");
        }

        upf1: UPF {
            @display("p=4400.7603,2818.9202;is=l");
        }
        upf2: UPF {
            @display("p=2605.98,2818.9202;is=l");
        }
        upf3: UPF {
            @display("p=1014,3346.2002;is=l");
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

        router.pppg++ <--> Eth10G <--> upf1.filterGate;
        router.pppg++ <--> Eth10G <--> upf2.filterGate;
        router.pppg++ <--> Eth10G <--> upf3.filterGate;

        gNodeB1.ppp <--> Eth10G <--> upf1.pppg++;
        gNodeB2.ppp <--> Eth10G <--> upf1.pppg++;
        gNodeB3.ppp <--> Eth10G <--> upf2.pppg++;
        gNodeB4.ppp <--> Eth10G <--> upf3.pppg++;

        //# X2 connections
        gNodeB1.x2++ <--> Eth10G <--> gNodeB2.x2++;//1 0 | 2 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB3.x2++;//1 1 | 3 0
        gNodeB1.x2++ <--> Eth10G <--> gNodeB4.x2++;//1 2 | 4 0
        gNodeB2.x2++ <--> Eth10G <--> gNodeB3.x2++;//2 1 | 3 1
        gNodeB2.x2++ <--> Eth10G <--> gNodeB4.x2++;//2 2 | 4 1
        gNodeB3.x2++ <--> Eth10G <--> gNodeB4.x2++;//3 2 | 4 2

        //N9 interface
        //        upf2.n9Interface++ <--> Eth10G <--> upf3.n9Interface++;
        upf2.n9Interface++ <--> Eth10G <--> upf1.n9Interface++;
        upf3.n9Interface++ <--> Eth10G <--> upf2.n9Interface++;



}
