/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPREnvironment.hpp"

#include <sempr/core/Core.hpp>
#include <sempr/storage/ODBStorage.hpp>
#include <sempr/processing/SopranoModule.hpp>
#include <sempr/processing/DBUpdateModule.hpp>
#include <sempr/processing/DebugModule.hpp>
#include <sempr/processing/ActiveObjectStore.hpp>
#include <sempr-rete-reasoning/ReteReasonerModule.hpp>
#include <sempr/processing/SpatialIndex.hpp>

#include <sempr/query/SPARQLQuery.hpp>
#include <sempr/query/ObjectQuery.hpp>
#include <sempr/query/SpatialIndexQuery.hpp>

#include <sempr/entity/spatial/SpatialObject.hpp>
#include <sempr/entity/spatial/reference/LocalCS.hpp>

#include <LocalCS_odb.h>
#include <SpatialObject_odb.h>
#include <RDFDocument_odb.h>
#include <RuleSet_odb.h>
#include <RDFVector_odb.h>

#include <sempr/core/IncrementalIDGeneration.hpp>
#include <sempr/core/IDGenUtil.hpp>

#include "Anchoring.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

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

    ReteReasonerModule::Ptr rete( new ReteReasonerModule() );
    SpatialIndex::Ptr spatial(new SpatialIndex() );

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
    sempr_->addModule(rete);
    sempr_->addModule(spatial);

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


bool SEMPREnvironment::addTriple(const ::sempr_rock::Triple &arg0)
{
    // get or create a global RDFVector to store the triples in
    const std::string rdfID = "RDF_Triple_Assertions";
    auto query = std::make_shared<ObjectQuery<RDFVector>>(
        [&rdfID](RDFVector::Ptr e) {
            return e->id() == rdfID;
        }
    );

    sempr_->answerQuery(query);

    RDFVector::Ptr rdf;
    if (query->results.empty())
    {
        // create a new one
        rdf.reset(new RDFVector(new PredefinedID(rdfID)));
        sempr_->addEntity(rdf);
    } else {
        rdf = query->results[0];
    }

    // add the triple
    Triple t;
    t.subject = arg0.subject_;
    t.predicate = arg0.predicate_;
    t.object = arg0.object_;

    bool valid = rdf->addTriple(t);
    rdf->changed();
    return valid;
}


bool SEMPREnvironment::removeTriple(::std::string const & entity, ::sempr_rock::Triple const & triple)
{
    // try to find the entity
    auto query = std::make_shared<ObjectQuery<RDFVector>>(
        [&entity](RDFVector::Ptr obj) {
            return (obj->id() == entity) || ((sempr::baseURI() + obj->id()) == entity);
        }
    );
    sempr_->answerQuery(query);

    if (query->results.empty())
    {
        std::cout << "Cannot remove triple. Entity: " << entity << " not found." << std::endl;
        return false; // entity not found
    }

    auto obj = query->results[0];

    Triple t;
    t.subject = triple.subject_;
    t.predicate = triple.predicate_;
    t.object = triple.object_;
    t.document = sempr::baseURI() + obj->id();

    bool actuallyRemovedSomething = obj->removeTriple(t);
    if (actuallyRemovedSomething) obj->changed();
    else
    {
        std::cout << "obj->removeTriple failed. Try to remove manually." << std::endl;
        // why? I guess that triple.document as stored in soprano is different from triple.document in the rdfentity!
        int index = -1;
        int count = 0;
        for (auto tmp : *obj) {
            if (tmp.subject == t.subject &&
                tmp.predicate == t.predicate &&
                tmp.object == t.object)
            {
                // for now, ignore the document. we have the correct entity,
                index = count;
                break;
            }
            count++;
        }

        if (index == -1) {
            std::cout << "... failed." << std::endl;
            return false;
        }

        obj->removeTripleAt(index);
        actuallyRemovedSomething = true;
        obj->changed();
    }

    return actuallyRemovedSomething;
}



::sempr_rock::SPARQLResult SEMPREnvironment::answerQuery(::std::string const & arg0)
{
    auto query = std::make_shared<SPARQLQuery>();
    query->query = arg0;
    sempr_->answerQuery(query);

    sempr_rock::SPARQLResult results;

    for (auto result : query->results)
    {
        sempr_rock::KVMap kvmap;
        //kvmap.fromMap(result);
        for (auto entry : result)
        {
            sempr_rock::KVPair pair;
            pair.key = entry.first;
            pair.value = entry.second.second;
//            pair.type = entry.second.first; // not implemented in sempr-rock/ObjectMessages.hpp yet
            kvmap.pairs.push_back(pair);
        }

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


::std::vector< ::std::string > SEMPREnvironment::getObjectsInCone(
    ::base::Pose const & direction,
    float length, float angle,
    ::std::string const & type)
{
    std::vector<std::string> objects;

    /**
        1. compute bounding box of the cone
        2. find all geometries inside the bbox (spatial index)
        3. find all geometries inside the cone
        4. find all spatial objects to the geometries
    */

    // compute a rough bounding box of the cone
    // first estimation: bounding box of the center line
    Eigen::Vector3d start, r, end;
    start = direction.position;
    r = direction.orientation * Eigen::Vector3d(1, 0, 0); // x axis of pose is direction vector
    end = start + length * r;

    std::cout << "line: " << start.matrix().transpose() << " to " << end.matrix().transpose() << std::endl;

    double min[3], max[3];
    min[0] = std::min(start[0], end[0]); max[0] = std::max(start[0], end[0]);
    min[1] = std::min(start[1], end[1]); max[1] = std::max(start[1], end[1]);
    min[2] = std::min(start[2], end[2]); max[2] = std::max(start[2], end[2]);


    // add radius of the cone at its end to all dimensions
    double radius_end = length * std::tan(angle * M_PI / 180.);
    min[0] -= radius_end; max[0] += radius_end;
    min[1] -= radius_end; max[1] += radius_end;
    min[2] -= radius_end; max[2] += radius_end;

    std::cout << "query min: (" << min[0] << " " << min[1] << " " << min[2] << ")" << std::endl;
    std::cout << "query max: (" << max[0] << " " << max[1] << " " << max[2] << ")" << std::endl;

    // get the root coordinate system, used to create the query
    auto objQuery = std::make_shared<ObjectQuery<LocalCS>>();
    sempr_->answerQuery(objQuery);

    if (objQuery->results.empty()) return objects; // error: no local cs found
    auto rootCS = objQuery->results[0]->getRoot();

    std::cout << "Got root: " << rootCS->id() << std::endl;

    // spatial index query to find object candidates
    Eigen::Vector3d lower(min[0], min[1], min[2]), upper(max[0], max[1], max[2]);
    SpatialIndexQuery::Ptr query = SpatialIndexQuery::intersectsBox(lower, upper, rootCS);
    sempr_->answerQuery(query);

    std::cout << "Number of candidates: " << query->results.size() << std::endl;

    // for every candidate **geometry**, check if it is actually in the cone
    Eigen::Vector3d p0, p1;
    std::set<entity::Geometry::Ptr> geometryInCone;
    for (auto geometry : query->results)
    {
        std::cout << "Candidate Geometry: " << geometry->id() << std::endl;
        p0 = geometry->getCS()->transformationToRoot() * Eigen::Vector3d(0, 0, 0);
        p1 = p0 - start;
        double l = p1.dot(r) / r.norm();
        std::cout << "l: 0 <= " << l << " <= " << length << "?" << std::endl;
        // first check: is the point above the line segment?
        if (0 <= l && l <= length)
        {
            // second check: is it inside the cone?
            std::cout << "p1.norm(): " << p1.norm() << std::endl;
            double dsq = p1.dot(p1) - l*l;
            double d = std::sqrt(dsq);
            std::cout << "d: " << d << std::endl;
            double dmax = std::tan(angle * M_PI / 180.) * l;
            std::cout << "distance: " << d << " < " << dmax << " ? " << std::endl;
            if (d <= dmax)
            {
                // okay, center of coordinate system of the object is within the cone.
                geometryInCone.insert(geometry);
            }
        }
    }

    // find the spatial objects that belong to the geometry
    auto soQuery = std::make_shared<ObjectQuery<SpatialObject>>(
        [&geometryInCone](SpatialObject::Ptr obj)
        {
            return geometryInCone.find(obj->geometry()) != geometryInCone.end();
        }
    );
    sempr_->answerQuery(soQuery);
    for (auto obj : soQuery->results)
    {
        objects.push_back(obj->id());
    }

    return objects;
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


    // old: rules for soprano
    // also, load the rules
    // auto rulequery = std::make_shared<ObjectQuery<RuleSet>>();
    // sempr_->answerQuery(rulequery);
    //
    // // make sure old rules are removed
    // for (auto rule : rulequery->results)
    // {
    //     sempr_->removeEntity(rule);
    // }
    //
    // // and reload them from the file
    // RuleSet::Ptr rules(new RuleSet());
    // sempr_->addEntity(rules);
    //
    // std::ifstream rulefile(_rules_file.get());
    // std::string line;
    // while (std::getline(rulefile, line))
    // {
    //     if (line.find("#") != 0) {
    //         rules->add(line);
    //         std::cout << "adding rule: " << line << std::endl;
    //     }
    // }
    // rules->changed();


    // new: rules for rete reasoner
    auto rete = sempr_->getModule<ReteReasonerModule>();
    std::ifstream rulefile(_rules_file.get());
    std::stringstream buffer;
    buffer << rulefile.rdbuf();

    rete->reset();
    rete->addRules(buffer.str());

    std::ofstream out1("rete_without_facts.dot");
    out1 << rete->reasoner().net().toDot();

    rete->rebuildKnowledge();

    // save network!
    std::ofstream out2("rete_with_facts.dot");
    out2 << rete->reasoner().net().toDot();



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
        // std::cout   << "detected: " << std::setw(21) << std::left
                    // << detections.detections[i].results[0].type
                    // << " --> "
                    // << (matches[i] ? matches[i]->id() : " nullptr ") << std::endl;


        SpatialObject::Ptr object;
        if (!matches[i])
        {
            auto obj = createSpatialObject(sempr_);
            updateSpatialObject(obj, detectionPairs[i]);
            std::cout << "created: " << obj->id() << '\n';
            object = obj;

        } else {
            updateSpatialObject(matches[i], detectionPairs[i]);
            std::cout << "updated: " << matches[i]->id() << std::endl;
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
