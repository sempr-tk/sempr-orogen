/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPREnvironment.hpp"

using namespace sempr;

SEMPREnvironment::SEMPREnvironment(std::string const& name)
    : SEMPREnvironmentBase(name)
{
}

SEMPREnvironment::SEMPREnvironment(std::string const& name, RTT::ExecutionEngine* engine)
    : SEMPREnvironmentBase(name, engine)
{
}

SEMPREnvironment::~SEMPREnvironment()
{
}

bool SEMPREnvironment::addObjectAssertion(::sempr_rock::ObjectAssertion const & arg0)
{
    std::cout << "TODO: addObjectAssertion" << '\n';
    return bool();
}

::sempr_rock::SPARQLResult SEMPREnvironment::answerQuery(::std::string const & arg0)
{
    std::cout << "TODO: answerQuery" << '\n';
    return ::sempr_rock::SPARQLResult();
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See SEMPREnvironment.hpp for more detailed
// documentation about them.

bool SEMPREnvironment::configureHook()
{
    if (! SEMPREnvironmentBase::configureHook())
        return false;
    return true;
}
bool SEMPREnvironment::startHook()
{
    if (! SEMPREnvironmentBase::startHook())
        return false;
    return true;
}
void SEMPREnvironment::updateHook()
{
    SEMPREnvironmentBase::updateHook();
}
void SEMPREnvironment::errorHook()
{
    SEMPREnvironmentBase::errorHook();
}
void SEMPREnvironment::stopHook()
{
    SEMPREnvironmentBase::stopHook();
}
void SEMPREnvironment::cleanupHook()
{
    SEMPREnvironmentBase::cleanupHook();
}
