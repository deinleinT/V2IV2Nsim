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

#include "common/timer/TTimer.h"

using namespace inet;

void TTimer::start(simtime_t t)
{
    if (busy_)
        return;
    intr_ = new TTimerMsg("timer");
    intr_->setType(TTSIMPLE);
    intr_->setTimerId(timerId_);
    module_->scheduleAt(t + NOW, intr_);
    busy_ = true;
    start_ = NOW;
    expire_ = NOW + t;
}

void TTimer::stop()
{
    if (busy_)
    {
        module_->cancelAndDelete(intr_);
    }
    busy_ = false;
}

void TTimer::handle()
{
    busy_ = false;
}

void TMultiTimer::add(const simtime_t time, const unsigned int event)
{
    simtime_t remaining = 1;

    // Create an iterator to the multimap element.
    iterator_d it;

    // retrieve the event expire time
    if (busy_)
    {
        it = directList_.begin();
        remaining = (it->first - NOW);
    }

    Event_it rIt;
    // add the event to the priority queue, along with its argument
    // We use the enhanced version of insert. A suggestion to the
    // position is given (i.e. the last element).
    rIt = directList_.insert(directList_.end(),
        std::pair<simtime_t, unsigned int>((NOW + time), event));

    // add the information to the reverse List
    // If the element already exists abort
    if (reverseList_.find(event) != reverseList_.end())
        throw cRuntimeError("TMultiTimer::add(): element %d already exists", event);

    // Add the event to the reverse list
    reverseList_.insert(
        std::pair<const unsigned int, const Event_it>(event, rIt));

    // if this event finishes earlier than the earliest scheduled event, if any
    // then we have to reschedule the event
    if (!busy_ || time < remaining)
    {
        if (busy_)
            module_->cancelAndDelete(intr_);
        intr_ = new TMultiTimerMsg("timer");
        intr_->setType(TTMULTI);
        intr_->setTimerId(timerId_);
        intr_->setEvent(event);
        module_->scheduleAt(time + NOW, intr_);
    }
    // Set the timer as busy
    busy_ = true;
}

bool TMultiTimer::busy(unsigned int event) const
{
    return ((reverseList_.find(event) != reverseList_.end()) ? true : false);
}

void TMultiTimer::handle(unsigned int event)
{
    iterator_d direct;

    // retrieve the earliest event to dispatch

    if (directList_.begin() == directList_.end())
        throw cRuntimeError("TMultiTimer::handle(): The list is empty");

    // Note that, the first element in the map is the element with the
    // lowest key value
    direct = directList_.begin();

    // Retrieve the Event ID
    unsigned int element = direct->second;

    // Remove the event from the reverse list, element will contain the
    // reverse list key.
    reverseList_.erase(element);

    // Remove the element from the direct list (i.e. pop front)
    directList_.erase(direct);

    // reschedule the timer to manage the earliest event, if any
    if (directList_.empty())
        busy_ = false;
    else
    {
        direct = directList_.begin();

        simtime_t time = direct->first;
        unsigned int event = direct->second;

        intr_ = new TMultiTimerMsg("timer");
        intr_->setTimerId(timerId_);
        intr_->setType(TTMULTI);
        intr_->setEvent(event);
        module_->scheduleAt(time, intr_);
    }
}

void TMultiTimer::remove(const unsigned int event)
{
    // Iterator to Reverse List
    iterator_r rIt;
    // Search for the element to be removed.
    rIt = reverseList_.find(event);

    if (rIt == reverseList_.end())
        throw cRuntimeError("TMultiTimer::remove(): element %d not found", event);

    // Iterator to Direct List
    iterator_d dIt;

    // Save the Event Id. It will be used to remove an element in the
    // direct List.
    dIt = (rIt->second);

    // Check if the event to be removed is the next in the event scheduler
    if (dIt == directList_.begin())
    {
        // Remove the Event from the direct list
        directList_.erase(dIt);
        // Remove the event from the connected simpleModule .
        module_->cancelAndDelete(intr_);

        if (directList_.empty())
            busy_ = false;
        else
        {
            simtime_t time = directList_.begin()->first;
            unsigned int event = directList_.begin()->second;
            intr_ = new TMultiTimerMsg("timer");
            intr_->setTimerId(timerId_);
            intr_->setType(TTMULTI);
            intr_->setEvent(event);
            module_->scheduleAt(time, intr_);
        }
    }
    else
    {
        // Remove the Event from the direct list
        directList_.erase(dIt);
    }
    // Remove the Event from the reverse list
    reverseList_.erase(rIt);
}
