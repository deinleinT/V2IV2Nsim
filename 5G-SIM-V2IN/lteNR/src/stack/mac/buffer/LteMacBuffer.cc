//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/LteMacBuffer.h"

LteMacBuffer::LteMacBuffer()
{
    queueOccupancy_ = 0;
    queueLength_ = 0;
    processed_ = 0;
    Queue_.clear();
}

LteMacBuffer::LteMacBuffer(const LteMacQueue& queue)
{
    operator=(queue);
}

LteMacBuffer::~LteMacBuffer()
{
    Queue_.clear();
}

LteMacBuffer& LteMacBuffer::operator=(const LteMacBuffer& queue)
{
    queueOccupancy_ = queue.queueOccupancy_;
    queueLength_ = queue.queueLength_;
    Queue_ = queue.Queue_;
    return *this;
}

LteMacBuffer* LteMacBuffer::dup() const
{
    return new LteMacBuffer(*this);
}

void LteMacBuffer::pushBack(PacketInfo pkt)
{

    //std::cout << "LteMacBuffer::pushBack start at " << simTime().dbl() << std::endl;

    queueLength_ = queueLength_ + 1;
    queueOccupancy_ += pkt.first;
    Queue_.push_back(pkt);

    //std::cout << "LteMacBuffer::pushBack end at " << simTime().dbl() << std::endl;
}

void LteMacBuffer::pushFront(PacketInfo pkt)
{

    //std::cout << "LteMacBuffer::pushFront start at " << simTime().dbl() << std::endl;

    queueLength_++;
    queueOccupancy_ += pkt.first;
    Queue_.push_front(pkt);

    //std::cout << "LteMacBuffer::pushBack end at " << simTime().dbl() << std::endl;
}

PacketInfo LteMacBuffer::popFront()
{

    //std::cout << "LteMacBuffer::popFront start at " << simTime().dbl() << std::endl;

    if (queueLength_ <= 0)
        throw cRuntimeError("Packet queue empty");

    PacketInfo pkt = Queue_.front();
    Queue_.pop_front();
    processed_++;
    queueLength_--;
    queueOccupancy_ -= pkt.first;

    //std::cout << "LteMacBuffer::popFront end at " << simTime().dbl() << std::endl;

    return pkt;
}

PacketInfo LteMacBuffer::popBack()
{

    //std::cout << "LteMacBuffer::popBack start at " << simTime().dbl() << std::endl;

    if (queueLength_ <= 0)
        throw cRuntimeError("Packet queue empty");

    PacketInfo pkt = Queue_.back();
    Queue_.pop_back();
    queueLength_--;
    queueOccupancy_ -= pkt.first;

    //std::cout << "LteMacBuffer::popBack end at " << simTime().dbl() << std::endl;

    return pkt;
}

PacketInfo& LteMacBuffer::front()
{
    //std::cout << "LteMacBuffer::front start at " << simTime().dbl() << std::endl;

    if (queueLength_ <= 0)
        throw cRuntimeError("Packet queue empty");

    //std::cout << "LteMacBuffer::front end at " << simTime().dbl() << std::endl;

    return Queue_.front();
}

PacketInfo LteMacBuffer::back() const
{
    //std::cout << "LteMacBuffer::back start at " << simTime().dbl() << std::endl;

    if (queueLength_ <= 0)
        throw cRuntimeError("Packet queue empty");

    //std::cout << "LteMacBuffer::back end at " << simTime().dbl() << std::endl;

    return Queue_.back();
}

void LteMacBuffer::setProcessed(unsigned int i)
{
    //std::cout << "LteMacBuffer::setProcessed  at " << simTime().dbl() << std::endl;

    processed_ = i;
}

simtime_t LteMacBuffer::getHolTimestamp() const
{

    //std::cout << "LteMacBuffer::getHolTimestamp start at " << simTime().dbl() << std::endl;

    if (queueLength_ <= 0)
        throw cRuntimeError("Packet queue empty");

    //std::cout << "LteMacBuffer::getHolTimestamp end at " << simTime().dbl() << std::endl;

    return Queue_.front().second;
}

unsigned int LteMacBuffer::getProcessed() const
{

    //std::cout << "LteMacBuffer::getProcessed  at " << simTime().dbl() << std::endl;

    return processed_;
}

const std::list<PacketInfo>*
LteMacBuffer::getPacketlist() const
{
    //std::cout << "LteMacBuffer::getPacketlist  at " << simTime().dbl() << std::endl;

    return &Queue_;
}

unsigned int LteMacBuffer::getQueueOccupancy() const
{
    //std::cout << "LteMacBuffer::getQueueOccupancy  at " << simTime().dbl() << std::endl;

    return queueOccupancy_;
}

int LteMacBuffer::getQueueLength() const
{
    //std::cout << "LteMacBuffer::getQueueLength  at " << simTime().dbl() << std::endl;

    return queueLength_;
}

bool LteMacBuffer::isEmpty() const
{
    //std::cout << "LteMacBuffer::isEmpty start at " << simTime().dbl() << std::endl;

    return (queueLength_ == 0);
}

std::ostream& operator << (std::ostream &stream, const LteMacBuffer* queue)
{
    stream << "LteMacBuffer-> Length: " << queue->getQueueLength() <<
        " Occupancy: " << queue->getQueueOccupancy() <<
        " HolTimestamp: " << queue->getHolTimestamp() <<
        " Processed: " << queue->getProcessed();
    return stream;
}

