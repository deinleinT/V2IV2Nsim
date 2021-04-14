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

#include "nr/stack/phy/ChannelModel/NRRealisticChannelModel.h"
#include "stack/phy/layer/LtePhyBase.h"

/*
 * ChannelModels --> see
 * IndoorHotspoteMBB --> InH_A, InH_B
 * DenseUrbaneMBB --> MacroLayer: UMa_A, UMa_B / MicroLayer: UMi_A, UMi_B
 * RuralMBB --> RMa_A, RMa_B
 * UrbanMacromMTC --> UMa_A, UMa_B
 * UrbanMacroURLLC --> UMa_A, UMa_B
 *
 *
 * from 3gpp 38.901 IndoorFactory --> InFSL, InFDL, InFSH, InFDH, InFHH
 * InFSL: sparse clutter, low BS, ceilingHeight 5-25m, d_clutter 10m, clutter_density_r < 0.4, hClutter < ceilingHeight || 0-10m
 * InFDL: dense clutter, low BS, ceilingHeight 5-15m, d_clutter 2m, clutter_density_r >= 0.4, hClutter < ceilingHeight || 0-10m
 * InFSH: sparse clutter, high BS, ceilingHeight 5-25m, d_clutter 10m, clutter_density_r < 0.4, hClutter < ceilingHeight || 0-10m
 * InFDH: dense clutter, high BS, ceilingHeight 5-15m, d_clutter 2m, clutter_density_r >= 0.4, hClutter < ceilingHeight || 0-10m
 * InFHH: high Tx, high Rx, ceilingHeight 5-25m, d_clutter any, clutter_density_r any, hClutter < ceilingHeight || 0-10m
 *
 * Remark:
 * RMa_B is based on the same formulas as RMa in 38.901
 * UMa_B is based on the same formulas as UMa in 38.901
 * UMi_B is based on the same formulas as UMi-Street Canyon in 38.901
 *
 */

Define_Module(NRRealisticChannelModel);

void NRRealisticChannelModel::initialize(int stage) {
	if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT_2) {
		scenario_ = aToDeploymentScenario(par("scenario").stringValue());
		hNodeB_ = par("nodeb_height").doubleValue();
		carrierFrequency_ = par("carrierfrequency").doubleValue();
		shadowing_ = par("shadowing").boolValue();
		hBuilding_ = par("building_height").doubleValue();

		tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
		hUe_ = par("ue_height").doubleValue();

		//for Indoor Factory channel models from 38.901, table 7.2-4
		d_clutter = par("d_clutter").doubleValue();; //typical clutter size (10m, 2m or above)
		clutter_density_r = par("clutter_density_r").doubleValue();; //percentage of surface area occupied by clutter
		hClutter = par("hClutter").doubleValue(); // hc, effective clutter heigth
		ceilingHeight = par("ceilingHeight").doubleValue();
		//

		wStreet_ = par("street_wide").doubleValue();

		correlationDistance_ = par("correlation_distance").doubleValue();
		harqReduction_ = par("harqReduction").doubleValue();

		lambdaMinTh_ = par("lambdaMinTh");
		lambdaMaxTh_ = par("lambdaMaxTh");
		lambdaRatioTh_ = par("lambdaRatioTh");

		antennaGainUe_ = par("antennaGainUe").doubleValue();
		antennaGainEnB_ = par("antennGainEnB").doubleValue();
		antennaGainGnB_ = antennaGainEnB_;
		antennaGainMicro_ = par("antennGainMicro").doubleValue();
		thermalNoise_ = par("thermalNoise").doubleValue();
		cableLoss_ = par("cable_loss").doubleValue();
		ueNoiseFigure_ = par("ue_noise_figure").doubleValue();
		bsNoiseFigure_ = par("bs_noise_figure").doubleValue();
		useTorus_ = par("useTorus");
		dynamicLos_ = par("dynamic_los").boolValue();
		dynamicNlos_ = par("dynamicNlos").boolValue();
		fixedLos_ = par("fixed_los").boolValue();

		fading_ = par("fading");
		std::string fType = par("fading_type");
		if (fType.compare("JAKES") == 0)
			fadingType_ = JAKES;
		else if (fType.compare("RAYLEIGH") == 0)
			fadingType_ = RAYLEIGH;
		else
			fadingType_ = JAKES;

		fadingPaths_ = par("fading_paths");
		enableExtCellInterference_ = par("extCell_interference");
		enableDownlinkInterference_ = par("downlink_interference");
		enableUplinkInterference_ = par("uplink_interference");
		enableD2DInterference_ = par("d2d_interference");
		delayRMS_ = par("delay_rms");

		//get binder
		binder_ = getNRBinder();
		//clear jakes fading map structure
		jakesFadingMap_.clear();

		// statistics
		rcvdSinr_ = registerSignal("rcvdSinr");
		//
		scenarioNR_ = aToDeploymentScenarioNR(par("scenarioNR").stringValue());

		channelModelType_ = aToNRChannelModel(par("channelModelType").stringValue());

		isNodeB_ = par("isNodeB").boolValue(); //OK in NRNic

		checkScenarioAndChannelModel();
		checkIndoorFactoryParameters();

		myCoord_ = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
		if (isNodeB_) {
			myCoord_.z = hNodeB_;
		} else {
			myCoord_.z = hUe_;
		}
		myCoord3d = myCoord_;

		if (dynamicNlos_) {
			veinsObstacleShadowing = par("veinsObstacleShadowing").boolValue();
			NlosEvaluationIn3D = par("NlosEvaluationIn3D").boolValue();
		} else {
			veinsObstacleShadowing = false;
			NlosEvaluationIn3D = false;
		}

		errorCount = 0;

		if (scenarioNR_ == UNKNOW_SCENARIO_NR)
			throw cRuntimeError("Only NR ChannelModels are allowed!");

		lastStatisticRecord = -1;
	}

}


/*
 * InFSL: sparse clutter, low BS, ceilingHeight 5-25m, d_clutter 10m, clutter_density_r < 0.4, hClutter < ceilingHeight || 0-10m
 * InFDL: dense clutter, low BS, ceilingHeight 5-15m, d_clutter 2m, clutter_density_r >= 0.4, hClutter < ceilingHeight || 0-10m
 * InFSH: sparse clutter, high BS, ceilingHeight 5-25m, d_clutter 10m, clutter_density_r < 0.4, hClutter < ceilingHeight || 0-10m
 * InFDH: dense clutter, high BS, ceilingHeight 5-15m, d_clutter 2m, clutter_density_r >= 0.4, hClutter < ceilingHeight || 0-10m
 * InFHH: high Tx, high Rx, ceilingHeight 5-25m, d_clutter any, clutter_density_r any, hClutter < ceilingHeight || 0-10m
 */
void NRRealisticChannelModel::checkIndoorFactoryParameters() {
	//std::cout << "NRRealisticChannelModel::checkIndoorFactoryParameters start at " << simTime().dbl() << std::endl;

	if (channelModelType_ == InFSL) {
		if (d_clutter != 10 || clutter_density_r > 0.4 || (ceilingHeight > 25 || ceilingHeight < 5) || (hClutter > ceilingHeight) || hNodeB_ > hClutter) {
			throw cRuntimeError("Error - InddorFactoryParameters not valid - check clutter and ceiling values");
		}
		return;
	}

	if (channelModelType_ == InFDL) {
		if (d_clutter != 2 || clutter_density_r < 0.4 || (ceilingHeight > 15 || ceilingHeight < 5) || (hClutter > ceilingHeight) || hNodeB_ > hClutter) {
			throw cRuntimeError("Error - InddorFactoryParameters not valid - check clutter and ceiling values");
		}
		return;
	}

	if (channelModelType_ == InFSH) {
		if (d_clutter != 10 || clutter_density_r > 0.4 || (ceilingHeight > 25 || ceilingHeight < 5) || (hClutter > ceilingHeight) || hNodeB_ < hClutter) {
			throw cRuntimeError("Error - InddorFactoryParameters not valid - check clutter and ceiling values");
		}
		return;
	}

	if (channelModelType_ == InFDH) {
		if (d_clutter != 2 || clutter_density_r < 0.4 || (ceilingHeight > 15 || ceilingHeight < 5) || (hClutter > ceilingHeight) || hNodeB_ < hClutter) {
			throw cRuntimeError("Error - InddorFactoryParameters not valid - check clutter and ceiling values");
		}
		return;
	}

	//std::cout << "NRRealisticChannelModel::checkIndoorFactoryParameters end at " << simTime().dbl() << std::endl;
}

double NRRealisticChannelModel::getStdDevNR(const double &d3d, const double &d2d, const MacNodeId &nodeId) {

	//std::cout << "NRRealisticChannelModel::getStdDevNR start at " << simTime().dbl() << std::endl;

	if (channelModelType_ == InH_A) {
		//LOS
		if (losMap_[nodeId]) {
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 6) {
				if (!(d2d <= 150 && d2d >= 0))
					throw cRuntimeError("Error LOS indoor path loss model is not valid");
				else {
					return 3.0;
				}
			} else if (carrierFrequency_ > 6 && carrierFrequency_ <= 100) {
				if (!(d3d <= 150 && d3d >= 1))
					throw cRuntimeError("Error LOS indoor path loss model is not valid");
				else {
					return 3.0;
				}
			}
		} else {
			//NLOS
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 6) {
				if (!(d2d <= 150 && d2d >= 0))
					throw cRuntimeError("Error NLOS indoor path loss model is not valid");
				else {
					return 4.0;
				}
			} else if (carrierFrequency_ > 6 && carrierFrequency_ <= 100) {
				if (!(d3d <= 150 && d3d >= 1))
					throw cRuntimeError("Error NLOS indoor path loss model is not valid");
				else {
					return 8.03;
				}
			}
		}
	} else if (channelModelType_ == InH_B) {
		if (losMap_[nodeId]) {
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 100) {
				if (!(d2d <= 150 && d3d >= 1))
					throw cRuntimeError("Error LOS indoor path loss model is not valid");
				else {
					return 3.0;
				}
			} else
				throw cRuntimeError("Error LOS UMi_B path loss model is not valid");
		} else {
			//NLOS
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 100) {
				if (!(d3d <= 150 && d3d >= 1))
					throw cRuntimeError("Error NLOS indoor path loss model is not valid");
				else {
					return 8.03;
				}
			} else
				throw cRuntimeError("Error NLOS UMi_B path loss model is not valid");
		}
	} else if (channelModelType_ == UMa_A) {
		if (losMap_[nodeId]) {
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
				if (!(10 <= d2d && d2d <= 5000))
					throw cRuntimeError("Error LOS UMa_A path loss model is not valid");
				else {
					return 4.0;
				}
			} else
				throw cRuntimeError("Error LOS UMi_B path loss model is not valid");
		} else {
			//NLOS
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
				if (!(10 <= d2d && d2d <= 5000))
					throw cRuntimeError("Error NLOS UMa_A path loss model is not valid");
				else
					return 6.0;
			} else if (6 < carrierFrequency_ && carrierFrequency_ <= 100) {
				if (!(10 <= d2d && d2d <= 5000))
					throw cRuntimeError("Error NLOS UMa_A path loss model is not valid");
				else
					return 6.0;
			} else
				throw cRuntimeError("Error NLOS UMa_A path loss model is not valid");
		}
	} else if (channelModelType_ == UMa_B) {
		if (losMap_[nodeId]) {
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
				if (!(10 <= d2d && d2d <= 5000))
					throw cRuntimeError("Error LOS UMa_B path loss model is not valid");
				else {
					return 4.0;
				}
			} else
				throw cRuntimeError("Error LOS UMi_B path loss model is not valid");
		} else {
			//NLOS
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
				if (!(10 <= d2d && d2d <= 5000))
					throw cRuntimeError("Error NLOS UMa_B path loss model is not valid");
				else
					return 6.0;
			} else
				throw cRuntimeError("Error NLOS UMa_B path loss model is not valid");
		}
	} else if (channelModelType_ == UMi_A) {

		if (losMap_[nodeId]) {
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
				return 3.0;
			} else if (6 < carrierFrequency_ && carrierFrequency_ <= 100) {
				return 4.0;
			} else
				throw cRuntimeError("Error LOS UMi_A path loss model is not valid");
		} else {
			//NLOS
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
				return 4.0;
			} else if (6 < carrierFrequency_ && carrierFrequency_ <= 100) {
				return 8.2;
			} else
				throw cRuntimeError("Error NLOS UMi_A path loss model is not valid");
		}
	} else if (channelModelType_ == UMi_B) {

		if (losMap_[nodeId]) {
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
				return 4.0;
			} else
				throw cRuntimeError("Error LOS UMi_B path loss model is not valid");
		} else {
			//NLOS
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
				return 7.82;
			} else
				throw cRuntimeError("Error NLOS UMi_B path loss model is not valid");
		}
	} else if (channelModelType_ == RMa_A) {
		if (losMap_[nodeId]) {
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
				double dbp = calcDistanceBreakPointRMa(d2d);
				if (10 <= d2d && d2d <= dbp)
					return 4.0;
				else if (dbp <= d2d && d2d <= 21000)
					return 6.0;
				else
					throw cRuntimeError("Error LOS RMa_A path loss model is not valid");
			} else
				throw cRuntimeError("Error LOS RMa_A path loss model is not valid");
		} else {
			//NLOS
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
				if (10 < d2d && d2d < 21000)
					return 8.0;
				else
					throw cRuntimeError("Error NLOS RMa_A path loss model is not valid");
			} else
				throw cRuntimeError("Error NLOS RMa_A path loss model is not valid");
		}
	} else if (channelModelType_ == RMa_B) {
		if (losMap_[nodeId]) {
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 30) {
				double dbp = calcDistanceBreakPointRMa(d2d);
				if (10 <= d2d && d2d <= dbp)
					return 4.0;
				else if (dbp <= d2d && d2d <= 21000)
					return 6.0;
				else
					throw cRuntimeError("Error LOS RMa_A path loss model is not valid");
			} else
				throw cRuntimeError("Error LOS RMa_A path loss model is not valid");
		} else {
			//NLOS
			if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 30) {
				if (10 < d2d && d2d < 21000)
					return 8.0;
				else
					throw cRuntimeError("Error NLOS RMa_B path loss model is not valid");
			} else
				throw cRuntimeError("Error NLOS RMa_B path loss model is not valid");
		}

	}
	//INDOOR FACTORY from 3gpp 38.901, Table 7.4.1-1
	else if (channelModelType_ == InFDH || channelModelType_ == InFDL || channelModelType_ == InFHH || channelModelType_ == InFSH || channelModelType_ == InFSL) {
		if (!(1 <= d3d && d3d <= 600))
			throw cRuntimeError("Error LOS INDOOR_FACTORY path loss model is not valid");
		if (!(0.5 <= carrierFrequency_ && carrierFrequency_ <= 30))
			throw cRuntimeError("Error LOS INDOOR_FACTORY path loss model is not valid");

		if (losMap_[nodeId]) {
			return 4.3;
		} else {
			//NLOS
			switch (channelModelType_) {
			case InFSL:
				return 5.7;
			case InFDL:
				return 7.2;
			case InFSH:
				return 5.9;
			case InFDH:
				return 4.0;
			default:
				throw cRuntimeError("Error NLOS path loss model INDOOR_FACTORY is not valid");
			}
		}
	} else
		throw cRuntimeError("Error NLOS path loss model is not valid");

	return 0.0;
}

void NRRealisticChannelModel::checkScenarioAndChannelModel() {
	//std::cout << "NRRealisticChannelModel::checkScenarioAndChannelModel start at " << simTime().dbl() << std::endl;

	switch (scenarioNR_) {
	case INDOOR_HOTSPOT_EMBB:
		if (channelModelType_ == InH_A || channelModelType_ == InH_B)
			return;
	case DENSE_URBAN_EMBB:
		if (channelModelType_ == UMa_A || channelModelType_ == UMa_B || channelModelType_ == UMi_A || channelModelType_ == UMi_B)
			return;
	case RURAL_EMBB:
		if (channelModelType_ == RMa_A || channelModelType_ == RMa_B)
			return;
	case URBAN_MACRO_MMTC:
		if (channelModelType_ == UMa_A || channelModelType_ == UMa_B)
			return;
	case URBAN_MACRO_URLLC:
		if (channelModelType_ == UMa_A || channelModelType_ == UMa_B)
			return;
	case INDOOR_FACTORY:
		if(channelModelType_ == InFDH || channelModelType_ == InFDL ||channelModelType_ == InFHH ||channelModelType_ == InFSH ||channelModelType_ == InFSL)
			return;
	default:
		throw cRuntimeError("Wrong value %s for path-loss scenario: Channel model %s not allowed", DeploymentScenarioNRToA(scenarioNR_).c_str(), NRChannelModelToA(channelModelType_).c_str());
	}
}

/*
 * taken from simulte
 */
double computeAngularAttenuation(double angle) {
	//std::cout << "NRRealisticChannelModel::computeAngularAttenuation start at " << simTime().dbl() << std::endl;

	double angularAtt;
	double angularAttMin = 25;
	// compute attenuation due to angular position
	// see TR 36.814 V9.0.0 for more details
	angularAtt = 12 * pow(angle / 70.0, 2);

	//EV << "\t angularAtt[" << angularAtt << "]" << endl;
	// max value for angular attenuation is 25 dB
	if (angularAtt > angularAttMin)
		angularAtt = angularAttMin;

	//std::cout << "NRRealisticChannelModel::computeAngularAttenuation end at " << simTime().dbl() << std::endl;

	return angularAtt;
}

double NRRealisticChannelModel::computeSpeed(const MacNodeId nodeId, const Coord coord, double &mov) {
	double speed = 0.0;
	double movement = 0.0;
	mov = movement;

	if (positionHistory_.find(nodeId) == positionHistory_.end()) {
		// no entries
		return speed;
	} else {
		//compute distance traveled from last update by UE (eNodeB position is fixed)

		if (positionHistory_[nodeId].size() == 1) {
			//  the only element refers to present , return 0
			return speed;
		}

		movement = positionHistory_[nodeId].front().second.distance(coord);
		mov = movement;

		if (movement <= 0.0)
			return speed;
		else {
			double time = (NOW.dbl()) - (positionHistory_[nodeId].front().first.dbl());
			if (time <= 0.0) // time not updated since last speed call
				throw cRuntimeError("Multiple entries detected in position history referring to same time");
			// compute speed
			speed = (movement) / (time);
		}
	}
	return speed;
}

std::vector<double> NRRealisticChannelModel::getSINR(LteAirFrame *frame, UserControlInfo *lteInfo, bool recordStats) {
	//std::cout << "NRRealisticChannelModel::getSINR start at " << simTime().dbl() << std::endl;

	//init myCoord
	myCoord_ = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"))->getPosition();
	myCoord3d = myCoord_;

	if (isNodeB_) {
		//nodeB
		myCoord3d.z = hNodeB_;
	} else {
		//ue
		myCoord3d.z = hUe_;
	}

	std::vector<double> snrVector;
	MacNodeId ueId = 0;
	MacNodeId eNbId = 0;

	AttenuationVector::iterator it;
	//get tx power
	double recvPower = lteInfo->getTxPower(); // dBm

	//Get the Resource Blocks used to transmit this packet
	RbMap rbmap = lteInfo->getGrantedBlocks();

	//get move object associated to the packet
	//this object is refereed to eNodeB if direction is DL or UE if direction is UL
	Coord coord = lteInfo->getCoord();

	// position of eNb and UE
	Coord ueCoord;
	Coord enbCoord;
	//Coord myCoord = myCoord_;

	double antennaGainTx = 0.0;
	double antennaGainRx = 0.0;
	double noiseFigure = 0.0;
	double speed = 0.0;

	// true if we are computing a CQI for the DL direction
	bool cqiDl = false;

	//EnbType eNbType;

	Direction dir = (Direction) lteInfo->getDirection();

	double d3d = 0.0;
	double d2d = 0.0;
	double movement = 0.0;

	//EV << "------------ GET SINR ----------------" << endl;
	//std::cout << "------------ GET SINR ----------------" << endl;
	//===================== PARAMETERS SETUP ============================
	/*
	 * if NRPhyUe calls this method during initialize (dynamicCellAssociation) or Handover
	 */
	if (dir == DL && (lteInfo->getFrameType() == FEEDBACKPKT || lteInfo->getFrameType() == HANDOVERPKT) && !isNodeB_) {
		noiseFigure = ueNoiseFigure_; //dB
		//set antenna gain Figure
		antennaGainTx = antennaGainGnB_; //dB
		antennaGainRx = antennaGainUe_;  //dB

		// get MacId for Ue and eNb
		ueId = lteInfo->getDestId();
		eNbId = lteInfo->getSourceId();

		// get position of Ue and eNb
		ueCoord = myCoord_;
		ueCoord.z = hUe_;
		myCoord3d = ueCoord;

		enbCoord = lteInfo->getCoord();
		enbCoord.z = hNodeB_;
		coord.z = hNodeB_;

		d3d = ueCoord.distance(enbCoord);
		d2d = sqrt(pow(ueCoord.x - enbCoord.x, 2) + pow(ueCoord.y - enbCoord.y, 2));

		//cqiDl = true;
		speed = computeSpeed(ueId, ueCoord, movement);

	}
	/*
	 * if direction is DL and this is not a feedback packet,
	 * this function has been called by NRRealisticChannelModel::isCorrupted() in the UE
	 *
	 *         DownLink error computation
	 */
	else if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT)) {
		//assert(recvPower == 46);
		//set noise Figure
		noiseFigure = ueNoiseFigure_; //dB
		//set antenna gain Figure
		antennaGainTx = antennaGainGnB_; //dB
		antennaGainRx = antennaGainUe_;  //dB

		// get MacId for Ue and eNb
		ueId = lteInfo->getDestId();
		eNbId = lteInfo->getSourceId();

		// get position of Ue and eNb
		ueCoord = myCoord_;
		ueCoord.z = hUe_;
		myCoord3d = ueCoord;

		enbCoord = lteInfo->getCoord();
		enbCoord.z = hNodeB_;
		coord.z = hNodeB_;

		d3d = ueCoord.distance(enbCoord);
		d2d = sqrt(pow(ueCoord.x - enbCoord.x, 2) + pow(ueCoord.y - enbCoord.y, 2));

		speed = computeSpeed(ueId, ueCoord, movement);
//			cqiDl = true;
	}
	/*
	 * If direction is UL OR
	 * if the packet is a feedback packet
	 * it means that this function is called by the feedback computation module
	 *
	 * located in the eNodeB that compute the feedback received by the UE
	 * Hence the UE macNodeId can be taken by the sourceId of the lteInfo
	 * and the speed of the UE is contained by the Move object associated to the lteinfo
	 */
	else // UL/DL CQI & UL error computation (isCorrupted) --> requestFeedback in LtePhyEnb
	{
		// get MacId for Ue and eNb
		ueId = lteInfo->getSourceId();
		eNbId = lteInfo->getDestId();
		//eNbType = getCellInfo(eNbId)->getEnbType();

		if (dir == DL) {
			//assert(recvPower == 46);
			//set noise Figure
			noiseFigure = ueNoiseFigure_; //dB
			//set antenna gain Figure
			antennaGainTx = antennaGainGnB_; //dB
			antennaGainRx = antennaGainUe_;  //dB

			// use the jakes map in the UE side
			cqiDl = true;
			ASSERT(isNodeB_);
		} else // if( dir == UL )
		{
			//assert(recvPower == 26);

			antennaGainTx = antennaGainUe_;
			antennaGainRx = antennaGainGnB_;
			noiseFigure = bsNoiseFigure_;

			// use the jakes map in the eNb side
			cqiDl = false;
		}

		// get position of Ue and eNb
		ueCoord = coord;
		ueCoord.z = hUe_;
		coord.z = hUe_;
		enbCoord = myCoord3d;

		d3d = enbCoord.distance(ueCoord);
		d2d = sqrt(pow(coord.x - enbCoord.x, 2) + pow(coord.y - enbCoord.y, 2));
		speed = computeSpeed(ueId, ueCoord, movement);

	}
	//LteCellInfo *eNbCell = getCellInfo(eNbId);
	//const char *eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

	double attenuation;

	attenuation = getAttenuationNR(ueId, dir, ueCoord, enbCoord, recordStats); // dB

	//compute attenuation (PATHLOSS + SHADOWING)
	recvPower -= attenuation; // (dBm-dB)=dBm

	//add antenna gain
	recvPower += antennaGainTx; // (dBm+dB)=dBm
	recvPower += antennaGainRx; // (dBm+dB)=dBm

	//sub cable loss
	recvPower -= cableLoss_; // (dBm-dB)=dBm

	//=============== angular ATTENUATION =================
	if (dir == DL) {
		//get tx angle
		LtePhyBase *ltePhy = check_and_cast<LtePhyBase*>(getSimulation()->getModule(binder_->getOmnetId(eNbId))->getSubmodule("lteNic")->getSubmodule("phy"));

		if (ltePhy->getTxDirection() == ANISOTROPIC) {
			// get tx angle --> azimuth only
			double txAngle = ltePhy->getTxAngle();

			// compute the angle between uePosition and reference axis, considering the eNb as center
			double ueAngle = computeAngle(enbCoord, ueCoord);

			// compute the reception angle between ue and eNb
			double recvAngle = fabs(txAngle - ueAngle);

			if (recvAngle > 180)
				recvAngle = 360 - recvAngle;

			// compute attenuation due to sectorial tx
			double angularAtt = computeAngularAttenuation(recvAngle);

			recvPower -= angularAtt;
		}
		// else, antenna is omni-directional
	}
	//=============== END angular ATTENUATION =================

	//snrVector.resize(band_, 0.0);
	// compute and add interference due to fading
	// Apply fading for each band
	// if the phy layer is localized we can assume that for each logical band we have different fading attenuation
	// if the phy layer is distributed the number of logical band should be set to 1
	double fadingAttenuation = 0;
	//for each logical band
	for (unsigned int i = 0; i < band_; i++) {
		fadingAttenuation = 0;
		//if fading is enabled
		if (fading_) {
			//Appling fading
			if (fadingType_ == RAYLEIGH)
				fadingAttenuation = rayleighFading(ueId, i);

			else if (fadingType_ == JAKES)
				fadingAttenuation = jakesFading(ueId, speed, i, cqiDl);
		}
		// add fading contribution to the received pwr
		double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

		//if txmode is multi user the tx power is dived by the number of paired user
		// in db divede by 2 means -3db
		if (lteInfo->getTxMode() == MULTI_USER) {
			finalRecvPower -= 3;
		}

		snrVector.push_back(finalRecvPower);
	}
	//============ END PATH LOSS + SHADOWING + FADING ===============

	/*
	 * The SINR will be calculated as follows
	 *
	 *              Pwr
	 * SINR = ---------
	 *           N  +  I
	 *
	 * Ndb = thermalNoise_ + noiseFigure (measured in decibel)
	 * I = extCellInterference + multiCellInterference
	 */

	//============ MULTI CELL INTERFERENCE COMPUTATION =================
	//vector containing the sum of multiCell interference for each band
	std::vector<double> multiCellInterference; // Linear value (mW)
	// prepare data structure
	multiCellInterference.resize(band_, 0);
	if (enableDownlinkInterference_ && dir == DL) {
		computeDownlinkInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT || lteInfo->getFrameType() == HANDOVERPKT), rbmap, &multiCellInterference);
	} else if (enableUplinkInterference_ && dir == UL) {
		computeUplinkInterference(eNbId, ueId, (lteInfo->getFrameType() == FEEDBACKPKT || lteInfo->getFrameType() == HANDOVERPKT), rbmap, &multiCellInterference);
	}

	//============ EXTCELL INTERFERENCE COMPUTATION =================
	//vector containing the sum of multiCell interference for each band
	std::vector<double> extCellInterference; // Linear value (mW)
	// prepare data structure
	extCellInterference.resize(band_, 0);
	if (enableExtCellInterference_ && ueId != 0) {
		computeExtCellInterferenceNR(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), extCellInterference, enbCoord); // dBm
	}

	//===================== SINR COMPUTATION ========================
	// compute and linearize total noise
	double totN = dBmToLinear(thermalNoise_ + noiseFigure);

	// denominator expressed in dBm as (N+extCell+multiCell)
	double den;
	//EV << "NRRealisticChannelModel::getSINR - distance from my eNb=" << enbCoord.distance(ueCoord) << " - DIR=" << ((dir == DL) ? "DL" : "UL") << endl;

	// add interference for each band
	for (unsigned int i = 0; i < band_; i++) {
		// if we are decoding a data transmission and this RB has not been used, skip it
		// TODO fix for multi-antenna case
		if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
			continue;

		//               (      mW            +  mW  +        mW            )
		den = linearToDBm(extCellInterference[i] + totN + multiCellInterference[i]);

		//EV << "\t ext[" << extCellInterference[i] << "] - multi[" << multiCellInterference[i] << "] - recvPwr[" << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i] - den << "]\n";

		// compute final SINR
		snrVector[i] -= den;
	}

	if (ueId >= UE_MIN_ID && ueId <= UE_MAX_ID) {
		updatePositionHistory(ueId, ueCoord);
	}

	if (lastStatisticRecord != NOW) {
		lastStatisticRecord = NOW;
		if (ueId != 0 && recordStats) {
			if (isNodeB_) {

				NRPhyGnb *nrPhy = check_and_cast<NRPhyGnb*>(getSimulation()->getModule(binder_->getOmnetId(eNbId))->getSubmodule("lteNic")->getSubmodule("phy"));
				nrPhy->recordAttenuation(attenuation);
				nrPhy->recordDistance2d(d2d);
				nrPhy->recordDistance3d(d3d);
				for (const auto &var : snrVector) {
					nrPhy->recordSNIR(var);
				}
				if (speed > 0) {
					nrPhy->recordSpeed((speed * 60.0 * 60.0) / 1000.0);
				}
			} else {
				NRPhyUe *nrPhy = check_and_cast<NRPhyUe*>(getSimulation()->getModule(binder_->getOmnetId(ueId))->getSubmodule("lteNic")->getSubmodule("phy"));
				nrPhy->recordAttenuation(attenuation);
				nrPhy->recordDistance2d(d2d);
				nrPhy->recordDistance3d(d3d);
				for (const auto &var : snrVector) {
					nrPhy->recordSNIR(var);
				}
				if (speed > 0) {
					nrPhy->recordSpeed((speed * 60.0 * 60.0) / 1000.0);
				}
			}
		}
	}

//std::cout << "NRRealisticChannelModel::getSINR end at " << simTime().dbl() << std::endl;

	return snrVector;
}

void NRRealisticChannelModel::computeLosProbabilityNR(const double &d2d, const MacNodeId &nodeId, bool recordStats) {
	//std::cout << "NRRealisticChannelModel::computeLosProbabilityNR start at " << simTime().dbl() << std::endl;

	double p = 0;
	double k_subsce;
	if (!dynamicLos_) { //set by default to true
		losMap_[nodeId] = fixedLos_; // fixedLos_ set by default to false
		return;
	}
	switch (channelModelType_) {
	case InH_A:
	case InH_B:
		if (d2d <= 5) {
			p = 1;
		} else if (5 < d2d && d2d <= 49) {
			p = exp(-((d2d - 5) / 70.8));
		} else { //49 < d2d
			p = exp(-((d2d - 49) / 211.7)) * 0.54;
		}
		break;
	case UMa_A:
		//only outdoor users!--> see table A1-9 in ITU-R M.2412-0
	case UMa_B:
		double c;
		if (hUe_ <= 13)
			c = 0.0;
		else if (13 < hUe_ && hUe_ <= 23)
			c = pow((hUe_ - 13) / 10, 1.5);
		else
			throw cRuntimeError("Error NLOS UMaA/UMaB loss model wrong UE Height");

		if (d2d <= 18)
			p = 1;
		else
			//18 < d2D
			p = ((18 / d2d) + exp(-(d2d / 63)) * (1 - (18 / d2d))) * (1 + c * (5 / 4) * pow(d2d / 100, 3) * exp(-(d2d / 150)));
		break;
	case UMi_A:
	case UMi_B:
		if (d2d <= 18)
			p = 1;
		else if (18 < d2d)
			p = (18 / d2d) + exp(-(d2d / 36)) * (1 - (18 / d2d));
		else
			throw cRuntimeError("Error NLOS UMiA/UMiB loss model wrong distance (2d)");

		break;
	case RMa_A:
	case RMa_B:
		if (d2d <= 10)
			p = 1;
		else if (10 < d2d)
			p = exp(-((d2d - 10) / 1000));
		else
			throw cRuntimeError("Error NLOS RMaA/RMaB loss model wrong distance (2d)");
		break;

	//from 38.901, Indoor Factory
	case InFSL:
	case InFDL:
		k_subsce = -(d_clutter / log(1 - clutter_density_r));
		p = exp(-(d2d / k_subsce));
		break;
	case InFSH:
	case InFDH:
		k_subsce = -(d_clutter / log(1 - clutter_density_r)) * ((hNodeB_ - hUe_)/(hClutter - hUe_));
		p = exp(-(d2d / k_subsce));
		break;
	case InFHH:
		p = 1;
		break;

	default:
		throw cRuntimeError("Wrong path-loss scenario value %d", scenario_);
	}
	double random = omnetpp::uniform(getEnvir()->getRNG(0), 0.0, 1.0);
	if (random <= p) {
		losMap_[nodeId] = true;
		if (recordStats)
			getNRBinder()->incrementLosDetected();
	} else {
		losMap_[nodeId] = false;
		if (recordStats)
			getNRBinder()->incrementNlosDetected();
	}

	//std::cout << "NRRealisticChannelModel::computeLosProbabilityNR end at " << simTime().dbl() << std::endl;
}
/*
 *
 */
double NRRealisticChannelModel::getAttenuationNR(const MacNodeId &nodeId, const Direction &dir, const Coord &uecoord, const Coord &enodebcoord, bool recordStats) {
	//std::cout << "NRRealisticChannelModel::getAttenuationNR start at " << simTime().dbl() << std::endl;

	double movement = .0;
	double speed = .0;

	double d3ddistance = enodebcoord.distance(uecoord);
	double d2ddistance = sqrt(pow(enodebcoord.x - uecoord.x, 2) + pow(enodebcoord.y - uecoord.y, 2));

	speed = computeSpeed(nodeId, uecoord, movement);

	//If traveled distance is greater than correlation distance UE could have changed its state and
	// its visibility from eNodeb, hence it is correct to recompute the los probability
	if (dynamicNlos_ && (movement >= correlationDistance_ || losMap_.find(nodeId) == losMap_.end())) {
		bool nlos;

		if (dir == DL) {
			nlos = getNRBinder()->checkIsNLOS(enodebcoord, uecoord, hBuilding_, NlosEvaluationIn3D);	//use veinsObstacleControl for NLOS evaluation
		} else {
			nlos = getNRBinder()->checkIsNLOS(uecoord, enodebcoord, hBuilding_, NlosEvaluationIn3D);	//use veinsObstacleControl for NLOS evaluation
		}

		losMap_[nodeId] = !nlos;
		if (recordStats) {
			if (nlos)
				getNRBinder()->incrementNlosDetected();
			else
				getNRBinder()->incrementLosDetected();
		}
	} else if (movement >= correlationDistance_ || losMap_.find(nodeId) == losMap_.end()) { // use NLOS Probability of the ITU-ChannelModels
		computeLosProbabilityNR(d2ddistance, nodeId, recordStats);
	}

	double attenuation = 0.0;
	if (!dynamicNlos_ || (dynamicNlos_ && !veinsObstacleShadowing)) {
		switch (scenarioNR_) {
		case INDOOR_HOTSPOT_EMBB:
			attenuation = computeIndoorHotspot(d3ddistance, d2ddistance, nodeId);
			break;
		case DENSE_URBAN_EMBB:
			attenuation = computeDenseUrbanEmbb(d3ddistance, d2ddistance, nodeId);
			break;
		case RURAL_EMBB:
			attenuation = computeRuralEmbb(d3ddistance, d2ddistance, nodeId);
			break;
		case URBAN_MACRO_MMTC:
			attenuation = computeUrbanMacroMmtc(d3ddistance, d2ddistance, nodeId);
			break;
		case URBAN_MACRO_URLLC:
			attenuation = computeUrbanMacroUrllc(d3ddistance, d2ddistance, nodeId);
			break;
		case INDOOR_FACTORY:
			attenuation = computeIndoorFactory(d3ddistance, d2ddistance, nodeId);
			break;
		default:
			throw cRuntimeError("Wrong value %d for path-loss scenario", scenarioNR_);
		}
	} else {
		attenuation = getNRBinder()->calculateAttenuationPerCutAndMeter(enodebcoord, uecoord); //use veinsObstacleControl for NLOS, here it is nlos, and we want to calculate attenuation with veins obstacleControl
	}

//    Applying shadowing only if it is enabled by configuration
//    log-normal shadowing
	if (shadowing_) {
		double mean = 0;

		//Get std deviation according to los/nlos and selected scenario
		double stdDev = getStdDevNR(d3ddistance, d2ddistance, nodeId);
		double time = 0;
		double space = 0;
		double att;

		// if direction is DOWNLINK it means that this module is located in UE stack than
		// the Move object associated to the UE is myMove_ varible
		// if direction is UPLINK it means that this module is located in UE stack than
		// the Move object associated to the UE is move varible

		// if shadowing for current user has never been computed
		if (lastComputedSF_.find(nodeId) == lastComputedSF_.end()) {
			//Get the log normal shadowing with std deviation stdDev
			att = omnetpp::normal(getEnvir()->getRNG(0), mean, stdDev);

			//store the shadowing attenuation for this user and the temporal mark
			std::pair<simtime_t, double> tmp(NOW, att);
			lastComputedSF_[nodeId] = tmp;

			//If the shadowing attenuation has been computed at least one time for this user
			// and the distance traveled by the UE is greater than correlation distance
		} else if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed > correlationDistance_) {

			//get the temporal mark of the last computed shadowing attenuation
			time = (NOW - lastComputedSF_.at(nodeId).first).dbl();

			//compute the traveled distance
			space = time * speed;

			//Compute shadowing with a EAW (Exponential Average Window) (step1)
			double a = exp(-0.5 * (space / correlationDistance_));

			//Get last shadowing attenuation computed
			double old = lastComputedSF_.at(nodeId).second;

			//Compute shadowing with a EAW (Exponential Average Window) (step2)
			att = a * old + sqrt(1 - pow(a, 2)) * omnetpp::normal(getEnvir()->getRNG(0), mean, stdDev);

			// Store the new computed shadowing
			std::pair<simtime_t, double> tmp(NOW, att);
			lastComputedSF_[nodeId] = tmp;

			// if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
		} else {
			att = lastComputedSF_.at(nodeId).second;
		}
		attenuation += att;
	}

// update current user position

	if (nodeId >= UE_MIN_ID && nodeId <= UE_MAX_ID) {
		updatePositionHistory(nodeId, uecoord);
	}

	//EV << "NRRealisticChannelModel::getAttenuation - computed attenuation at distance(2d) " << d2ddistance << " (3d) " << d3ddistance << " for eNb is " << attenuation << endl;

	//std::cout << "NRRealisticChannelModel::getAttenuationNR end at " << simTime().dbl() << std::endl;

	return attenuation;
}

/*
 * represents Table A1-2 taken from ITU-R M.2412-0
 * Optional Formulas / Models not implemented!
 */
double NRRealisticChannelModel::computeIndoorHotspot(const double &d3d, double &d2d, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeIndoorHotspot start at " << simTime().dbl() << std::endl;

	if (channelModelType_ == InH_A) {
		if (!(hNodeB_ >= 3.0 && hNodeB_ <= 6.0 && hUe_ >= 1.0 && hUe_ <= 2.5))
			throw cRuntimeError("Error Path Loss IndoorHotspot invalid, height enodeb or ue not valid");
		//LOS
		if (losMap_[nodeId]) {            //TODO change
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 6) {
				if (0 <= d2d && d2d <= 150)
					return 16.9 * log10(d3d) + 32.8 + 20 * log10(carrierFrequency_);
				else {
					throw cRuntimeError("Error LOS indoor path loss model is not valid");
				}
			} else if (carrierFrequency_ > 6 && carrierFrequency_ <= 100) {
				if (1 <= d3d && d3d <= 150)
					return 32.4 + 17.3 * log10(d3d) + 20 * log10(carrierFrequency_);
				else {
					throw cRuntimeError("Error LOS indoor path loss model is not valid");
				}
			}
		} else {
			//NLOS
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 6) {
				if (0 <= d2d && d2d <= 150)
					return 43.3 * log10(d3d) + 11.5 + 20 * log10(carrierFrequency_);
				else {
					throw cRuntimeError("Error NLOS indoor path loss model is valid");
				}
			} else if (carrierFrequency_ > 6 && carrierFrequency_ <= 100) {
				if (1 <= d3d && d3d <= 150) {
					double plInHNlos = 43.3 * log10(d3d) + 11.5 + 20 * log10(carrierFrequency_);
					//PL'InH-NLOS
					double PLInHNlos = 38.3 * log10(d3d) + 17.3 + 24.9 * log10(carrierFrequency_);
					return max(plInHNlos, PLInHNlos);
				} else {
					throw cRuntimeError("Error NLOS indoor path loss model is valid");
				}
			}
		}
	}
//InH_B
	else {
		//LOS
		if (losMap_[nodeId]) {
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 100) {
				if (1 <= d3d && d3d <= 150)
					return 32.4 + 17.3 * log10(d3d) + 20 * log10(carrierFrequency_);
				else {
					throw cRuntimeError("Error LOS indoor path loss model is not valid");
				}
			}
		} else {
			//NLOS
			if (carrierFrequency_ >= 0.5 && carrierFrequency_ <= 100) {
				if (1 <= d3d && d3d <= 150) {
					double plInHNlos = 32.4 + 17.3 * log10(d3d) + 20 * log10(carrierFrequency_);
					//PL'InH-NLOS
					double PLInHNlos = 38.3 * log10(d3d) + 17.3 + 24.9 * log10(carrierFrequency_);
					return max(plInHNlos, PLInHNlos);
				} else {
					throw cRuntimeError("Error NLOS indoor path loss model is valid");
				}
			}
		}
	}
	//std::cout << "NRRealisticChannelModel::computeIndoorHotspot end at " << simTime().dbl() << std::endl;

	return 0.0;
}

/*
 * calculates the distance breakpoint in Scenario UMa_x
 */
double NRRealisticChannelModel::calcDistanceBreakPoint(const double &d2d) {
	//std::cout << "NRRealisticChannelModel::calcDistanceBreakPoint start at " << simTime().dbl() << std::endl;

	double dbp = 0.0;
//h'BS
	double hBS = 0.0;
//h'UT
	double hUT = 0.0;
//C(d2d,hUe)
	double c = 0.0;
	double g = 0.0;
//1/(1+c)
	double probability = 0.0;
//effective environment height
	double he = 0.0;

//calc g(d2d)
	if (d2d <= 18)
		g = 0;
	else if (18 < d2d)
		g = (5 / 4) * pow(d2d / 100, 3) * exp(-d2d / 150);

//calc C(d2d,hUT)
	if (hUe_ < 13)
		c = 0.0;
	else if (13 <= hUe_ && hUe_ <= 23) {
		c = pow((hUe_ - 13) / 10, 1.5) * g;
	}

	probability = 1 / (1 + c);
	double random = omnetpp::uniform(getEnvir()->getRNG(0), 0.0, 1.0);
	if (random <= probability)
		he = 1.0;
	else
		he = omnetpp::uniform(getEnvir()->getRNG(0), 10.5, hUe_ - 1.5);

	hBS = hNodeB_ - he;
	hUT = hUe_ - he;

	dbp = 4 * hBS * hUT * (carrierFrequency_ * 1000000000 / SPEED_OF_LIGHT);

	//std::cout << "NRRealisticChannelModel::calcDistanceBreakPoint end at " << simTime().dbl() << std::endl;

	return dbp;
}
/*
 * dBP in Note 4, ITU-R M.2412-0
 */
double NRRealisticChannelModel::calcDistanceBreakPointRMa(const double &d2d) {
	return 2 * M_PI * hNodeB_ * hUe_ * (carrierFrequency_ * 1000000000 / SPEED_OF_LIGHT);
}

double NRRealisticChannelModel::computePLrmaLos(const double &d3d, double &d2d) {
	//std::cout << "NRRealisticChannelModel::computePLrmaLos start at " << simTime().dbl() << std::endl;

	if (!(10 <= d2d && d2d <= 21000))
		throw cRuntimeError("Error Path Loss RMa --> invalid distance (2d)");

//dbp Note 4
	double dbp = calcDistanceBreakPointRMa(d2d);
	double a1 = (0.03 * pow(hBuilding_, 1.72));
	double b1 = 0.044 * pow(hBuilding_, 1.72);
	double a = min(a1, 10.0);
	double b = min(b1, 14.77);

	double plOne = 20 * log10(40 * M_PI * d3d * (carrierFrequency_ / 3)) + a * log10(d3d) - b + 0.002 * log10(hBuilding_) * d3d;

//PL1
	if (10 <= d2d && d2d <= dbp)
		return plOne;
	else if (dbp <= d2d && d2d <= 21000)  //PL2
		return 20 * log10(40 * M_PI * dbp * (carrierFrequency_ / 3)) + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp + 40 * log10(d3d / dbp);
	else
		throw cRuntimeError("Error Path Loss RMa --> invalid distance (2d)");

	//std::cout << "NRRealisticChannelModel::computePLrmaLos end at " << simTime().dbl() << std::endl;

}

double NRRealisticChannelModel::computePLrmaNlos(const double &d3d, double &d2d) {
	//std::cout << "NRRealisticChannelModel::computePLrmaNlos start at " << simTime().dbl() << std::endl;

	if (10 < d2d && d2d < 21000){
		return 161.04 - 7.1
				* log10(wStreet_) + 7.5
				* log10(hBuilding_)
				- (24.37 - 3.7 * pow((hBuilding_ / hNodeB_), 2))
				* log10(hNodeB_) + (43.42 - 3.1 * log10(hNodeB_))
				* (log10(d3d) - 3) + 20
				* log10(carrierFrequency_)
				- (3.2 * pow((log10(11.75 * hUe_)), 2) - 4.97);
	}
	else
		throw cRuntimeError("Error Path Loss RMa --> invalid distance (2d)");

	//std::cout << "NRRealisticChannelModel::computePLrmaNlos end at " << simTime().dbl() << std::endl;
}

/*
 * Table A1-5, LMLC not implemented
 */
double NRRealisticChannelModel::computeRMaA(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeRMaA start at " << simTime().dbl() << std::endl;

//check range of parameters --> parameterRanges.txt
	if (!(5 <= hBuilding_ && hBuilding_ <= 50 && 5 <= wStreet_ && wStreet_ <= 50 && 10 <= hNodeB_ && hNodeB_ <= 150 && 1 <= hUe_ && hUe_ <= 10))
		throw cRuntimeError("Error Path Loss RMaA --> invalid, height enodeb or ue not valid --> see parameterRanges.txt");

	if (d2ddistance < 10)
		d2ddistance = 10;
	if (d3ddistance < 10)
		d3ddistance = 10;
	if (d2ddistance > 21000) {
		if (tolerateMaxDistViolation_)
			return ATT_MAXDISTVIOLATED;
		else
			throw cRuntimeError("Error LOS RMaA path loss model is valid for d<5000 m");
	}

	if (losMap_[nodeId]) {
		//LOS
		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6)
			return computePLrmaLos(d3ddistance, d2ddistance);
		else
			throw cRuntimeError("Error LOS RMaA path loss model is not valid --> frequency not valid");
	} else {
		// LMLC NOT IMPLEMENTED!
		//NLOS
		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6)
			return computePLrmaNlos(d3ddistance, d2ddistance);
		else
			throw cRuntimeError("Error NLOS RMaA path loss model is not valid --> frequency not valid");
	}

	//std::cout << "NRRealisticChannelModel::computeRMaA end at " << simTime().dbl() << std::endl;
}

/*
 * from ITU-R M.2412-0, also defined in 3gpp 38.901 Table 7.4.1-1 (RMa)
 */
double NRRealisticChannelModel::computeRMaB(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeRMaB start at " << simTime().dbl() << std::endl;

//check range of parameters --> parameterRanges.txt
	if (!(5 <= hBuilding_ && hBuilding_ <= 50 && 5 <= wStreet_ && wStreet_ <= 50 && 10 <= hNodeB_ && hNodeB_ <= 150 && 1 <= hUe_ && hUe_ <= 10))
		throw cRuntimeError("Error Path Loss RMaB --> invalid, height gnodeb or ue not valid --> see parameterRanges.txt");

	if (d2ddistance < 10)
		d2ddistance = 10;
	if (d3ddistance < 10)
		d3ddistance = 10;
	if (d2ddistance > 21000) {
		if (tolerateMaxDistViolation_)
			return ATT_MAXDISTVIOLATED;
		else
			throw cRuntimeError("Error LOS RMaB path loss model is valid for d<5000 m");
	}

	if (losMap_[nodeId]) {
		//LOS
		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 30)
			return computePLrmaLos(d3ddistance, d2ddistance);
		else
			throw cRuntimeError("Error LOS RMaB path loss model is not valid --> frequency not valid");
	} else {
		// LMLC NOT IMPLEMENTED
		//NLOS
		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 30) {
			double plRmaLos = computePLrmaLos(d3ddistance, d2ddistance);
			double plRmaNlos = computePLrmaNlos(d3ddistance, d2ddistance);
			return max(plRmaLos, plRmaNlos);
		} else
			throw cRuntimeError("Error NLOS RMaB path loss model is not valid --> frequency not valid");
	}

	//std::cout << "NRRealisticChannelModel::computeRMaB end at " << simTime().dbl() << std::endl;
}

double NRRealisticChannelModel::computeUMiB(double &d3d, double &d2d, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeUMiB start at " << simTime().dbl() << std::endl;

	if (!(hNodeB_ == 10 && (1.5 <= hUe_ && hUe_ <= 22.5)))
		throw cRuntimeError("Error Path Loss UMiB --> invalid, height enodeb or ue not valid");

	if (d2d < 10)
		d2d = 10;
	if (d3d < 10)
		d3d = 10;
	if (d2d > 5000) {
		if (tolerateMaxDistViolation_)
			return ATT_MAXDISTVIOLATED;
		else
			throw cRuntimeError("Error LOS UMaA path loss model is valid for d<5000 m");
	}

	if (losMap_[nodeId]) {
		//LOS
		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
			return computePLumiBLos(d3d, d2d);
		} else
			throw cRuntimeError("Error LOS UMiB path loss model is not valid --> frequency not valid");
	} else {
		//NLOS
		double plumiLos;
		double plumiNlos;

		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
			if (10 <= d2d && d2d <= 5000) {
				plumiLos = computePLumiBLos(d3d, d2d);
				plumiNlos = 35.3 * log10(d3d) + 22.4 + 21.3 * log10(carrierFrequency_) - 0.3 * (hUe_ - 1.5);
			} else
				throw cRuntimeError("Error LOS UMiB path loss model is not valid --> distance not valid (d > 5000m");
		} else
			throw cRuntimeError("Error LOS UMiB path loss model is not valid --> frequency not valid");

		return max(plumiLos, plumiNlos);
	}

	//std::cout << "NRRealisticChannelModel::computeUMiB end at " << simTime().dbl() << std::endl;
}

double NRRealisticChannelModel::computeUMiA(double &d3d, double &d2d, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeUMiA start at " << simTime().dbl() << std::endl;

	if (!(/*hNodeB_ == 10 &&*/(1.5 <= hUe_ && hUe_ <= 22.5)))
		throw cRuntimeError("Error Path Loss UMiA --> invalid, height enodeb or ue not valid");

	if (d2d < 10)
		d2d = 10;
	if (d3d < 10)
		d3d = 10;
	if (d2d > 5000) {
		if (tolerateMaxDistViolation_)
			return ATT_MAXDISTVIOLATED;
		else
			throw cRuntimeError("Error LOS UMiA path loss model is valid for d<5000 m");
	}

	if (losMap_[nodeId]) {
		//LOS
		return computePLumiALos(d3d, d2d);
	} else {
		//NLOS
		if (!(0.5 <= carrierFrequency_ && carrierFrequency_ <= 100))
			throw cRuntimeError("Error LOS UMiA  path loss model --> frequency not valid");

		//Calc PLuma-los first
		double plUmaLos;
		double plumaNlos;

		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
			if (d2d > 2000) {
				if (tolerateMaxDistViolation_)
					return ATT_MAXDISTVIOLATED;
				else
					throw cRuntimeError("Error LOS UMiA path loss model is valid for d<5000 m");
			}

			plUmaLos = computePLumiALos(d3d, d2d);
			plumaNlos = 36.7 * log10(d3d) + 22.7 + 26 * log10(carrierFrequency_) - 0.3 * (hUe_ - 1.5);
		} else if (6 < carrierFrequency_ && carrierFrequency_ <= 100) {

			if (!(10 < d2d && d2d < 5000))
				throw cRuntimeError("Error LOS UMiA path loss model --> distance not valid");

			plUmaLos = computePLumiALos(d3d, d2d);
			plumaNlos = 35.3 * log(d3d) + 22.4 + 21.3 * log10(carrierFrequency_) - 0.3 * (hUe_ - 1.5);
		}

		//std::cout << "NRRealisticChannelModel::computeUMiA end at " << simTime().dbl() << std::endl;

		return max(plUmaLos, plumaNlos);
	}
}

/*
 * table A1-3
 */
double NRRealisticChannelModel::computeUMaB(double &d3d, double &d2d, const MacNodeId &nodeId) {

	//std::cout << "NRRealisticChannelModel::computeUMaB start at " << simTime().dbl() << std::endl;

	if (d2d < 10)
		d2d = 10;
	if (d3d < 10)
		d3d = 10;
	if (d2d > 5000) {
		if (tolerateMaxDistViolation_)
			return ATT_MAXDISTVIOLATED;
		else
			throw cRuntimeError("Error LOS UMaB path loss model is valid for d<5000 m");
	}

	if (!(hNodeB_ == 25 && (1.5 <= hUe_ && hUe_ <= 22.5)))
		throw cRuntimeError("Error Path Loss UMaB --> invalid, height enodeb or ue not valid");

	if (losMap_[nodeId]) {
		//LOS

		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
			return computePLumaLos(d3d, d2d);
		} else
			throw cRuntimeError("Error LOS UMaB path loss model is not valid --> frequency not valid");
	} else {
		//NLOS
		if (!(0.5 <= carrierFrequency_ && carrierFrequency_ <= 100))
			throw cRuntimeError("Error LOS UMaB  path loss model --> frequency not valid");

		//Calc PLuma-los first
		double plUmaLos = computePLumaLos(d3d, d2d);
		double plumaNlos = 13.54 + 39.08 * log10(d3d) + 20 * log10(carrierFrequency_) - 0.6 * (hUe_ - 1.5);

		//std::cout << "NRRealisticChannelModel::computeUMaB end at " << simTime().dbl() << std::endl;

		return max(plUmaLos, plumaNlos);
	}
}

/*
 * table A1-4, UMi_B, LOS
 */
double NRRealisticChannelModel::computePLumiBLos(const double &d3d, double &d2d) {
	//std::cout << "NRRealisticChannelModel::computePLumiBLos start at " << simTime().dbl() << std::endl;

	double dbp = calcDistanceBreakPoint(d2d);

	if (10 <= d2d && d2d <= dbp)
		return 32.4 + 21.0 * log10(d3d) + 20.0 * log10(carrierFrequency_);
	else if (dbp <= d2d && d2d <= 5000)
		return 32.4 + 40.0 * log10(d3d) + 20.0 * log10(carrierFrequency_) - 9.5 * log10(pow(dbp, 2) + pow(hNodeB_ - hUe_, 2));
	else
		throw cRuntimeError("Error LOS UMiA/UMiB path loss model --> distance not valid");

	//std::cout << "NRRealisticChannelModel::computePLumiBLos end at " << simTime().dbl() << std::endl;
}

/*
 * table A1-4, UMi_A, LOS
 */
double NRRealisticChannelModel::computePLumiALos(const double &d3d, double &d2d) {

	//std::cout << "NRRealisticChannelModel::computePLumiALos start at " << simTime().dbl() << std::endl;

	if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
		//Note 3
		double dbp = calcDistanceBreakPoint(d2d);
		if (10 < d2d && d2d < dbp)
			return 22.0 * log10(d3d) + 28.0 + 20 * log10(carrierFrequency_);
		else if (dbp < d2d && d2d < 5000)
			return 40 * log10(d3d) + 28.0 + 20 * log10(carrierFrequency_) - 9 * log10(pow(dbp, 2) + pow(hNodeB_ - hUe_, 2));
		else
			throw cRuntimeError("Error LOS UMiA path loss model --> distance not valid");

	} else if (6 < carrierFrequency_ && carrierFrequency_ <= 100) {

		//std::cout << "NRRealisticChannelModel::computePLumiALos end at " << simTime().dbl() << std::endl;
		return computePLumiBLos(d3d, d2d);
	} else
		throw cRuntimeError("Error LOS UMiA path loss model is not valid --> frequency not valid");
}

/*
 * table A1-3, UMa_A, LOS
 */
double NRRealisticChannelModel::computePLumaLos(const double &d3d, double &d2d) {
//dbp --> see Note 3: d'BP --> Note 3: 4 * h'BS * h'UT * f

	//std::cout << "NRRealisticChannelModel::computePLumaLos start at " << simTime().dbl() << std::endl;

	double dbp = calcDistanceBreakPoint(d2d);
	if (10 <= d2d && d2d <= dbp) {
		//PL1
		return 28.0 + 22 * log10(d3d) + 20 * log10(carrierFrequency_);
	} else if (dbp <= d2d && d2d <= 5000) {
		//PL2
		return 40 * log10(d3d) + 28.0 + 20 * log10(carrierFrequency_) - 9 * log10((pow(dbp, 2) + pow((hNodeB_ - hUe_), 2)));
	} else
		throw cRuntimeError("Error LOS UMaA path loss model --> distance not valid");

	//std::cout << "NRRealisticChannelModel::computePLumaLos end at " << simTime().dbl() << std::endl;
}

/*
 * table A1-3
 */
double NRRealisticChannelModel::computeUMaA(double &d3d, double &d2d, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeUMaA start at " << simTime().dbl() << std::endl;

	if (d2d < 10)
		d2d = 10;
	if (d3d < 10)
		d3d = 10;
	if (d2d > 5000) {
		if (tolerateMaxDistViolation_)
			return ATT_MAXDISTVIOLATED;
		else
			throw cRuntimeError("Error LOS UMaA path loss model is valid for d<5000 m");
	}

	if (!(hNodeB_ == 25 && (1.5 <= hUe_ && hUe_ <= 22.5)))
		throw cRuntimeError("Error Path Loss UMa_A invalid, height enodeb or ue not valid");

	if (losMap_[nodeId]) {
		//LOS
		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 100) {
			return computePLumaLos(d3d, d2d);
		} else
			throw cRuntimeError("Error LOS UMaA path loss model is not valid --> frequency not valid");
	} else {
		//NLOS

		if (!(0.5 <= carrierFrequency_ && carrierFrequency_ <= 100))
			throw cRuntimeError("Error LOS UMaA path loss model --> frequency not valid");

		//Calc PLuma-los first
		double plumaLos;
		double plumaNlos;

		if (0.5 <= carrierFrequency_ && carrierFrequency_ <= 6) {
			if (!(10 <= d2d && d2d <= 5000 && wStreet_ == 20 /*&& hBuilding_ == 20*/))
				throw cRuntimeError("Error LOS UMaA path loss model --> d2d, wStreet or hBuidling not valid");
			plumaLos = computePLumaLos(d3d, d2d);
			plumaNlos = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_) - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_) + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d3d) - 3)
					+ 20 * log10(carrierFrequency_) - (3.2 * pow(log10(17.625), 2) - 4.97) - 0.6 * (hUe_ - 1.5);

		} else if (6 < carrierFrequency_ && carrierFrequency_ <= 100) {
			plumaLos = computePLumaLos(d3d, d2d);
			plumaNlos = 13.54 + 39.08 * log10(d3d) + 20 * log10(carrierFrequency_) - 0.6 * (hUe_ - 1.5);
		}
		//std::cout << "NRRealisticChannelModel::computeUMaA end at " << simTime().dbl() << std::endl;

		return max(plumaLos, plumaNlos);
	}
}

/*
 * Represents Table A1-3 A1-4 taken from ITU-R M.2412-0
 * Optional Formulas / Models not implemented!
 */
double NRRealisticChannelModel::computeDenseUrbanEmbb(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeDenseUrbanEmbb start at " << simTime().dbl() << std::endl;

	if (channelModelType_ == UMa_A) {
		return computeUMaA(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == UMa_B) {
		return computeUMaB(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == UMi_A) {
		return computeUMiA(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == UMi_B) {
		return computeUMiB(d3ddistance, d2ddistance, nodeId);
	}
	throw cRuntimeError("Error LOS Dense Urban Embb path loss model is not valid --> invalid channelModel");
}

double NRRealisticChannelModel::computeRuralEmbb(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {

	//std::cout << "NRRealisticChannelModel::computeRuralEmbb start at " << simTime().dbl() << std::endl;
	if (channelModelType_ == RMa_A) {
		return computeRMaA(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == RMa_B) {
		return computeRMaB(d3ddistance, d2ddistance, nodeId);
	} else
		throw cRuntimeError("Error LOS RuralEmbb path loss model is not valid --> invalid channelModel");
}

double NRRealisticChannelModel::computeUrbanMacroMmtc(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {

	//std::cout << "NRRealisticChannelModel::computeUrbanMacroMmtc start at " << simTime().dbl() << std::endl;
	if (channelModelType_ == UMa_A) {
		return computeUMaA(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == UMa_B) {
		return computeUMaB(d3ddistance, d2ddistance, nodeId);
	} else
		throw cRuntimeError("Error LOS Urban Macro Mmtc path loss model is not valid --> invalid channelModel");
}

double NRRealisticChannelModel::computeUrbanMacroUrllc(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeUrbanMacroUrllc start at " << simTime().dbl() << std::endl;

	if (channelModelType_ == UMa_A) {
		return computeUMaA(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == UMa_B) {
		return computeUMaB(d3ddistance, d2ddistance, nodeId);
	} else
		throw cRuntimeError("Error LOS UrbanMacroUrllc path loss model is not valid --> invalid channelModel");
}

/*
 * Taken from 38.901, IndoorFactory, Table 7.4.1-1
 */
double NRRealisticChannelModel::computeIndoorFactory(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeIndoorFactory start at " << simTime().dbl() << std::endl;
	double pathloss = 0.0;
	if (losMap_[nodeId]) {
		pathloss = computeInFLOS(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == InFSL) {
		return computeInFSL(d3ddistance, d2ddistance, nodeId);
	} else if (channelModelType_ == InFDL) {
		double pathlossLOS = computeInFLOS(d3ddistance, d2ddistance, nodeId);
		double pathlossNLOS = 33.0 + 25.5 * log10(d3ddistance) + 20.0 * log10(carrierFrequency_);
		double pathlossInFSL = computeInFSL(d3ddistance, d2ddistance, nodeId);
		pathloss = max(pathlossLOS, max(pathlossNLOS, pathlossInFSL));
	} else if (channelModelType_ == InFSH) {
		double pathlossLOS = computeInFLOS(d3ddistance, d2ddistance, nodeId);
		double pathlossNLOS = 32.4 + 23.0 * log10(d3ddistance) + 20.0 * log10(carrierFrequency_);
		pathloss = max(pathlossLOS, pathlossNLOS);
	} else if (channelModelType_ == InFDH) {
		double pathlossLOS = computeInFLOS(d3ddistance, d2ddistance, nodeId);
		double pathlossNLOS = 33.63 + 21.9 * log10(d3ddistance) + 20.0 * log10(carrierFrequency_);
		pathloss = max(pathlossLOS, pathlossNLOS);
	}
	return pathloss;
	throw cRuntimeError("Error LOS Indoor Factory path loss model is not valid --> invalid channelModel");
}

double NRRealisticChannelModel::computeInFLOS(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeInFLOS start at " << simTime().dbl() << std::endl;
	return (31.84 + 21.50 * log10(d3ddistance) + 19.00 * log10(carrierFrequency_));
}

double NRRealisticChannelModel::computeInFSL(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeInFSL start at " << simTime().dbl() << std::endl;
	double pathlossLOS = 31.84 + 21.50 * log10(d3ddistance) + 19.00 * log10(carrierFrequency_);
	double pathlossNLOS = 33.0 + 25.5 * log10(d3ddistance) + 20.0 * log10(carrierFrequency_);
	return max(pathlossLOS, pathlossNLOS);
}

//TODO
double NRRealisticChannelModel::getAttenuation_D2D(MacNodeId nodeId, Direction dir, inet::Coord coord, MacNodeId node2_Id, inet::Coord coord_2) {
	return LteRealisticChannelModel::getAttenuation_D2D(nodeId, dir, coord, node2_Id, coord_2);
}

//TODO
std::vector<double> NRRealisticChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord) {
	return LteRealisticChannelModel::getRSRP_D2D(frame, lteInfo_1, destId, destCoord);
}

//TODO
bool NRRealisticChannelModel::error_D2D(LteAirFrame *frame, UserControlInfo *lteI, std::vector<double> rsrpVector) {
	return LteRealisticChannelModel::error_D2D(frame, lteI, rsrpVector);
}

bool NRRealisticChannelModel::isCorrupted(LteAirFrame *frame, UserControlInfo *lteInfo) {
	//std::cout << "NRRealisticChannelModel::error start at " << simTime().dbl() << std::endl;

	bool tmp = false;
	MacNodeId ueId = 0;
	MacNodeId eNbId = 0;

	Direction dir = (Direction) lteInfo->getDirection();

	//Get MacNodeId of UE
	if (dir == DL) {
//        id = lteInfo->getDestId();
		ueId = lteInfo->getDestId();
		eNbId = lteInfo->getSourceId();
	} else {
//        id = lteInfo->getSourceId();
		ueId = lteInfo->getSourceId();
		eNbId = lteInfo->getDestId();
	}

	//get codeword
	unsigned char cw = lteInfo->getCw();
	//get number of codeword
	int size = lteInfo->getUserTxParams()->readCqiVector().size();

	//get position associated to the packet
	Coord coord = lteInfo->getCoord();

	//if total number of codeword is equal to 1 the cw index should be only 0
	if (size == 1)
		cw = 0;

	//get cqi used to transmit this cw
	Cqi cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];

	MacNodeId id;

	//Get MacNodeId of UE
	if (dir == DL)
		id = lteInfo->getDestId();
	else
		id = lteInfo->getSourceId();

	// Get Number of RTX
	unsigned char nTx = lteInfo->getTxNumber();

	//consistency check
	if (nTx == 0)
		throw cRuntimeError("Transmissions counter should not be 0");

	//Get txmode
	TxMode txmode = (TxMode) lteInfo->getTxMode();

	// If rank is 1 and we used SMUX to transmit we have to corrupt this packet
	if (txmode == CL_SPATIAL_MULTIPLEXING || txmode == OL_SPATIAL_MULTIPLEXING) {
		//compare lambda min (smaller eingenvalues of channel matrix) with the threshold used to compute the rank
		if (binder_->phyPisaData.getLambda(id, 1) < lambdaMinTh_)
			return false;
	}

	// Take sinr
	std::vector<double> snrV;
	if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI) {
		MacNodeId destId = lteInfo->getDestId();
		Coord destCoord = phy_->getCoord();
		MacNodeId enbId = binder_->getNextHop(lteInfo->getSourceId());
		snrV = getSINR_D2D(frame, lteInfo, destId, destCoord, enbId);
	} else {
		snrV = getSINR(frame, lteInfo, true);
	}

	//Get the resource Block id used to transmist this packet
	RbMap rbmap = lteInfo->getGrantedBlocks();

	//Get txmode
	unsigned int itxmode = txModeToIndex[txmode];

	double bler = 0;
	std::vector<double> totalbler;
	double finalSuccess = 1;
	RbMap::iterator it;
	std::map<Band, unsigned int>::iterator jt;

	// for statistic purposes
	double sumSnr = 0.0;
	int usedRBs = 0;

	//for each Remote unit used to transmit the packet
	for (it = rbmap.begin(); it != rbmap.end(); ++it) {
		//for each logical band used to transmit the packet
		for (jt = it->second.begin(); jt != it->second.end(); ++jt) {
			//this Rb is not allocated
			if (jt->second == 0)
				continue;

			//check the antenna used in Das
			if ((lteInfo->getTxMode() == CL_SPATIAL_MULTIPLEXING || lteInfo->getTxMode() == OL_SPATIAL_MULTIPLEXING) && rbmap.size() > 1)
				//we consider only the snr associated to the LB used
				if (it->first != lteInfo->getCw())
					continue;

			//Get the Bler
			if (cqi == 0 || cqi > 15)
				throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", cqi, lteInfo->getTxMode(), dir, jt->second, cw,
						nTx);

			// for statistic purposes
			sumSnr += snrV[jt->first];
			usedRBs++;

			int snr = snrV[jt->first];        //XXX because jt->first is a Band (=unsigned short)
			if (snr < 0)
				return false;
			else if (snr > binder_->phyPisaData.maxSnr())
				bler = 0;
			else
				bler = binder_->phyPisaData.getBler(itxmode, cqi - 1, snr);

			//EV << "\t bler computation: [itxMode=" << itxmode << "] - [cqi-1=" << cqi-1                   << "] - [snr=" << snr << "]" << endl;

			double success = 1 - bler;
			//compute the success probability according to the number of RB used
			double successPacket = pow(success, (double) jt->second);
			// compute the success probability according to the number of LB used
			finalSuccess *= successPacket;

			//EV << " NRRealisticChannelModel::error direction " << dirToA(dir) << " node " << id << " remote unit " << dasToA((*it).first) << " Band " << (*jt).first << " SNR " << snr << " CQI "<< cqi << " BLER " << bler << " success probability " << successPacket << " total success probability " << finalSuccess << endl;
		}
	}

	//Compute total error probability
	double per = 1 - finalSuccess;
	//Harq Reduction
	double totalPer = per * pow(harqReduction_, nTx - 1);

	double er = omnetpp::uniform(getEnvir()->getRNG(0), 0.0, 1.0);

	LteControlInfo *info = check_and_cast<LteControlInfo*>(lteInfo);

	//EV << " NRRealisticChannelModel::error direction " << dirToA(dir) << " node " << id << " total ERROR probability  " << per << " per with H-ARQ error reduction " << totalPer << " - CQI[" << cqi << "]- random error extracted[" << er << "]" << endl;

	if (er <= totalPer) {
		//EV << "This is NOT your lucky day (" << er << " < " << totalPer << ") -> do not receive." << endl;
		// Signal too weak, we can't receive it
		tmp = false;

		//simplified consideration of codeblockgroups
		if (getSimulation()->getSystemModule()->par("useCodeBlockGroups").boolValue() && lteInfo->getFrameType() == DATAPKT) {
			considerCodeBlockGroups(info, nTx, totalPer, frame);
		}
		//

	} else {
		// Signal is strong enough, receive this Signal
		//EV << "This is your lucky day (" << er << " > " << totalPer << ") -> Receive AirFrame." << endl;
		tmp = true;
	}

	if (!tmp) {
		if (isNodeB_) {
			NRPhyGnb *nrPhy = check_and_cast<NRPhyGnb*>(getSimulation()->getModule(binder_->getOmnetId(eNbId))->getSubmodule("lteNic")->getSubmodule("phy"));
			nrPhy->errorDetected();

		} else {
			NRPhyUe *nrPhy = check_and_cast<NRPhyUe*>(getSimulation()->getModule(binder_->getOmnetId(ueId))->getSubmodule("lteNic")->getSubmodule("phy"));
			nrPhy->errorDetected();
		}
	}

	//std::cout << "NRRealisticChannelModel::error end at " << simTime().dbl() << std::endl;

	return tmp;
//	return true;
}

void NRRealisticChannelModel::considerCodeBlockGroups(LteControlInfo *& info, unsigned char & nTx, double & totalPer, LteAirFrame *& frame){

	//std::cout << "NRRealisticChannelModel::considerCodeBlockGroups start at " << simTime().dbl() << std::endl;

	unsigned int numberOfCodeBlockGroups = getSimulation()->getSystemModule()->par("numberOfCodeBlockGroups").intValue();
	ASSERT(numberOfCodeBlockGroups == 2 || numberOfCodeBlockGroups == 4 || numberOfCodeBlockGroups == 6 || numberOfCodeBlockGroups == 8);
	unsigned int wholeTransportBlockByteSize = frame->getByteLength();
	unsigned int corruptedBytes, bytesOfOneCodeBlockGroup, numberOfCodeBlockGroupToRetransmit = 0;

	//calculate corrupted bytes -> these must be retransmitted
	corruptedBytes = ceil(wholeTransportBlockByteSize * totalPer);

	if (nTx == 1) {

		//calculate the number of bytes of one codeBlockGroup
		bytesOfOneCodeBlockGroup = ceil(double(wholeTransportBlockByteSize) / double(numberOfCodeBlockGroups));
		info->setInitialByteSize(wholeTransportBlockByteSize);

		//if one codeblockgroup is 20 bytes large or less --> do not consider it!
		if(bytesOfOneCodeBlockGroup <= 20){
			info->setRestByteSize(wholeTransportBlockByteSize);
			info->setCodeBlockGroupsActivated(true);
			info->setNumberOfCodeBlockGroups(numberOfCodeBlockGroups);
			return;
		}

	} else {

		//if one codeblockgroup has left --> return
		if (info->getBlocksForCodeBlockGroups() <= 1) {
			return;
		}

		//reduced to the rest size from previous transmission
		wholeTransportBlockByteSize = info->getRestByteSize();

		//how many bytes in one codeblockgroup
		bytesOfOneCodeBlockGroup = ceil(double(info->getInitialByteSize()) / double(numberOfCodeBlockGroups));

		//calculate corrupted bytes -> has to be retransmitted
		//totalPer considers the number of resource blocks which were used for that transmission (not for the first transmission)
		corruptedBytes = ceil(wholeTransportBlockByteSize * totalPer);
	}

	for (unsigned int i = 1; i <= numberOfCodeBlockGroups; i++) {
		//find the numberOfCodeBlockGroups which cover the erroneous bytes
		if (i * bytesOfOneCodeBlockGroup >= corruptedBytes) {
			numberOfCodeBlockGroupToRetransmit = i;
			break;
		}
	}


	unsigned int numberOfBytesToRetransmit = numberOfCodeBlockGroupToRetransmit * bytesOfOneCodeBlockGroup;
	//this is needed to guarantee that the original size of the transport block is not exceeded
	numberOfBytesToRetransmit = min(numberOfBytesToRetransmit, info->getInitialByteSize());
	info->setRestByteSize(numberOfBytesToRetransmit);
	info->setCodeBlockGroupsActivated(true);
	info->setNumberOfCodeBlockGroups(numberOfCodeBlockGroups);
	info->setBlocksForCodeBlockGroups(numberOfCodeBlockGroupToRetransmit);

	//std::cout << "NRRealisticChannelModel::considerCodeBlockGroups end at " << simTime().dbl() << std::endl;

}

double NRRealisticChannelModel::computeExtCellPathLossNR(double &d3ddistance, double &d2ddistance, const MacNodeId &nodeId) {
	//std::cout << "NRRealisticChannelModel::computeExtCellPathLossNR start at " << simTime().dbl() << std::endl;

//	double movement = .0;
//	double speed = .0;
//
//	speed = computeSpeed(nodeId, myCoord3d, movement);

//compute attenuation based on selected scenario and based on LOS or NLOS
	double attenuation = 0;
//    double dbp = 0;
	switch (scenarioNR_) {
	case INDOOR_HOTSPOT_EMBB:
		attenuation = computeIndoorHotspot(d3ddistance, d2ddistance, nodeId);
		break;
	case DENSE_URBAN_EMBB:
		attenuation = computeDenseUrbanEmbb(d3ddistance, d2ddistance, nodeId);
		break;
	case RURAL_EMBB:
		attenuation = computeRuralEmbb(d3ddistance, d2ddistance, nodeId);
		break;
	case URBAN_MACRO_MMTC:
		attenuation = computeUrbanMacroMmtc(d3ddistance, d2ddistance, nodeId);
		break;
	case URBAN_MACRO_URLLC:
		attenuation = computeUrbanMacroUrllc(d3ddistance, d2ddistance, nodeId);
		break;
	default:
		throw cRuntimeError("Wrong value %d for path-loss scenario", scenarioNR_);

	}

//    Applying shadowing only if it is enabled by configuration
//    log-normal shadowing
	if (shadowing_) {
		double att = lastComputedSF_.at(nodeId).second;

		//EV << "(" << att << ")";
		attenuation += att;
	}

	//std::cout << "NRRealisticChannelModel::computeExtCellPathLossNR end at " << simTime().dbl() << std::endl;

	return attenuation;
}

/*
 * interference from ue to other cells
 */
bool NRRealisticChannelModel::computeMultiCellInterferenceNR(const MacNodeId &eNbId, const MacNodeId &ueId, const Coord &ueCoord, bool isCqi, std::vector<double> &interference, Direction dir,
		const Coord &enodebcoord) {
	//std::cout << "NRRealisticChannelModel::computeMultiCellInterferenceNR start at " << simTime().dbl() << std::endl;

	//EV << "**** Multi Cell Interference ****" << endl;

// reference to the mac/phy/channel of each cell
	LtePhyBase *ltePhy;

	int temp;
	double att;

	double txPwr;

	std::vector<EnbInfo*> *enbList = binder_->getEnbList();
	std::vector<EnbInfo*>::iterator it = enbList->begin(), et = enbList->end();

	while (it != et) {
		MacNodeId id = (*it)->id;

		if (id == eNbId) {
			++it;
			continue;
		}

		// initialize data structures
		Coord enbCoord;
		if (!(*it)->init) {
			// obtain a reference to enb phy and obtain tx power
			ltePhy = check_and_cast<LtePhyBase*>(getSimulation()->getModule(binder_->getOmnetId(id))->getSubmodule("lteNic")->getSubmodule("phy"));
			(*it)->txPwr = ltePhy->getTxPwr();    //dBm

			// get tx direction
			(*it)->txDirection = ltePhy->getTxDirection();

			// get tx angle
			(*it)->txAngle = ltePhy->getTxAngle();

			// get real Channel

			(*it)->realChan = dynamic_cast<NRRealisticChannelModel*>(ltePhy->getChannelModel());
			enbCoord = dynamic_cast<NRRealisticChannelModel*>((*it)->realChan)->getMyPosition();

			//get reference to mac layer
			(*it)->mac = check_and_cast<NRMacGnb*>(getMacByMacNodeId(id));

			(*it)->init = true;
		}

		// compute attenuation using data structures within the cell

		att = dynamic_cast<NRRealisticChannelModel*>((*it)->realChan)->getAttenuationNR(ueId, UL, ueCoord, enbCoord, false);
		//EV << "EnbId [" << id << "] - attenuation [" << att << "]" << endl;

		//=============== angular ATTENUATION =================
		double angularAtt = 0;
		if ((*it)->txDirection == ANISOTROPIC) {
			//get tx angle
			double txAngle = (*it)->txAngle;

			// compute the angle between uePosition and reference axis, considering the eNb as center
			double ueAngle = computeAngle(dynamic_cast<NRRealisticChannelModel*>((*it)->realChan)->myCoord3d, ueCoord);

			// compute the reception angle between ue and eNb
			double recvAngle = fabs(txAngle - ueAngle);
			if (recvAngle > 180)
				recvAngle = 360 - recvAngle;

			// compute attenuation due to sectorial tx
			angularAtt = computeAngularAttenuation(recvAngle);

		}
		// else, antenna is omni-directional
		//=============== END angular ATTENUATION =================

		txPwr = (*it)->txPwr - angularAtt - cableLoss_ + antennaGainGnB_ + antennaGainUe_;

		if (true)        // check slot occupation for this TTI
		{
			for (unsigned int i = 0; i < band_; i++) {
				// compute the number of occupied slot (unnecessary)
				temp = (*it)->mac->getDlBandStatus(i);
				if (temp != 0)
					interference[i] += dBmToLinear(txPwr - att);       //(dBm-dB)=dBm

				//EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << interference[i] << "]" << endl;
			}
		} else // error computation. We need to check the slot occupation of the previous TTI
		{
			for (unsigned int i = 0; i < band_; i++) {

				// compute the number of occupied slot (unnecessary)
				temp = (*it)->mac->getDlPrevBandStatus(i);
				if (temp != 0)
					interference[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

				//EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << interference[i] << "]" << endl;
			}
		}
		++it;
	}

	//std::cout << "NRRealisticChannelModel::computeMultiCellInterferenceNR end at " << simTime().dbl() << std::endl;

	return true;
}

//NEW
bool NRRealisticChannelModel::computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, Coord ueCoord, bool isCqi, RbMap rbmap, std::vector<double> *interference) {
	//EV << "**** Downlink Interference ****" << endl;

	// reference to the mac/phy/channel of each cell
	LtePhyBase *ltePhy;

	int temp;
	double att;

	double txPwr;

	std::vector<EnbInfo*> *enbList = binder_->getEnbList();
	std::vector<EnbInfo*>::iterator it = enbList->begin(), et = enbList->end();

	//iterate over all nodeBs and calculate the attenuation measured at the ue

	while (it != et) {
		MacNodeId id = (*it)->id;

		if (id == eNbId) {
			++it;
			continue;
		}

		// initialize eNb data structures
		if (!(*it)->init) {
			// obtain a reference to enb phy and obtain tx power
			ltePhy = check_and_cast<LtePhyBase*>(getSimulation()->getModule(binder_->getOmnetId(id))->getSubmodule("lteNic")->getSubmodule("phy"));
			(*it)->txPwr = ltePhy->getTxPwr(); //dBm

			// get tx direction
			(*it)->txDirection = ltePhy->getTxDirection();

			// get tx angle
			(*it)->txAngle = ltePhy->getTxAngle();

			// get real Channel
			(*it)->realChan = dynamic_cast<NRRealisticChannelModel*>(ltePhy->getChannelModel());

			(*it)->position = dynamic_cast<NRRealisticChannelModel*>((*it)->realChan)->getMyPosition();

			//get reference to mac layer
			(*it)->mac = check_and_cast<NRMacGnb*>(getMacByMacNodeId(id));

			(*it)->init = true;
		}

		if ((ueId >= UE_MIN_ID && ueId <= UE_MAX_ID)) {
			//we use the physical layer from the UE to calculate the attenuation
			LtePhyBase *uePhy = check_and_cast<LtePhyBase*>(getSimulation()->getModule(binder_->getOmnetId(ueId))->getSubmodule("lteNic")->getSubmodule("phy"));
			att = dynamic_cast<NRRealisticChannelModel*>(uePhy->getChannelModel())->getAttenuationNR(ueId, DL, ueCoord, (*it)->position, false);
		} else {
			//this happens during initialize function, ueId is not determined so far
			//--> call getAttenuation from cellPhy
			att = dynamic_cast<NRRealisticChannelModel*>((*it)->realChan)->getAttenuationNR(ueId, DL, ueCoord, (*it)->position, false);
			//EV << "EnbId [" << id << "] - attenuation [" << att << "]" << endl;
		}

		//=============== ANGOLAR ATTENUATION =================
		double angolarAtt = 0;
		if ((*it)->txDirection == ANISOTROPIC) {
			//get tx angle
			double txAngle = (*it)->txAngle;

			// compute the angle between uePosition and reference axis, considering the eNb as center
			double ueAngle = computeAngle((*it)->position, ueCoord);

			// compute the reception angle between ue and eNb
			double recvAngle = fabs(txAngle - ueAngle);
			if (recvAngle > 180)
				recvAngle = 360 - recvAngle;

			// compute attenuation due to sectorial tx
			angolarAtt = computeAngularAttenuation(recvAngle);

		}
		// else, antenna is omni-directional
		//=============== END ANGOLAR ATTENUATION =================

		txPwr = (*it)->txPwr - angolarAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;

		if (isCqi)       // check slot occupation for this TTI
		{
			for (unsigned int i = 0; i < band_; i++) {
				// compute the number of occupied slot (unnecessary)
				temp = (*it)->mac->getDlBandStatus(i);
				if (temp != 0)
					(*interference)[i] += dBmToLinear(txPwr - att);       //(dBm-dB)=dBm

				//EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
			}
		} else // error computation. We need to check the slot occupation of the previous TTI
		{
			for (unsigned int i = 0; i < band_; i++) {
				// if we are decoding a data transmission and this RB has not been used, skip it
				// TODO fix for multi-antenna case
				if (rbmap[MACRO][i] == 0)
					continue;

				// compute the number of occupied slot (unnecessary)
				temp = (*it)->mac->getDlPrevBandStatus(i);
				if (temp != 0)
					(*interference)[i] += dBmToLinear(txPwr - att);               //(dBm-dB)=dBm

				//EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
			}
		}
		++it;
	}

	return true;
}

bool NRRealisticChannelModel::computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, RbMap rbmap, std::vector<double> *interference) {
//   EV << "**** Uplink Interference for cellId[" << eNbId << "] node["<<senderId<<"] ****" << endl;

	const std::vector<UeAllocationInfo> *allocatedUes;
	std::vector<UeAllocationInfo>::const_iterator ue_it, ue_et;

	if (isCqi)               // check slot occupation for this TTI
	{
		for (unsigned int i = 0; i < band_; i++) {
			// compute the number of occupied slot (unnecessary)
			allocatedUes = binder_->getUlTransmissionMap(CURR_TTI, i);
			if (allocatedUes->empty()) // no UEs allocated on this band
				continue;

			ue_it = allocatedUes->begin(), ue_et = allocatedUes->end();
			for (; ue_it != ue_et; ++ue_it) {
				MacNodeId ueId = ue_it->nodeId;
				MacCellId cellId = ue_it->cellId;

				//node has left the simulation
				if (binder_->getOmnetId(ueId) == 0)
					continue;

				LtePhyUe *uePhy = check_and_cast<LtePhyUe*>(ue_it->phy);
				Direction dir = ue_it->dir;

				// no self interference
				if (ueId == senderId)
					continue;

				// no interference from UL/D2D connections of the same cell  (no D2D-UL reuse allowed)
				if (cellId == eNbId)
					continue;

				//EV<<NOW<<" NRRealisticChannelModel::computeUplinkInterference - Interference from UE: "<< ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

				// get tx power and attenuation from this UE
				double txPwr = uePhy->getTxPwr(dir) - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
				LtePhyBase *gNodeBPhy = check_and_cast<LtePhyBase*>(getBinder()->getMacFromMacNodeId(eNbId)->getParentModule()->getSubmodule("phy", 0));
//				double att = getAttenuationNR(ueId, UL, check_and_cast<NRRealisticChannelModel*>(uePhy->getChannelModel())->getMyPosition(),
//						check_and_cast<NRRealisticChannelModel*>(gNodeBPhy->getChannelModel())->getMyPosition(), false);
				double att = dynamic_cast<NRRealisticChannelModel*>(gNodeBPhy->getChannelModel())->getAttenuationNR(ueId, UL, check_and_cast<NRRealisticChannelModel*>(uePhy->getChannelModel())->getMyPosition(),
										check_and_cast<NRRealisticChannelModel*>(gNodeBPhy->getChannelModel())->getMyPosition(), false);
				(*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

				//EV << "\t band " << i << "/pwr[" << txPwr-att << "]-int[" << (*interference)[i] << "]" << endl;
			}
		}
	} else // Error computation. We need to check the slot occupation of the previous TTI
	{
		// For each band we have to check if the Band in the previous TTI was occupied by the interferringId
		for (unsigned int i = 0; i < band_; i++) {
			// if we are decoding a data transmission and this RB has not been used, skip it
			// TODO fix for multi-antenna case
			if (rbmap[MACRO][i] == 0)
				continue;

			allocatedUes = binder_->getUlTransmissionMap(PREV_TTI, i);

			if (allocatedUes->empty()) // no UEs allocated on this band
				continue;

			ue_it = allocatedUes->begin(), ue_et = allocatedUes->end();
			for (; ue_it != ue_et; ++ue_it) {
				MacNodeId ueId = ue_it->nodeId;
				MacCellId cellId = ue_it->cellId;

				// node has left the simulation
				if (binder_->getOmnetId(ueId) == 0)
					continue;

				LtePhyUe *uePhy = check_and_cast<LtePhyUe*>(ue_it->phy);
				Direction dir = ue_it->dir;

				// no self interference
				if (ueId == senderId)
					continue;

				// no interference from UL connections of the same cell (no D2D-UL reuse allowed)
				if (cellId == eNbId)
					continue;

				//EV<<NOW<<" NRRealisticChannelModel::computeUplinkInterference - Interference from UE: "<< ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

				// get tx power and attenuation from this UE
				double txPwr = uePhy->getTxPwr(dir) - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
				LtePhyBase *gNodeBPhy = check_and_cast<LtePhyBase*>(getBinder()->getMacFromMacNodeId(eNbId)->getParentModule()->getSubmodule("phy", 0));
//				double att = getAttenuationNR(ueId, UL, check_and_cast<NRRealisticChannelModel*>(uePhy->getChannelModel())->getMyPosition(),
//						check_and_cast<NRRealisticChannelModel*>(gNodeBPhy->getChannelModel())->getMyPosition(), false);
				double att = dynamic_cast<NRRealisticChannelModel*>(gNodeBPhy->getChannelModel())->getAttenuationNR(ueId, UL, check_and_cast<NRRealisticChannelModel*>(uePhy->getChannelModel())->getMyPosition(),
														check_and_cast<NRRealisticChannelModel*>(gNodeBPhy->getChannelModel())->getMyPosition(), false);
				(*interference)[i] += dBmToLinear(txPwr - att); //(dBm-dB)=dBm

				//EV << "\t band " << i << "/pwr[" << txPwr-att << "]-int[" << (*interference)[i] << "]" << endl;
			}
		}
	}

	// Debug Output
	//EV << NOW << " NRRealisticChannelModel::computeUplinkInterference - Final Band Interference Status: "<<endl;
	for (unsigned int i = 0; i < band_; i++) {
		//EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;
	}

	return true;
}

bool NRRealisticChannelModel::computeExtCellInterferenceNR(const MacNodeId &eNbId, const MacNodeId &nodeId, const Coord &coord, bool isCqi, std::vector<double> &interference,
		const Coord &enodebcoord) {

	//std::cout << "NRRealisticChannelModel::computeExtCellInterferenceNR start at " << simTime().dbl() << std::endl;

	//EV << "**** Ext Cell Interference **** " << endl;

// get external cell list
	ExtCellList list = binder_->getExtCellList();
	ExtCellList::iterator it = list.begin();

	Coord c;
	double d3d, d2d, // meters
			recvPwr, // watt
			recvPwrDBm, // dBm
			att, // dBm
			angularAtt; // dBm

//compute distance for each cell
	while (it != list.end()) {
		// get external cell position
		c = (*it)->getPosition();
		// computer distance between UE and the ext cell
		d3d = coord.distance(c);
		d2d = sqrt(pow(coord.x - c.x, 2) + pow(coord.y - c.y, 2));

		//EV << "\t distance between UE[" << coord.x << "," << coord.y << "] and extCell[" << c.x << "," << c.y << "] is -> " << d2d << "\t";

		// compute attenuation according to some path loss model
		att = computeExtCellPathLossNR(d3d, d2d, nodeId);

		//=============== angular ATTENUATION =================
		if ((*it)->getTxDirection() == OMNI) {
			angularAtt = 0;
		} else {
			// compute the angle between uePosition and reference axis, considering the eNb as center
			double ueAngle = computeAngle(c, coord);

			// compute the reception angle between ue and eNb
			double recvAngle = fabs((*it)->getTxAngle() - ueAngle);

			if (recvAngle > 180)
				recvAngle = 360 - recvAngle;

			// compute attenuation due to sectorial tx
			angularAtt = computeAngularAttenuation(recvAngle);
		}
		//=============== END angular ATTENUATION =================

		// TODO do we need to use (- cableLoss_ + antennaGainGnB_) in ext cells too?
		// compute and linearize received power
		recvPwrDBm = (*it)->getTxPower() - att - angularAtt - cableLoss_ + antennaGainGnB_ + antennaGainUe_;
		recvPwr = dBmToLinear(recvPwrDBm);

		// add interference in those bands where the ext cell is active
		for (unsigned int i = 0; i < band_; i++) {
			int occ;
			if (isCqi)  // check slot occupation for this TTI
			{
				occ = (*it)->getBandStatus(i);
			} else        // error computation. We need to check the slot occupation of the previous TTI
			{
				occ = (*it)->getPrevBandStatus(i);
			}

			// if the ext cell is active, add interference
			if (occ) {
				interference[i] += recvPwr;
			}
		}

		it++;
	}

	//std::cout << "NRRealisticChannelModel::computeExtCellInterferenceNR end at " << simTime().dbl() << std::endl;

	return true;
}

