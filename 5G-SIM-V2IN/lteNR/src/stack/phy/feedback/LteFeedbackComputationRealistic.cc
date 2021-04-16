//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This file has been modified/enhanced for 5G-SIM-V2I/N.
// Date: 2020
// Author: Thomas Deinlein
//

#include <math.h>
#include "stack/phy/feedback/LteFeedbackComputationRealistic.h"
#include "corenetwork/blerCurves/PhyPisaData.h"
#include "corenetwork/binder/LteBinder.h"

LteFeedbackComputationRealistic::LteFeedbackComputationRealistic(double targetBler, std::map<MacNodeId, Lambda>* lambda,
    double lambdaMinTh, double lambdaMaxTh, double lambdaRatioTh, unsigned int numBands)
{
    targetBler_ = targetBler;
    lambda_ = lambda;
    numBands_ = numBands;
    lambdaMinTh_ = lambdaMinTh;
    lambdaMaxTh_ = lambdaMaxTh;
    lambdaRatioTh_ = lambdaRatioTh;
    phyPisaData_ = &(getBinder()->phyPisaData);
    blerNR = &(getBinder()->blerNR);


    baseMin_.resize(phyPisaData_->nMcs(), 1);
}

LteFeedbackComputationRealistic::~LteFeedbackComputationRealistic()
{
    // TODO Auto-generated destructor stub
}

void LteFeedbackComputationRealistic::generateBaseFeedback(int numBands, int numPreferredBands, LteFeedback& fb,
    FeedbackType fbType, int cw, RbAllocationType rbAllocationType, TxMode txmode, std::vector<double> snr)
{
    //std::cout << "LteFeedbackComputationRealistic::generateBaseFeedback start at " << simTime().dbl() << std::endl;

    int layer = 1;
    std::vector<CqiVector> cqiTmp2;
    CqiVector cqiTmp;
    if (txmode == OL_SPATIAL_MULTIPLEXING)
    {
        //if rank is 1 SMUX is not a valid choice as Tx Mode
        if (fb.getRankIndicator() < 2)
            return;
        layer = fb.getRankIndicator();
    }
    layer = cw < layer ? cw : layer;

    Cqi cqi;
    double mean = 0;
    if (fbType == WIDEBAND || rbAllocationType == TYPE2_DISTRIBUTED)
        mean = meanSnr(snr);
    if (fbType == WIDEBAND)
    {
        cqiTmp.resize(layer, 0);
        cqi = getCqi(txmode, mean);
        for (unsigned int i = 0; i < cqiTmp.size(); i++)
            cqiTmp[i] = cqi;
        fb.setWideBandCqi(cqiTmp);
    }
    else if (fbType == ALLBANDS)
    {
        if (rbAllocationType == TYPE2_LOCALIZED)
        {
            cqiTmp2.resize(layer);
            for (unsigned int i = 0; i < cqiTmp2.size(); i++)
            {
                cqiTmp2[i].resize(numBands, 0);
                for (int j = 0; j < numBands; j++)
                {
                    cqi = getCqi(txmode, snr[j]);
                    cqiTmp2[i][j] = cqi;
                }
                fb.setPerBandCqi(cqiTmp2[i], i);
            }
        }
        else if (rbAllocationType == TYPE2_DISTRIBUTED)
        {
            cqi = getCqi(txmode, mean);
            cqiTmp2.resize(layer);
            for (unsigned int i = 0; i < cqiTmp2.size(); i++)
            {
                cqiTmp2[i].resize(numBands, 0);
                for (int j = 0; j < numBands; j++)
                {
                    cqiTmp2[i][j] = cqi;
                }
                fb.setPerBandCqi(cqiTmp2[i], i);
            }
        }
    }
    else if (fbType == PREFERRED)
    {
        //TODO
    }

    //std::cout << "LteFeedbackComputationRealistic::generateBaseFeedback end at " << simTime().dbl() << std::endl;
}

unsigned int LteFeedbackComputationRealistic::computeRank(MacNodeId id)
{
    //std::cout << "LteFeedbackComputationRealistic::computeRank  at " << simTime().dbl() << std::endl;

    if (lambda_->at(id).lambdaMin < lambdaMinTh_)
        return 1;
    else
        return 2;
}

Cqi LteFeedbackComputationRealistic::getCqi(TxMode txmode, double snr)
{
    //std::cout << "LteFeedbackComputationRealistic::getCqi start at " << simTime().dbl() << std::endl;

	//from the paper Downlink SNR to CQI Mapping for Different Multiple Antenna Techniques in LTE --> Table III, (111, 3)
	//MohammadT.Kawser et al.
	//cqiFlag can be set in the omnetpp.ini
	if (cqiFlag) {
		if (snr < 4.05)
			return 1;
		else if (snr < 5.1)
			return 2;
		else if (snr < 8)
			return 3;
		else if (snr < 10)
			return 4;
		else if (snr < 11.8)
			return 5;
		else if (snr < 13.9)
			return 6;
		else if (snr < 16.1)
			return 7;
		else if (snr < 17.45)
			return 8;
		else if (snr < 19.5)
			return 9;
		else if (snr < 21.5)
			return 10;
		else if (snr < 23.1)
			return 11;
		else if (snr < 24.9)
			return 12;
		else if (snr < 27.0)
			return 13;
		else if (snr < 29.1)
			return 14;
		else
			return 15;

	} else {
		int newsnr = floor(snr + 0.5);

		if (getSimulation()->getSystemModule()->par("blerCurvesNR").boolValue()) {
			if (newsnr < blerNR->minSnr())
				return 0 + 1;
			if (newsnr > blerNR->maxSnr())
				return 15;
		} else {
			if (newsnr < 0)
				return 0 + 1;
			if (newsnr > phyPisaData_->maxSnr())
				return 15;
		}

		unsigned int txm = txModeToIndex[txmode];
		std::vector<double> min;
		int found = 0;
		double low = 1;

		min = baseMin_;
		for (int i = 0; i < phyPisaData_->nMcs(); i++) {
			double tmp;
			if (getSimulation()->getSystemModule()->par("blerCurvesNR").boolValue()) {
				tmp = blerNR->getBler(txm, i, newsnr);
			} else {
				tmp = phyPisaData_->getBler(txm, i, newsnr);
			}

			double diff = targetBler_ - tmp;
			min[i] = (diff > 0) ? diff : (diff * -1);
			if (low >= min[i]) {
				found = i;
				low = min[i];
			}
		}
		return found + 1;
	}

    //std::cout << "LteFeedbackComputationRealistic::getCqi end at " << simTime().dbl() << std::endl;

}

LteFeedbackDoubleVector LteFeedbackComputationRealistic::computeFeedback(FeedbackType fbType,
    RbAllocationType rbAllocationType, TxMode currentTxMode,
    std::map<Remote, int> antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
    std::vector<double> snr, MacNodeId id)
{
    //std::cout << "LteFeedbackComputationRealistic::computeFeedback start at " << simTime().dbl() << std::endl;

    //add enodeB to the number of antenna
    numRus++;
    // New Feedback
    LteFeedbackDoubleVector fbvv;
    fbvv.resize(numRus);
    //resize all the vectors
    for (int i = 0; i < numRus; i++)
        fbvv[i].resize(DL_NUM_TXMODE);
    //for each Remote
    for (int j = 0; j < numRus; j++)
    {
        LteFeedback fb;
        //for each txmode we generate a feedback exclude MU_MIMO because it is threated as siso
        for (int z = 0; z < DL_NUM_TXMODE - 1; z++)
        {
            //reset the feedback object
            fb.reset();
            fb.setTxMode((TxMode) z);
            unsigned int rank = 1;
            if (z == OL_SPATIAL_MULTIPLEXING)
                rank = computeRank(id);
            if ((z == OL_SPATIAL_MULTIPLEXING && rank > 1) || z == TRANSMIT_DIVERSITY || z == SINGLE_ANTENNA_PORT0)
            {
                //set the rank
                fb.setRankIndicator(rank);
                //set the remote
                fb.setAntenna((Remote) j);
                //set the pmi
                fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(rank, (double) 2)));
                //generate feedback for txmode z
                generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws[(Remote) j], rbAllocationType,
                    (TxMode) z, snr);
            }
            // add the feedback to the feedback structure
            LteFeedback fb2 = fb;
            if (z == SINGLE_ANTENNA_PORT0)
            {
                fb2.setTxMode(MULTI_USER);
                fbvv[j][MULTI_USER] = fb2;
            }
            fbvv[j][z] = fb;
        }
    }

    //std::cout << "LteFeedbackComputationRealistic::computeFeedback end at " << simTime().dbl() << std::endl;

    return fbvv;
}

LteFeedbackVector LteFeedbackComputationRealistic::computeFeedback(const Remote remote, FeedbackType fbType,
    RbAllocationType rbAllocationType, TxMode currentTxMode,
    int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
    std::vector<double> snr, MacNodeId id)
{
    //std::cout << "LteFeedbackComputationRealistic::computeFeedback start at " << simTime().dbl() << std::endl;

    // New Feedback
    LteFeedbackVector fbv;
    //resize
    fbv.resize(DL_NUM_TXMODE);
    LteFeedback fb;
    //for each txmode we generate a feedback
    for (int z = 0; z < DL_NUM_TXMODE; z++)
    {
        fb.reset();
        fb.setTxMode((TxMode) z);
        unsigned int rank = 1;
        if (z == OL_SPATIAL_MULTIPLEXING)
            rank = computeRank(id);
        if ((z == OL_SPATIAL_MULTIPLEXING && rank > 1) || z == TRANSMIT_DIVERSITY || z == SINGLE_ANTENNA_PORT0)
        {
            //set the rank
            fb.setRankIndicator(rank);
            //set the remote
            fb.setAntenna(remote);
            //set the pmi
            fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(rank, (double) 2)));
            //generate feedback for txmode z
            generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, (TxMode) z,
                snr);
        }
        // add the feedback to the feedback structure
        LteFeedback fb2 = fb;
        if (z == SINGLE_ANTENNA_PORT0)
        {
            fb2.setTxMode(MULTI_USER);
            fbv[MULTI_USER] = fb2;
        }
        fbv[z] = fb;
    }

    //std::cout << "LteFeedbackComputationRealistic::computeFeedback end at " << simTime().dbl() << std::endl;

    return fbv;
}

LteFeedback LteFeedbackComputationRealistic::computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
    RbAllocationType rbAllocationType,
    int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
    std::vector<double> snr, MacNodeId id)
{
    //std::cout << "LteFeedbackComputationRealistic::computeFeedback start at " << simTime().dbl() << std::endl;

    // New Feedback
    LteFeedback fb;
    unsigned int rank = 1;
    // set the rank for all the tx mode except for SISO, and Tx div
    if (txmode == OL_SPATIAL_MULTIPLEXING || txmode == CL_SPATIAL_MULTIPLEXING || txmode == MULTI_USER)
    {
        // Compute Rank Index
        rank = computeRank(id);
        if (rank < 2 && (txmode == OL_SPATIAL_MULTIPLEXING || CL_SPATIAL_MULTIPLEXING))
            return fb;
        //Set Rank
        fb.setRankIndicator(rank);
    }
    else
        fb.setRankIndicator(rank);
    //set the remote in feedback object
    fb.setAntenna(remote);
    fb.setTxMode(txmode);
    generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, txmode, snr);
    // set pmi only for cl smux and mumimo
    if (txmode == CL_SPATIAL_MULTIPLEXING || txmode == MULTI_USER)
    {
        //Set PMI
        fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(rank, (double) 2)));
    }

    //std::cout << "LteFeedbackComputationRealistic::computeFeedback end at " << simTime().dbl() << std::endl;

    return fb;
}

double LteFeedbackComputationRealistic::meanSnr(std::vector<double> snr)
{
    //std::cout << "LteFeedbackComputationRealistic::meanSnr start at " << simTime().dbl() << std::endl;

    double mean = 0;
    std::vector<double>::iterator it;
    for (it = snr.begin(); it != snr.end(); ++it)
        mean += *it;
    mean /= snr.size();

    //std::cout << "LteFeedbackComputationRealistic::meanSnr end at " << simTime().dbl() << std::endl;

    return mean;
}
