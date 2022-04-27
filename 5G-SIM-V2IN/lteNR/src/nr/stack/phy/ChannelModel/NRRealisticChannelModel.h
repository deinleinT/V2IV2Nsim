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

/*
 *
 * Part of 5G-Sim-V2I/N
 *
 *
 */

#pragma once

#define ATT_MAXDISTVIOLATED 1000

#include <omnetpp.h>
#include "nr/common/NRCommon.h"
#include "stack/phy/ChannelModel/LteRealisticChannelModel.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "nr/stack/phy/layer/NRPhyUe.h"
#include "nr/stack/phy/layer/NRPhyGnb.h"
#include "../../mac/layer/NRMacGnb.h"
#include "nr/world/radio/NRChannelControl.h"

//needed if useMatlabEngine is enabled
//link to matlabroot/R2021a/extern/include
//#include "MatlabEngine.hpp"
//#include "MatlabDataArray.hpp"


//needed if useMatlabLibraryCompiler is enabled
//#include "MatlabCppSharedLib.hpp"

/*
 * Realistic Channel Model taken from
 * "--- Guidelines for evaluation of radio interface technologies for IMT-2020 and 38.901"
 *
 * see inherit class for method description
 */
class NRRealisticChannelModel: public LteRealisticChannelModel {
public:
	virtual void initialize(int stage);

	virtual int numInitStages() const {
		return INITSTAGE_LAST + 1;
	}
	virtual void resetOnHandover(MacNodeId nodeId, MacNodeId oldMasterId) {
		positionHistory_.erase(nodeId);
		losMap_.erase(nodeId);
		lastComputedSF_.erase(nodeId);
		lastCorrelationPoint_.erase(nodeId);
		jakesFadingMap_.erase(nodeId);
	}
	inet::Coord& getMyPosition() {
		Enter_Method_Silent
		("getMyPosition()");
		return myCoord3d;
	}
	bool isNodeB() {
		return isNodeB_;
	}
protected:
	simtime_t lastStatisticRecord;

	bool considerInterferenceOnlyForPossibleOverlappingRB;

	DeploymentScenarioNR scenarioNR_;
	NRChannelModel channelModelType_;
	bool isNodeB_;
	//bool dynamic_los_;
	bool veinsObstacleShadowing;
	inet::Coord myCoord3d;

	bool dynamicNlos_; //using obstacleControl
	bool NlosEvaluationIn3D; // evaluate the Nlos/Los in 3D

	int errorCount;

	double antennaGainGnB_;

	//for Indoor Factory channel models from 38.901, table 7.2-4
	double d_clutter; //typical clutter size (10m, 2m or above)
	double clutter_density_r; //percentage of surface area occupied by clutter
	double hClutter; // hc, effective clutter heigth
	double ceilingHeight; // height of factory ceiling
	//

	virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo *lteInfo, bool recordStats);

	virtual double getAttenuationNR(const MacNodeId &nodeId, const Direction &dir, const inet::Coord &uecoord, const inet::Coord &enodebcoord, bool recordStats);

	virtual double getAttenuation_D2D(MacNodeId nodeId, Direction dir, inet::Coord coord, MacNodeId node2_Id, inet::Coord coord_2);

	virtual double computeIndoorHotspot(const double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeDenseUrbanEmbb(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeRuralEmbb(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeUrbanMacroMmtc(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeUrbanMacroUrllc(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeIndoorFactory(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeInFSL(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	virtual double computeInFLOS(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	void checkIndoorFactoryParameters();

	virtual bool isCorrupted(LteAirFrame *frame, UserControlInfo *lteInfo);

	virtual bool error_D2D(LteAirFrame *frame, UserControlInfo *lteI, std::vector<double> rsrpVector);

	virtual std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord);

	virtual double getStdDevNR(const double &d3ddistance, const double &d2ddistance, const MacNodeId &nodeId);

	void checkScenarioAndChannelModel();

	void computeLosProbabilityNR(const double &d2ddistance, const MacNodeId &nodeId, bool recordStats);

	double calcDistanceBreakPoint(const double &d2d);

	double computeUMaA(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	double computeUMaB(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	double computeUMiA(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	double computeUMiB(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	double computeRMaA(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	double computeRMaB(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	double computePLumaLos(const double &d3ddistance, double &d2ddistance);

	double computePLumiALos(const double &d3ddistance, double &d2ddistance);

	double computePLumiBLos(const double &d3ddistance, double &d2ddistance);

	double computePLrmaLos(const double &d3ddistance, double &d2ddistance);

	double computePLrmaNlos(const double &d3ddistance, double &d2ddistance);

	double calcDistanceBreakPointRMa(const double &d2ddistance);

	double computeExtCellPathLossNR(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId);

	bool computeMultiCellInterferenceNR(const MacNodeId &eNbId, const MacNodeId &ueId, const inet::Coord &uecoord, bool isCqi, std::vector<double> &interference, Direction dir,
			const Coord &enodebcoord);

	void considerCodeBlockGroups(LteControlInfo *& info, unsigned char & nTx, double & totalPer, LteAirFrame *& frame);

	 /* computes speed (m/s) for a given node
	   * @param nodeid mac node id of UE
	   * @return the speed in m/s
	   */
	virtual double computeSpeed(const MacNodeId nodeId, const inet::Coord coord, double & mov) override;

	virtual bool computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, RbMap rbmap, std::vector<double> *interference);
	virtual bool computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, inet::Coord ueCoord, bool isCqi, RbMap rbmap, std::vector<double> *interference);

	bool computeExtCellInterferenceNR(const MacNodeId &eNbId, const MacNodeId &nodeId, const Coord &uecoord, bool isCqi, std::vector<double> &interference, const Coord &enodebcoord);
};

