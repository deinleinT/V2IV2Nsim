# 5G-Sim-V2I/N
Open Source 5G V2I / V2N Simulation OMNeT++-framework

### Prerequisites and Installing
You need OMNeT++ (version 5.6.2, https://omnetpp.org/download/) for using this framework. Start OMNeT++ after building, create a new and empty workspace and import all folders which are within the 5G-SIM-V2IN folder. 
The framework consists of the Veins-Framework (folders Veins and Veins_inet3) in version 5 (https://veins.car2x.org/), the INET-framework (inet-folder) in Version 3.6.7 (https://inet.omnetpp.org/) and the SimuLTE-framework (https://simulte.com/) in version 1.1.0 in the lteNR folder. The latter contains all NR-relevant code in its src/nr-folder. We have tested the framework with Linux Mint 18.3, 19.3 and Ubuntu 18.04.

Before you can run the examples you have to install the traffic simulator SUMO in version 1.0.0 (https://sumo.dlr.de/docs/index.html). You also have to start the sumo-listener (sumo-launch.py) from the veins-folder before starting the simulation. See the veins-tutorial (https://veins.car2x.org/tutorial/) for all necessary steps.

In the lteNR-folder, the simulation examples for a motorway and an urban traffic scenario are available (in the simulationsNR-folder). In both scenerios you can simulate an example whichs runs four applications simultaneously in each car. For that, you just need to run the omnetpp.ini-files within the motorway and urban folder, respectively.


**MORE INFORMATION IS COMING SOON**

## Authors

* **Thomas Deinlein** - https://github.com/deinleinT

## License

See the [LICENSE.txt](LICENSE.txt) file for details.
