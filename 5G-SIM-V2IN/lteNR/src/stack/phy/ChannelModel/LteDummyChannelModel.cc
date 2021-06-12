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

#include "LteDummyChannelModel.h"

using namespace omnetpp;

Define_Module(LteDummyChannelModel);

void LteDummyChannelModel::initialize(int stage)
{
    LteChannelModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        per_=0.1;
        harqReduction_=0.3;
    }
}


std::vector<double> LteDummyChannelModel::getSINR(LteAirFrame *frame, UserControlInfo* lteInfo, bool flag)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   // fake SINR is needed by das (to decide which antenna set are used by the terminal)
   // and handhover function to decide if the terminal should trigger the hanhover
   return tmp;
}

std::vector<double> LteDummyChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   return tmp;
}

std::vector<double> LteDummyChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   // fake SINR is needed by das (to decide which antenna set are used by the terminal)
   // and handhover function to decide if the terminal should trigger the hanhover
   return tmp;
}

std::vector<double> LteDummyChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId,const std::vector<double>& rsrpVector)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   // fake SINR is needed by das (to decide which antenna set are used by the terminal)
   // and handhover function to decide if the terminal should trigger the hanhover
   return tmp;
}

std::vector<double> LteDummyChannelModel::getSIR(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   // fake SIR is needed by das (to decide which antenna set are used by the terminal)
   // and handhover function to decide if the terminal should trigger the hanhover
   return tmp;
}

bool LteDummyChannelModel::isCorrupted(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   // Number of RTX
   unsigned char nTx = lteInfo->getTxNumber();
   //Consistency check
   if (nTx == 0)
       throw cRuntimeError("Number of tx should not be 0");

   // compute packet error rate according to number of retransmission
   // and the harq reduction parameter
   double totalPer = per_ * pow(harqReduction_, nTx - 1);
   //Throw random variable
   double er = uniform(0.0, 1.0);

   if (er <= totalPer)
   {
       EV << "This is NOT your lucky day (" << er << " < " << totalPer
          << ") -> do not receive." << endl;
       // Signal too weak, we can't receive it
       return false;
   }
       // Signal is strong enough, receive this Signal
   EV << "This is your lucky day (" << er << " > " << totalPer
      << ") -> Receive AirFrame." << endl;
   return true;
}

bool LteDummyChannelModel::isError_D2D(LteAirFrame *frame, UserControlInfo* lteInfo,const std::vector<double>& rsrpVector)
{
   // Number of RTX
   unsigned char nTx = lteInfo->getTxNumber();
   //Consistency check
   if (nTx == 0)
       throw cRuntimeError("Number of tx should not be 0");

   // compute packet error rate according to number of retransmission
   // and the harq reduction parameter
   double totalPer = per_ * pow(harqReduction_, nTx - 1);
   //Throw random variable
   double er = uniform(0.0, 1.0);

   if (er <= totalPer)
   {
       EV << "This is NOT your lucky day (" << er << " < " << totalPer
          << ") -> do not receive." << endl;
       // Signal too weak, we can't receive it
       return false;
   }
       // Signal is strong enough, receive this Signal
   EV << "This is your lucky day (" << er << " > " << totalPer
      << ") -> Receive AirFrame." << endl;
   return true;
}
