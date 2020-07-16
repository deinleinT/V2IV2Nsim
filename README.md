# 5G-Sim-V2I/N
Open Source 5G V2I / V2N Simulation OMNeT++-framework. 
We tested the framework with Linux Mint 18.3, 19.3, Ubuntu 18.04 and 20.04 (we use gcc/g++ in version 7.5 for compiling). We could also successfully test the framework on Windows 10. For our tests we used OMNeT++ version 5.6.2.

### Prerequisites

* You need *OMNeT++* (version 5.6.2, https://omnetpp.org/download/) for using this framework and the traffic simulator *SUMO* in version 1.0.0 (https://sumo.dlr.de/docs/index.html).
* Compile and build both and ensure each of them works.
* Download the framework from this site.


### Installing
* Start OMNeT++ after building, create a new and empty workspace and import the 5G-SIM-folder by clicking *File --> Import --> General --> Existing Projects into Workspace --> Choose the downloaded and extracted 5G-SIM-Folder --> Mark "Copy projects into Workspace"*.

* The framework consists of the Veins-Framework (folders Veins and Veins_inet3) in version 5 (https://veins.car2x.org/), 
* The INET-framework (inet-folder) in Version 3.6.7 (https://inet.omnetpp.org/) --> Ensure that the inet_featuretool in the inet-folder has execute-privileges before compiling.
* The SimuLTE-framework (https://simulte.com/) in version 1.1.0 in the lteNR folder. All NR-relevant code is located in src/nr-folder.

* Push the compile button in the OMNeT++-Gui for building the framework.

### Run the examples

* You have to start the sumo-listener (sumo-launchd.py) from the veins-folder via command line before starting the simulation. See the veins-tutorial (https://veins.car2x.org/tutorial/) for all necessary steps.

* In the lteNR-folder, the simulation examples for a motorway and an urban traffic scenario are available (in the simulationsNR-folder). 
* In both scenarios you can simulate an example whichs runs four UDP-applications simultaneously in each car. For that, you just need to run the omnetpp.ini-files within the motorway and urban folder, respectively.


**MORE INFORMATION IS COMING SOON**

## Authors

* **Thomas Deinlein** - https://github.com/deinleinT

## Citation

If you use this framework, please cite it as follows:

*T. Deinlein, R. German, A. Djanatliev, “5G-SIM-V2I/N: Towards a simulation framework for the evaluation of 5G V2I/N Use Cases“, in 2020 European Conference on Networks and Communications (EuCNC): Wireless, Optical and Satellite Networks (WOS) (EuCNC2020 - WOS), Dubrovnik, Croatia, 2020.*


## License

See the [LICENSE.txt](LICENSE.txt) file for details.
