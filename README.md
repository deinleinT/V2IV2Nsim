# 5G-Sim-V2I/N
Open Source 5G V2I / V2N Application-Level Simulation OMNeT++-framework. 
We tested the framework with Linux Mint 18.3, 19.3 and Ubuntu 18.04 (we use gcc/g++ in version 7.5 for compiling). We could also successfully test the framework on Windows 10. For our tests we used OMNeT++ version 5.6.2.

### Features
* Covers the 5G User Plane (3GPP Release 15) for simulating use cases in the context of V2I / N.
* Measuring of several QoS-relevant parameters like packet delay, jitter, reliability and other at application layer.
* Allows simulating different parallel running applications in each vehicle (see default example).
* Comes by default with a motorway and an urban traffic scenario.
* Channel Models from ITU-Guidelines (ITU-R M.2412-0) / 3gpp Specification (38.901, RMa, UMa, UMi, InF) are included (see the folder channelConfigs under lteNR/simulationsNR/)
* Based on the well-known Frameworks SimuLTE/Simu5G, INET and Veins.

### Prerequisites
* You need *OMNeT++* (version 5.6.2, https://omnetpp.org/download/) for using this framework and the traffic simulator *SUMO* in version 1.8.0 (https://sumo.dlr.de/docs/index.html).
* Compile and build both and ensure each of them works.
* Download the framework from this site.

### Installing
* Start OMNeT++ after building, create a new and empty workspace and import the 5G-SIM-folder by clicking *File --> Import --> General --> Existing Projects into Workspace --> Choose the downloaded and extracted 5G-SIM-Folder --> Mark "Copy projects into Workspace"*.
* The framework consists of the Veins-Framework (folders Veins and Veins_inet3) in version 5.1 (https://veins.car2x.org/), 
* The INET-framework (inet-folder) in Version 3.7.0 (https://inet.omnetpp.org/) --> Ensure that the inet_featuretool in the inet-folder has execute-privileges before compiling,
* The SimuLTE-framework (https://simulte.com/) in version 1.1.0 in the lteNR folder. All NR-relevant code is located in src/nr-folder.
* Push the compile button in the OMNeT++-Gui for building the framework.

### Run the examples
* You have to start the sumo-listener (sumo-launchd.py) from the veins-folder via command line before starting the simulation. See the veins-tutorial (https://veins.car2x.org/tutorial/) for all necessary steps.
* In the lteNR-folder, the simulation examples for a motorway and an urban traffic scenario are available (in the simulationsNR-folder). 
* In both scenarios you can simulate an example which runs four UDP-applications simultaneously in each car. For that, you just need to run the omnetpp.ini-files within the motorway and urban folder, respectively.

### Post-processing
* Within the postProcessing-folder an R-Script is contained that can be used for plotting the KPIs packet delay, reliability and jitter from the default scenarios.
* You need the OMNeT++-R-Package for using the script with R Studio.
* See the OMNeT++-github-site (https://github.com/omnetpp/omnetpp-resultfiles) for further information how to install the package within R Studio.

**See changeLog.txt for information about the latest changes. The current version is v0.3.5**

**MORE INFORMATION IS COMING SOON**

## Authors

* **Thomas Deinlein** - https://github.com/deinleinT    thomas.deinlein@fau.de
* Please contact us via email if you have any questions.

## Citation

If you use this framework, please cite it as follows:

*T. Deinlein, R. German and A. Djanatliev, "5G-Sim-V2I/N: Towards a Simulation Framework for the Evaluation of 5G V2I/V2N Use Cases," 2020 European Conference on Networks and Communications (EuCNC), Dubrovnik, Croatia, 2020, pp. 353-357, doi: 10.1109/EuCNC48522.2020.9200949.*

## License

See the [LICENSE.txt](LICENSE.txt) file for details.
