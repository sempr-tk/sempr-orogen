/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPREnvironment.hpp"

#include <sempr/core/Core.hpp>
#include <sempr/storage/ODBStorage.hpp>
#include <sempr/processing/SopranoModule.hpp>
#include <sempr/processing/DBUpdateModule.hpp>
#include <sempr/processing/ActiveObjectStore.hpp>

#include <sempr/query/SPARQLQuery.hpp>
#include <sempr/query/ObjectQuery.hpp>

#include <sempr/entity/spatial/SpatialObject.hpp>
#include <sempr/entity/spatial/LocalCS.hpp>

#include <LocalCS_odb.h>
#include <SpatialObject_odb.h>

#include <sempr/core/IncrementalIDGeneration.hpp>
#include <sempr/core/IDGenUtil.hpp>

#include "Anchoring.hpp"

#include <iostream>
#include <iomanip>

using namespace sempr;
using namespace sempr::storage;
using namespace sempr::processing;
using namespace sempr::entity;
using namespace sempr::query;

SEMPREnvironment::SEMPREnvironment(std::string const& name)
    : SEMPREnvironmentBase(name)
{
    initializeSEMPR();
}

SEMPREnvironment::SEMPREnvironment(std::string const& name, RTT::ExecutionEngine* engine)
    : SEMPREnvironmentBase(name, engine)
{
    initializeSEMPR();
}


void SEMPREnvironment::initializeSEMPR()
{
    ODBStorage::Ptr storage( new ODBStorage() );

    DBUpdateModule::Ptr updater( new DBUpdateModule(storage) );
    ActiveObjectStore::Ptr active( new ActiveObjectStore() );
    SopranoModule::Ptr semantic( new SopranoModule() );

    sempr::core::IDGenerator::getInstance().setStrategy(
        std::unique_ptr<sempr::core::IncrementalIDGeneration>(
            new sempr::core::IncrementalIDGeneration( storage )
        )
    );

    sempr_ = new sempr::core::Core( storage );
    sempr_->addModule(updater);
    sempr_->addModule(active);
    sempr_->addModule(semantic);

}


SEMPREnvironment::~SEMPREnvironment()
{

}

bool SEMPREnvironment::addObjectAssertion(::sempr_rock::ObjectAssertion const & arg0)
{
    std::cout << "TODO: addObjectAssertion" << '\n'; // TODO
    return false;
}

::sempr_rock::SPARQLResult SEMPREnvironment::answerQuery(::std::string const & arg0)
{
    std::cout << "answerQuery:" << '\n';
    std::cout << arg0 << '\n';

    auto query = std::make_shared<SPARQLQuery>();
    query->query = arg0;
    sempr_->answerQuery(query);

    sempr_rock::SPARQLResult results;

    for (auto result : query->results)
    {
        sempr_rock::KVMap kvmap;
        kvmap.fromMap(result);
        results.results.push_back(kvmap);
    }

    return results;
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

    // TODO: get object detections msgs
    // TODO: anchoring

    // update only on input of detectionArray
    mars::Detection3DArray detections;
    auto status = _detectionArray.read(detections);
    if (status != RTT::FlowStatus::NewData) return;

    std::cout << "processing Detection3DArray" << std::endl;

    // compute matches
    std::vector<sempr::Detection> detectionPairs;
    for (auto d : detections.detections)
    {
        detectionPairs.push_back(Detection(d.results[0].type, d.results[0].pose.pose));
    }

    std::vector<entity::SpatialObject::Ptr> matches;
    sempr::getMatchingObjects(sempr_, detectionPairs, matches);

    for (int i = 0; i < detections.detections.size(); i++)
    {
        std::cout   << "detected: " << std::setw(21) << std::left
                    << detections.detections[i].results[0].type
                    << " --> "
                    << (matches[i] ? matches[i]->id() : " nullptr ") << std::endl;

        if (!matches[i])
        {
            auto obj = createSpatialObject(sempr_);
            updateSpatialObject(obj, detectionPairs[i]);
            std::cout << "created: " << obj->id() << '\n';
        } else {
            updateSpatialObject(matches[i], detectionPairs[i]);
        }
    }

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
