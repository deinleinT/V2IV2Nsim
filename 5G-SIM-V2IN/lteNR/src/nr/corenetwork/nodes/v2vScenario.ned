//
// Part of 5G-Sim-V2I/N
//

//used for V2V Use Cases (CoopPerception, CoopLaneMerge)
package nr.corenetwork.nodes;

network v2vScenario{
    parameters:
        bool v2vMulticastFlag = default(false);
        double v2vMulticastDistance = default(200.0);
        bool v2vCooperativeLaneMerge = default(false);
        
}