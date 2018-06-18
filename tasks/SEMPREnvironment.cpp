/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPREnvironment.hpp"

#include <sempr/core/Core.hpp>
#include <sempr/storage/ODBStorage.hpp>
#include <sempr/processing/SopranoModule.hpp>
#include <sempr/processing/DBUpdateModule.hpp>
#include <sempr/processing/DebugModule.hpp>
#include <sempr/processing/ActiveObjectStore.hpp>

#include <sempr/query/SPARQLQuery.hpp>
#include <sempr/query/ObjectQuery.hpp>

#include <sempr/entity/spatial/SpatialObject.hpp>
#include <sempr/entity/spatial/LocalCS.hpp>

#include <LocalCS_odb.h>
#include <SpatialObject_odb.h>
#include <RDFDocument_odb.h>
#include <RuleSet_odb.h>

#include <sempr/core/IncrementalIDGeneration.hpp>
#include <sempr/core/IDGenUtil.hpp>

#include "Anchoring.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>

using namespace sempr;
using namespace sempr::core;
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

    DebugModule::Ptr debug( new DebugModule() );
    DBUpdateModule::Ptr updater( new DBUpdateModule(storage) );
    ActiveObjectStore::Ptr active( new ActiveObjectStore() );
    SopranoModule::Ptr semantic( new SopranoModule() );

    sempr::core::IDGenerator::getInstance().setStrategy(
        std::unique_ptr<sempr::core::IncrementalIDGeneration>(
            new sempr::core::IncrementalIDGeneration( storage )
        )
    );

    sempr_ = new sempr::core::Core();
    // sempr_->addModule(debug);
    sempr_->addModule(updater);
    sempr_->addModule(active);
    sempr_->addModule(semantic);

    // load everything
    std::vector<Entity::Ptr> everything;
    storage->loadAll(everything);
    for (auto e : everything) {
        std::cout << "loaded " << e->id() << std::endl;
        e->loaded();
    }

    // loading the RDFDocument is considered a real configuration, so it is done in the
    // configureHook.
}


// SEMPREnvironment::~SEMPREnvironment()
// {
//
// }

bool SEMPREnvironment::addObjectAssertion(::sempr_rock::ObjectAssertion const & arg0)
{
    // get the object:
    auto query = std::make_shared<ObjectQuery<SpatialObject>>(
        [&arg0](SpatialObject::Ptr obj) {
            std::cout << "check object: " << obj->id() << std::endl;
            return (obj->id() == arg0.objectId) || ("sempr:" + obj->id() == arg0.objectId);
        }
    );
    sempr_->answerQuery(query);

    if(query->results.empty())
    {
        std::cout << "Couldn't find object with id: " << arg0.objectId << std::endl;
        return false; // object not found.
    }

    // add the key/value pair
    auto obj = query->results[0];
    std::string base = arg0.baseURI;
    if (base.empty()) base = sempr::baseURI();

    // save as RDFResource to *not* change e.g. '<whatever>' to '"<whatever>"^^xsd:string'
    (*obj->properties())(arg0.key, base) = RDFResource(arg0.value);
    obj->properties()->changed();

    return true;
}


void SEMPREnvironment::addTriple(const ::sempr_rock::Triple &arg0)
{
    // get or create a global RDFEntity to store the triples in
    const std::string rdfID = "RDF_Triple_Assertions";
    auto query = std::make_shared<ObjectQuery<RDFEntity>>(
        [&rdfID](RDFEntity::Ptr e) {
            return e->id() == rdfID;
        }
    );

    sempr_->answerQuery(query);

    RDFEntity::Ptr rdf;
    if (query->results.empty())
    {
        // create a new one
        rdf.reset(new RDFEntity(new PredefinedID(rdfID)));
        sempr_->addEntity(rdf);
    } else {
        rdf = query->results[0];
    }

    // add the triple
    Triple t;
    t.subject = arg0.subject_;
    t.predicate = arg0.predicate_;
    t.object = arg0.object_;

    rdf->addTriple(t);
    rdf->changed();
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


::base::Pose SEMPREnvironment::getObjectPose(::std::string const & arg0)
{
    // get the object
    auto query = std::make_shared<ObjectQuery<SpatialObject>>(
        [&arg0](SpatialObject::Ptr obj) {
            return (obj->id() == arg0) || ((sempr::baseURI() + obj->id()) == arg0);
        }
    );
    sempr_->answerQuery(query);

    // object not found? ... uhm.
    if (query->results.empty())
    {
        return base::Pose(); // what is this? identity? invalid?
    }

    // get the pose
    auto tf = query->results[0]->geometry()->getCS()->transformationToRoot();
    base::Pose pose;
    pose.fromTransform(tf);

    return pose;
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See SEMPREnvironment.hpp for more detailed
// documentation about them.

bool SEMPREnvironment::configureHook()
{
    if (! SEMPREnvironmentBase::configureHook())
        return false;


    // get the RDFDocument that contains the basic model as loaded from a file
    // (assumption: only one instance of this class present)
    auto docquery = std::make_shared<ObjectQuery<RDFDocument>>();
    sempr_->answerQuery(docquery);

    // delete it!
    for (auto doc : docquery->results)
    {
        sempr_->removeEntity(doc);
    }

    // create a new one, if a owl file was given
    std::string file = _rdf_file.get();
    if (!file.empty())
    {
        auto doc = RDFDocument::FromFile(file);
        sempr_->addEntity(doc);
    }



    // also, load the rules
    // but the ruleset is more or less fixed, and needs to be only loaded/created once.
    auto rulequery = std::make_shared<ObjectQuery<RuleSet>>();
    sempr_->answerQuery(rulequery);

    // make sure old rules are removed
    for (auto rule : rulequery->results)
    {
        sempr_->removeEntity(rule);
    }

    // and reload them from the file // TODO: property for filename?
    RuleSet::Ptr rules(new RuleSet());
    sempr_->addEntity(rules);

    std::ifstream rulefile("../resources/owl.rules");
    std::string line;
    while (std::getline(rulefile, line))
    {
        if (line.find("#") != 0) {
            rules->add(line);
            std::cout << "adding rule: " << line << std::endl;
        }
    }
    rules->changed();

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
        detectionPairs.push_back(Detection("http://trans.fit/" + d.results[0].type, d.results[0].pose.pose));
    }

    std::vector<entity::SpatialObject::Ptr> matches;
    sempr::getMatchingObjects(sempr_, detectionPairs, matches);

    for (int i = 0; i < detections.detections.size(); i++)
    {
        std::cout   << "detected: " << std::setw(21) << std::left
                    << detections.detections[i].results[0].type
                    << " --> "
                    << (matches[i] ? matches[i]->id() : " nullptr ") << std::endl;


        SpatialObject::Ptr object;
        if (!matches[i])
        {
            auto obj = createSpatialObject(sempr_);
            updateSpatialObject(obj, detectionPairs[i]);
            std::cout << "created: " << obj->id() << '\n';
            object = obj;

        } else {
            updateSpatialObject(matches[i], detectionPairs[i]);
            object = matches[i];
        }

        // when using the fake object detection we also get the id of the object
        (*object->properties())("simulationID", "http://trans.fit/") = (int) detections.detections[i].results[0].id;
        object->properties()->changed();

    }

}


void SEMPREnvironment::errorHook()
{
    SEMPREnvironmentBase::errorHook();
}
void SEMPREnvironment::stopHook()
{
    SEMPREnvironmentBase::stopHook();

    // persist stuff every time the component is stopped.
    sempr_->getModule<DBUpdateModule>()->updateDatabase();
}
void SEMPREnvironment::cleanupHook()
{
    SEMPREnvironmentBase::cleanupHook();
}
