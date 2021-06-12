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

#include "stack/compManager/LteCompManagerBase.h"

using namespace omnetpp;
using namespace inet;

void LteCompManagerBase::initialize()
{
    // get the node id
    nodeId_ = getAncestorPar("macCellId");

    // get reference to the gates
    x2Manager_[IN_GATE] = gate("x2ManagerIn");
    x2Manager_[OUT_GATE] = gate("x2ManagerOut");

    // get reference to mac layer
    mac_ = check_and_cast<LteMacEnb*>(getMacByMacNodeId(nodeId_));

    // get the number of available bands
    numBands_ = mac_->getCellInfo()->getNumBands();

    const char* nodeType = par("compNodeType").stringValue();
    if (strcmp(nodeType,"COMP_CLIENT") == 0)
        nodeType_ = COMP_CLIENT;
    else if (strcmp(nodeType,"COMP_CLIENT_COORDINATOR") == 0)
        nodeType_ = COMP_CLIENT_COORDINATOR;
    else if (strcmp(nodeType,"COMP_COORDINATOR") == 0)
        nodeType_ = COMP_COORDINATOR;
    else
        throw cRuntimeError("LteCompManagerBase::initialize - Unrecognized node type %s", nodeType);

    // register to the X2 Manager
    auto pkt = new Packet ("X2CompMsg");
    auto initMsg = makeShared<X2CompMsg>();
    pkt->insertAtFront(initMsg);
    auto ctrlInfo = pkt->addTagIfAbsent<X2ControlInfoTag>();
    ctrlInfo->setInit(true);

    send(pkt, x2Manager_[OUT_GATE]);

    if (nodeType_ != COMP_CLIENT)
    {
        // get the list of slave nodes
        std::vector<int> clients = cStringTokenizer(par("clientList").stringValue()).asIntVector();
        clientList_.resize(clients.size());
        for (unsigned int i=0; i<clients.size(); i++)
            clientList_[i] = clients[i];

        // get coordination period
        coordinationPeriod_ = par("coordinationPeriod").doubleValue();
        if (coordinationPeriod_ < TTI)
            coordinationPeriod_ = TTI;

        /* Start coordinator tick */
        compCoordinatorTick_ = new cMessage("compCoordinatorTick_");
        compCoordinatorTick_->setSchedulingPriority(3);        // compCoordinatorTick_ after slaves' TTI TICK. TODO check if it must be done before or after..
        scheduleAt(NOW + coordinationPeriod_, compCoordinatorTick_);
    }

    if (nodeType_ != COMP_COORDINATOR)
    {
        // statistics
        compReservedBlocks_ = registerSignal("compReservedBlocks");

        coordinatorId_ = par("coordinatorId");

        /* Start TTI tick */
        compClientTick_ = new cMessage("compClientTick_");
        compClientTick_->setSchedulingPriority(2);        // compClientTick_ after MAC's TTI TICK. TODO check if it must be done before or after..
        scheduleAt(NOW + TTI, compClientTick_);
    }
}

void LteCompManagerBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (strcmp(msg->getName(),"compClientTick_") == 0)
        {
            runClientOperations();
            scheduleAt(NOW+TTI, msg);
        }
        else if (strcmp(msg->getName(),"compCoordinatorTick_") == 0)
        {
            runCoordinatorOperations();
            scheduleAt(NOW+coordinationPeriod_, msg);
        }
        else
            throw cRuntimeError("LteCompManagerBase::handleMessage - Unrecognized self message %s", msg->getName());
    }
    else
    {
        Packet* pkt = check_and_cast<Packet*>(msg);
        cGate* incoming = pkt->getArrivalGate();
        if (incoming == x2Manager_[IN_GATE])
        {
            // incoming data from X2 Manager
            EV << "LteCompManagerBase::handleMessage - Received message from X2 manager" << endl;
            handleX2Message(pkt);
        }
        else {
            throw cRuntimeError("LteCompManagerBase::handleMessage - Unexpected message received at %s", incoming->getName());
            delete msg;
        }

    }
}

void LteCompManagerBase::runClientOperations()
{
    EV << "LteCompManagerBase::runClientOperations - node " << nodeId_ << endl;
    provisionalSchedule();
    X2CompRequestIE* requestIe = buildClientRequest();
    sendClientRequest(requestIe);
}

void LteCompManagerBase::runCoordinatorOperations()
{
    EV << "LteCompManagerBase::runCoordinatorOperations - node " << nodeId_ << endl;
    doCoordination();

    // for each client, send the appropriate reply
    std::vector<X2NodeId>::iterator cit = clientList_.begin();
    for (; cit != clientList_.end(); ++cit)
    {
        X2NodeId clientId = *cit;
        X2CompReplyIE* replyIe = buildCoordinatorReply(clientId);
        sendCoordinatorReply(clientId, replyIe);
    }

    if (nodeType_ == COMP_CLIENT_COORDINATOR)
    {
        // local reply
        X2CompReplyIE* replyIe = buildCoordinatorReply(nodeId_);
        sendCoordinatorReply(nodeId_, replyIe);
    }
}

void LteCompManagerBase::handleX2Message(Packet* pkt)
{
    auto compMsg = pkt->removeAtFront<X2CompMsg>();
    X2NodeId sourceId = compMsg->getSourceId();

    if (nodeType_ == COMP_COORDINATOR || nodeType_ == COMP_CLIENT_COORDINATOR)
    {
        // this is a request from a client
        handleClientRequest(compMsg);
    }
    else
    {
        // this is a reply from coordinator
        if (sourceId != coordinatorId_)
            throw cRuntimeError("LteCompManagerBase::handleX2Message - Sender is not the coordinator");

        handleCoordinatorReply(compMsg);
    }
    delete pkt;
}

void LteCompManagerBase::sendClientRequest(X2CompRequestIE* requestIe)
{
    // build X2 Comp Msg
    auto compMsg = makeShared<X2CompMsg>();
    compMsg->setSourceId(nodeId_);
    compMsg->pushIe(requestIe);

    EV<<NOW<<" LteCompManagerBase::sendCompRequest - Send CoMP request (len: "<< compMsg->getByteLength()<< " B)" << endl;

    if (nodeType_ == COMP_CLIENT_COORDINATOR)
    {
        handleClientRequest(compMsg);
    }
    else
    {
        auto pkt = new Packet("X2CompMsg");

        // build control info
        auto ctrlInfo = pkt->addTagIfAbsent<X2ControlInfoTag>();
        ctrlInfo->setSourceId(nodeId_);
        DestinationIdList destList;
        destList.push_back(coordinatorId_);
        ctrlInfo->setDestIdList(destList);

        // add COMP message to packet
        pkt->insertAtFront(compMsg);

        // send to X2 Manager
        send(pkt,x2Manager_[OUT_GATE]);
    }
}

void LteCompManagerBase::sendCoordinatorReply(X2NodeId clientId, X2CompReplyIE* replyIe)
{
    // build X2 Comp Msg
    auto compMsg = makeShared<X2CompMsg>();
    compMsg->pushIe(replyIe);
    compMsg->setSourceId(nodeId_);

    if (clientId == nodeId_)
    {
        if (nodeType_ != COMP_CLIENT_COORDINATOR)
            throw cRuntimeError("LteCompManagerBase::sendCoordinatorReply - Node %d cannot sends reply to itself, since it is not the coordinator", clientId);

        compMsg->setSourceId(nodeId_);
        handleCoordinatorReply(compMsg);
    }
    else
    {
        // create a new packet to be able to send info to X2 manager
        auto pkt = new Packet("X2CompMsg");

        // build control info and add it to the packet
        auto ctrlInfo = pkt->addTagIfAbsent<X2ControlInfoTag>();
        ctrlInfo->setSourceId(nodeId_);
        DestinationIdList destList;
        destList.push_back(clientId);
        ctrlInfo->setDestIdList(destList);

        // send packet to X2 Manager
        pkt->insertAtFront(compMsg);
        send(pkt,x2Manager_[OUT_GATE]);
    }
}


void LteCompManagerBase::setUsableBands(UsableBands& usableBands)
{
    usableBands_ = usableBands;
    mac_->getAmc()->setPilotUsableBands(nodeId_, usableBands_);
}

