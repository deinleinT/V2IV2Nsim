//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/conflict_graph/DistanceBasedConflictGraph.h"
#include "stack/phy/layer/LtePhyBase.h"

using namespace inet;
using namespace omnetpp;

/*!
 * \fn DistanceBasedConflictGraph()
 * \memberof DistanceBasedConflictGraph
 * \brief class constructor;
 */
DistanceBasedConflictGraph::DistanceBasedConflictGraph(LteMacEnbD2D* macEnb, bool reuseD2D, bool reuseD2DMulti, double dbmThresh)
        : ConflictGraph(macEnb,reuseD2D,reuseD2DMulti)
{
    d2dDbmThreshold_ = d2dMultiTxDbmThreshold_ = d2dMultiInterfDbmThreshold_ = dbmThresh;

    // non-initialized values
    d2dInterferenceRadius_ = -1.0;
    d2dMultiTransmissionRadius_ = -1.0;
    d2dMultiInterferenceRadius_ = -1.0;

    // get the reference to the PHY layer of the eNB
    phyEnb_ = check_and_cast<LtePhyBase*>(macEnb_->getParentModule()->getSubmodule("phy"));
}

void DistanceBasedConflictGraph::setThresholds(double d2dInterferenceRadius, double d2dMultiTransmissionRadius, double d2dMultiInterferenceRadius)
{
    d2dInterferenceRadius_ = d2dInterferenceRadius;
    d2dMultiTransmissionRadius_ = d2dMultiTransmissionRadius;
    d2dMultiInterferenceRadius_ = d2dMultiInterferenceRadius;
}

double DistanceBasedConflictGraph::getDbmFromDistance(double distance)
{
    bool los = false;    // TODO make it configurable
    double dbp = 0;
    double pLoss = 0;

    // get the reference to the channel model of the eNB
    LteChannelModel* channelModel = phyEnb_->getChannelModel();

    // obtain path loss in dBm
    if (channelModel == NULL)
        throw cRuntimeError("DistanceBasedConflictGraph::getDbmFromDistance - channel model is a null pointer. Abort");
    else
        pLoss = channelModel->computePathLoss(distance, dbp, los);

    return pLoss;
}

void DistanceBasedConflictGraph::findVertices(std::vector<CGVertex>& vertices)
{
    Binder* binder = getBinder();

    if (reuseD2D_)  // get point-to-point links
    {
        // get the list of point-to-point D2D connections

        typedef std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> > PeeringMap;
        PeeringMap* peeringMap = binder->getD2DPeeringMap();

        PeeringMap::iterator pit = peeringMap->begin(), pet = peeringMap->end();
        for (; pit != pet; ++pit)
        {
            std::map<MacNodeId, LteD2DMode>::iterator it = pit->second.begin(), et = pit->second.end();
            for (; it != et; ++it)
            {
                CGVertex v(pit->first, it->first);
                vertices.push_back(v);
            }
        }
    }

    if (reuseD2DMulti_)  // get point-to-multipoint transmitters
    {
        std::set<MacNodeId>& multicastTransmitterSet = binder->getD2DMulticastTransmitters();
        std::set<MacNodeId>::iterator it = multicastTransmitterSet.begin(), et = multicastTransmitterSet.end();
        for (; it != et; ++it)
        {
            CGVertex v(*it, 0);   // create a "fake" link
            vertices.push_back(v);
        }
    }
}

void DistanceBasedConflictGraph::findEdges(const std::vector<CGVertex>& vertices)
{
    std::vector<CGVertex>::const_iterator vit = vertices.begin(), vet = vertices.end();
    for (; vit != vet; ++vit)
    {
        CGVertex v1 = *vit;

        std::vector<CGVertex>::const_iterator it = vit;
        for (; it != vet; ++it)
        {
            CGVertex v2 = *it;

            if (v1 == v2)
            {
                // self conflict
                conflictGraph_[v1][v2] = true;
                conflictGraph_[v2][v1] = true;
                continue;
            }

            // Depending on the considered pair of vertices, we are in one of the following cases:
            //  -> P2P-P2P
            //  -> P2P-P2MP
            //  -> P2MP-P2P
            //  -> P2MP-P2MP
            //
            // Each case has a different condition to be verified. The condition can be based on either
            // distance or dBm thresholds, depending on whether distance thresholds are initialized or not


            if (!v1.isMulticast() && !v2.isMulticast())  // check P2P-P2P conflict
            {
                // obtain the position of v1's endpoints
                Coord v1SenderCoord = cellInfo_->getUePosition(v1.srcId);
                Coord v1DestCoord = cellInfo_->getUePosition(v1.dstId);
                // obtain the position of v2's endpoints
                Coord v2SenderCoord = cellInfo_->getUePosition(v2.srcId);
                Coord v2DestCoord = cellInfo_->getUePosition(v2.dstId);
                double distance1 = v1SenderCoord.distance(v2DestCoord);
                double distance2 = v2SenderCoord.distance(v1DestCoord);

                if (d2dInterferenceRadius_ > 0.0) // distance threshold initialized
                {
                    // compare distances

                    if (distance1 < d2dInterferenceRadius_ || distance2 < d2dInterferenceRadius_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }
                else
                {
                    // compare path-loss attenuations

                    if (getDbmFromDistance(distance1) < d2dDbmThreshold_ || getDbmFromDistance(distance2) < d2dDbmThreshold_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }
            }
            else if (!v1.isMulticast() && v2.isMulticast())   // check P2P-P2MP conflict
            {
                // obtain the position of v1's transmitter
                Coord v1SenderCoord = cellInfo_->getUePosition(v1.srcId);
                // obtain the position of v2 transmitter
                Coord v2SenderCoord = cellInfo_->getUePosition(v2.srcId);

                double distance = v1SenderCoord.distance(v2SenderCoord);

                if (d2dMultiTransmissionRadius_ > 0.0 && d2dInterferenceRadius_ > 0.0) // distance threshold initialized
                {
                    // compare distances

                    if (distance < d2dMultiTransmissionRadius_ + d2dInterferenceRadius_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }
                else
                {
                    // compare path-loss attenuations

                    if (getDbmFromDistance(distance) < d2dMultiTxDbmThreshold_ + d2dDbmThreshold_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }

            }
            else if (v1.isMulticast() && !v2.isMulticast())   // check P2MP-P2P conflict
            {
                // obtain the position of v1's transmitter
                Coord v1SenderCoord = cellInfo_->getUePosition(v1.srcId);
                // obtain the position of v2's receiveer
                Coord v2DestCoord = cellInfo_->getUePosition(v2.dstId);

                double distance = v1SenderCoord.distance(v2DestCoord);

                if (d2dMultiInterferenceRadius_ > 0.0) // distance threshold initialized
                {
                    // compare distances

                    if (distance < d2dMultiInterferenceRadius_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }
                else
                {
                    // compare path-loss attenuations

                    if (getDbmFromDistance(distance) < d2dMultiInterfDbmThreshold_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }

                }
            }
            else if (v1.isMulticast() && v2.isMulticast())    // check P2MP-P2MP conflict
            {
                // obtain the position of v1's transmitter
                Coord v1SenderCoord = cellInfo_->getUePosition(v1.srcId);
                // obtain the position of v2 transmitter
                Coord v2SenderCoord = cellInfo_->getUePosition(v2.srcId);

                double distance = v1SenderCoord.distance(v2SenderCoord);

                if (d2dMultiTransmissionRadius_ > 0.0 && d2dMultiInterferenceRadius_ > 0.0) // distance threshold initialized
                {
                    // compare distances

                    if (distance < d2dMultiTransmissionRadius_ + d2dMultiInterferenceRadius_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }
                else
                {
                    // compare path-loss attenuations

                    if (getDbmFromDistance(distance) < d2dMultiTxDbmThreshold_ + d2dMultiInterfDbmThreshold_)
                    {
                        // add edge to the conflict graph
                        conflictGraph_[v1][v2] = true;
                        conflictGraph_[v2][v1] = true;
                    }
                    else
                    {
                        conflictGraph_[v1][v2] = false;
                        conflictGraph_[v2][v1] = false;
                    }
                }
            }

        }  // end inner loop
    } // end outer loop
}

