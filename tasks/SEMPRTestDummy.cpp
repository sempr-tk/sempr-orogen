/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPRTestDummy.hpp"

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
