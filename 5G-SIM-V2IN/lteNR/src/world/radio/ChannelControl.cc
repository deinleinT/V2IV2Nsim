/***************************************************************************
 * file:        ChannelControl.cc
 *
 * copyright:   (C) 2005 Andras Varga
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * part of:     framework implementation developed by tkn
 **************************************************************************/


#include "world/radio/ChannelControl.h"
#include "inet/common/INETMath.h"
#include <cassert>

#include "stack/phy/packet/AirFrame_m.h"

Define_Module(ChannelControl);


std::ostream& operator<<(std::ostream& os, const ChannelControl::RadioEntry& radio)
{
    os << radio.radioModule->getFullPath() << " (x=" << radio.pos.x << ",y=" << radio.pos.y << "), "
       << radio.neighbors.size() << " neighbor(s)";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ChannelControl::TransmissionList& tl)
{
    for (ChannelControl::TransmissionList::const_iterator it = tl.begin(); it != tl.end(); ++it)
        os << endl << *it;
    return os;
}

ChannelControl::ChannelControl()
{
}

ChannelControl::~ChannelControl()
{
    for (unsigned int i = 0; i < transmissions.size(); i++)
        for (TransmissionList::iterator it = transmissions[i].begin(); it != transmissions[i].end(); it++)
            delete *it;
}

/**
 * Calculates maxInterferenceDistance.
 *
 * @ref calcInterfDist
 */
void ChannelControl::initialize()
{
    coreDebug = hasPar("coreDebug") ? (bool) par("coreDebug") : false;

    //EV << "initializing ChannelControl\n";

    numChannels = par("numChannels");
    transmissions.resize(numChannels);

    lastOngoingTransmissionsUpdate = 0;

    maxInterferenceDistance = calcInterfDist();

    WATCH(maxInterferenceDistance);
    WATCH_LIST(radios);
    WATCH_VECTOR(transmissions);
}

/**
 * Calculation of the interference distance based on the transmitter
 * power, wavelength, pathloss coefficient and a threshold for the
 * minimal receive Power
 *
 * You may want to overwrite this function in order to do your own
 * interference calculation
 */
double ChannelControl::calcInterfDist()
{
    //std::cout << "ChannelAccess::calcInterfDist start at " << simTime().dbl() << std::endl;

    double interfDistance;

    //the carrier frequency used
    double carrierFrequency = par("carrierFrequency");
    //maximum transmission power possible
    double pMax = par("pMax");
    //signal attenuation threshold
    double sat = par("sat");
    //path loss coefficient
    double alpha = par("alpha");

    double waveLength = (SPEED_OF_LIGHT / carrierFrequency);
    //minimum power level to be able to physically receive a signal
    double minReceivePower = pow(10.0, sat / 10.0);

    interfDistance = pow(waveLength * waveLength * pMax /
                         (16.0 * M_PI * M_PI * minReceivePower), 1.0 / alpha);

    //EV << "max interference distance:" << interfDistance << endl;

    //std::cout << "ChannelAccess::calcInterfDist end at " << simTime().dbl() << std::endl;

    return interfDistance;
}

ChannelControl::RadioRef ChannelControl::registerRadio(cModule *radio, cGate *radioInGate)
{
    //std::cout << "ChannelAccess::registerRadio start at " << simTime().dbl() << std::endl;

    Enter_Method_Silent("registerRadio");

    RadioRef radioRef = lookupRadio(radio);

    if (radioRef)
        throw cRuntimeError("Radio %s already registered", radio->getFullPath().c_str());

    if (!radioInGate)
        radioInGate = radio->gate("radioIn");

    RadioEntry re;
    re.radioModule = radio;
    re.radioInGate = radioInGate->getPathStartGate();
    re.isNeighborListValid = false;
    re.channel = 0;  // for now
    re.isActive = true;
    radios.push_back(re);

    //std::cout << "ChannelAccess::registerRadio end at " << simTime().dbl() << std::endl;

    return &radios.back(); // last element
}

void ChannelControl::unregisterRadio(RadioRef r)
{
    Enter_Method_Silent("unregisterRadio");

    //std::cout << "ChannelAccess::unregisterRadio  at " << simTime().dbl() << std::endl;

    for (RadioList::iterator it = radios.begin(); it != radios.end(); it++)
    {
        if (it->radioModule == r->radioModule)
        {
            RadioRef radioToRemove = &*it;
            // erase radio from all registered radios' neighbor list
            for (RadioList::iterator i2 = radios.begin(); i2 != radios.end(); ++i2)
            {
                RadioRef otherRadio = &*i2;
                otherRadio->neighbors.erase(radioToRemove);
                otherRadio->isNeighborListValid = false;
                radioToRemove->isNeighborListValid = false;
            }

            // erase radio from registered radios
            radios.erase(it);
            return;
        }
    }



    error("unregisterRadio failed: no such radio");
}

ChannelControl::RadioRef ChannelControl::lookupRadio(cModule *radio)
{
    Enter_Method_Silent();

    //std::cout << "ChannelAccess::lookupRadio at " << simTime().dbl() << std::endl;

    for (RadioList::iterator it = radios.begin(); it != radios.end(); it++)
        if (it->radioModule == radio)
            return &(*it);
    return 0;
}

const ChannelControl::RadioRefVector& ChannelControl::getNeighbors(RadioRef h)
{
    Enter_Method_Silent("getNeighbors");

    //std::cout << "ChannelAccess::getNeighbors at " << simTime().dbl() << std::endl;

    if (!h->isNeighborListValid)
    {
        h->neighborList.clear();
        for (std::set<RadioRef,RadioEntry::Compare>::iterator it = h->neighbors.begin(); it != h->neighbors.end(); it++)
            h->neighborList.push_back(*it);
        h->isNeighborListValid = true;
    }
    return h->neighborList;
}

void ChannelControl::updateConnections(RadioRef h)
{
    //std::cout << "ChannelAccess::updateConnections start at " << simTime().dbl() << std::endl;

    inet::Coord& hpos = h->pos;
    double maxDistSquared = maxInterferenceDistance * maxInterferenceDistance;
    for (RadioList::iterator it = radios.begin(); it != radios.end(); ++it)
    {
        RadioEntry *hi = &(*it);
        if (hi == h)
            continue;

        // get the distance between the two radios.
        // (omitting the square root (calling sqrdist() instead of distance()) saves about 5% CPU)
        bool inRange = hpos.sqrdist(hi->pos) < maxDistSquared;

        if (inRange)
        {
            // nodes within communication range: connect
            if (h->neighbors.insert(hi).second == true)
            {
                hi->neighbors.insert(h);
                h->isNeighborListValid = hi->isNeighborListValid = false;
            }
        }
        else
        {
            // out of range: disconnect
            if (h->neighbors.erase(hi))
            {
                hi->neighbors.erase(h);
                h->isNeighborListValid = hi->isNeighborListValid = false;
            }
        }
    }
    //std::cout << "ChannelAccess::updateConnections end at " << simTime().dbl() << std::endl;
}

void ChannelControl::checkChannel(int channel)
{
    //std::cout << "ChannelAccess::checkChannel at " << simTime().dbl() << std::endl;

    if (channel >= numChannels || channel < 0)
        error("Invalid channel, must above 0 and below %d", numChannels);
}

void ChannelControl::setRadioPosition(RadioRef r, const inet::Coord& pos)
{
    Enter_Method_Silent("setRadioPosition");

    //std::cout << "ChannelAccess::setRadioPosition at " << simTime().dbl() << std::endl;

    r->pos = pos;
    updateConnections(r);
}

void ChannelControl::setRadioChannel(RadioRef r, int channel)
{
    Enter_Method_Silent("setRadioChannel");

    //std::cout << "ChannelAccess::setRadioChannel at " << simTime().dbl() << std::endl;

    checkChannel(channel);

    r->channel = channel;
}

const ChannelControl::TransmissionList& ChannelControl::getOngoingTransmissions(int channel)
{
    Enter_Method_Silent("getOngoingTransmissions");

    //std::cout << "ChannelAccess::getOngoingTransmissions at " << simTime().dbl() << std::endl;

    checkChannel(channel);
    purgeOngoingTransmissions();
    return transmissions[channel];
}

void ChannelControl::addOngoingTransmission(RadioRef h, AirFrame *frame)
{
    Enter_Method_Silent("addOngoingTransmission");

    //std::cout << "ChannelAccess::addOngoingTransmission start at " << simTime().dbl() << std::endl;

    // we only keep track of ongoing transmissions so that we can support
    // NICs switching channels -- so there's no point doing it if there's only
    // one channel
    if (numChannels == 1)
    {
        delete frame;
        return;
    }

    // purge old transmissions from time to time
    if (simTime() - lastOngoingTransmissionsUpdate > TRANSMISSION_PURGE_INTERVAL)
    {
        purgeOngoingTransmissions();
        lastOngoingTransmissionsUpdate = simTime();
    }

    // register ongoing transmission
    take(frame);
    frame->setTimestamp(); // store time of transmission start
    transmissions[frame->getChannelNumber()].push_back(frame);

    //std::cout << "ChannelAccess::addOngoingTransmission end at " << simTime().dbl() << std::endl;
}

void ChannelControl::purgeOngoingTransmissions()
{
    //std::cout << "ChannelAccess::purgeOngoingTransmissions start at " << simTime().dbl() << std::endl;

    for (int i = 0; i < numChannels; i++)
    {
        for (TransmissionList::iterator it = transmissions[i].begin(); it != transmissions[i].end();)
        {
            TransmissionList::iterator curr = it;
            AirFrame *frame = *it;
            it++;

            if (frame->getTimestamp() + frame->getDuration() + TRANSMISSION_PURGE_INTERVAL < simTime())
            {
                delete frame;
                transmissions[i].erase(curr);
            }
        }
    }

    //std::cout << "ChannelAccess::purgeOngoingTransmissions end at " << simTime().dbl() << std::endl;
}

void ChannelControl::sendToChannel(RadioRef srcRadio, AirFrame *airFrame)
{
    //std::cout << "ChannelAccess::sendToChannel start at " << simTime().dbl() << std::endl;

    // NOTE: no Enter_Method()! We pretend this method is part of ChannelAccess

    Enter_Method_Silent("sendToChannel");

    // loop through all radios in range
    const RadioRefVector& neighbors = getNeighbors(srcRadio);
    int n = neighbors.size();
    int channel = airFrame->getChannelNumber();
    for (int i=0; i<n; i++)
    {
        RadioRef r = neighbors[i];
        if (!r->isActive)
        {
            //EV << "skipping disabled radio interface \n";
            continue;
        }
        if (r->channel == channel)
        {
            //EV << "sending message to radio listening on the same channel\n";
            // account for propagation delay, based on distance in meters
            // Over 300m, dt=1us=10 bit times @ 10Mbps
            simtime_t delay = srcRadio->pos.distance(r->pos) / SPEED_OF_LIGHT;
            check_and_cast<cSimpleModule*>(srcRadio->radioModule)->sendDirect(airFrame->dup(), delay, airFrame->getDuration(), r->radioInGate);
        }
        else{
            //EV << "skipping radio listening on a different channel\n";
        }
    }

    // register transmission
    addOngoingTransmission(srcRadio, airFrame);

    //std::cout << "ChannelAccess::sendToChannel end at " << simTime().dbl() << std::endl;
}
