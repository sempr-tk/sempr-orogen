/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPRTestDummy.hpp"

#include <map>
#include <string>
#include <iostream>
#include <rtt/transports/corba/TaskContextProxy.hpp>


using namespace sempr;

SEMPRTestDummy::SEMPRTestDummy(std::string const& name)
    : SEMPRTestDummyBase(name)
{
}

SEMPRTestDummy::SEMPRTestDummy(std::string const& name, RTT::ExecutionEngine* engine)
    : SEMPRTestDummyBase(name, engine)
{
}

SEMPRTestDummy::~SEMPRTestDummy()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See SEMPRTestDummy.hpp for more detailed
// documentation about them.

bool SEMPRTestDummy::configureHook()
{
    if (! SEMPRTestDummyBase::configureHook())
        return false;

    // try to get hold of the sempr-environment task context
    //-----------------------------------------------------------------------------------
    // std::cout << "(via this->getPeer)" << '\n';
    // sempr_ = this->getPeer(_sempr_task_name.get());
    //-----------------------------------------------------------------------------------
    sempr_ = RTT::corba::TaskContextProxy::Create(_sempr_task_name.get(), false);
    //-----------------------------------------------------------------------------------
    if (!sempr_) {
        std::cout << "Task context '" << _sempr_task_name.get() << "' not found." << '\n';
        return false;
    }

    addObjectAssertion_ = sempr_->getOperation("addObjectAssertion");
    if (!addObjectAssertion_.ready()) {
        std::cout << "Operation 'addObjectAssertion' not found." << '\n';
        return false;
    }

    answerQuery_ = sempr_->getOperation("answerQuery");
    if (!answerQuery_.ready()) {
        std::cout << "Operation 'answerQuery' not found." << '\n';
        return false;
    }

    return true;
}


bool SEMPRTestDummy::startHook()
{
    if (! SEMPRTestDummyBase::startHook())
        return false;
    return true;
}
void SEMPRTestDummy::updateHook()
{
    SEMPRTestDummyBase::updateHook();

    // just call the answerQuery_ operation. string is an easy parameter, and we'll just print it on the remote end.
    auto result = answerQuery_("SELECT * WHERE { ?s ?p ?o .}");

    // oh, and print the result here:
    for (auto kv : result.results)
    {
        std::map<std::string, std::string> map;
        kv.toMap(map);

        for (auto entry : map)
        {
            std::cout << entry.first << ":" << entry.second << " | ";
        }
        std::cout << std::endl;
    }
}
void SEMPRTestDummy::errorHook()
{
    SEMPRTestDummyBase::errorHook();
}
void SEMPRTestDummy::stopHook()
{
    SEMPRTestDummyBase::stopHook();
}
void SEMPRTestDummy::cleanupHook()
{
    SEMPRTestDummyBase::cleanupHook();
}
