
package nr.simulationsNR.Validation;

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
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;

import nr.corenetwork.nodes.UPF;
import nr.corenetwork.nodes.gNodeB;
import nr.corenetwork.nodes.GeneralParameters;


network ValidationScenario extends GeneralParameters
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=5000,3500");

    submodules:

        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = playgroundSizeX;
                playgroundSizeY = playgroundSizeY;
                playgroundSizeZ = playgroundSizeZ;
                @display("p=1611.84,238.27199;i=misc/globe");
        }

        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=609.696,238.27199");
        }

		//INET
        routingRecorder: RoutingTableRecorder {
            @display("p=553.63196,3202.6558;is=s");
        }
        //INET
        configurator: IPv4NetworkConfigurator {
            @display("p=3314.784,3202.6558");
            config = xmldoc("demo.xml");
        }

        //# Veins manager module
        veinsManager: VeinsInetManager {
            @display("p=1331.5199,3202.6558;is=s");
        }

        obstacles: ObstacleControl {
            @display("p=2340.6719,238.27199");
        }

        annotations: AnnotationManager {
            @display("p=3966.5278,3202.6558");
        }

        //# NR modules
        //changed
        channelControl: NRChannelControl {
            @display("p=2670.0479,3202.6558;is=s");
        }

        //changed
        binder: NRBinder {
            @display("p=2004.2878,3202.6558;is=s");
        }

        //INET
        server1: StandardHost {
            @display("p=709.38,2122.51;is=n;i=device/server");
        }


        //INET
        router: Router {
            @display("p=1621.4401,2116.8801;i=device/smallrouter");
        }

        upf: UPF {
            @display("p=2674.25,2122.51;is=l");
        }

        gNodeB1: gNodeB {//4015
            @display("p=4025,1026.6;is=vl");
        }

    connections allowunconnected:
        server1.pppg++ <--> Eth10G <--> router.pppg++;

        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
}
