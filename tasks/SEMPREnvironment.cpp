/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SEMPREnvironment.hpp"

#include <sempr/core/Core.hpp>
#include <sempr/storage/ODBStorage.hpp>
#include <sempr/processing/SopranoModule.hpp>
#include <sempr/processing/DBUpdateModule.hpp>
#include <sempr/processing/DebugModule.hpp>
#include <sempr/processing/ActiveObjectStore.hpp>
#include <sempr/processing/CallbackModule.hpp>
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

#include <sempr-anchoring/Detection3D.hpp>

#include "Anchoring.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

#include <pcl/conversions.h>
#include <pcl/point_representation.h>

#include <hybrit_rock_msgs/EnvUpdate.hpp>

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
    SopranoModule::Ptr semantic( new SopranoModule(false) );

    ReteReasonerModule::Ptr rete( new ReteReasonerModule(1000000) );
    SpatialIndex::Ptr spatial(new SpatialIndex() );

    // create a sempr_rock::SpatialObject message everytime something
    // changes
    auto collectObjectUpdates = processing::CreateCallbackModule(
        [this](core::EntityEvent<entity::SpatialObject>::Ptr event)
        {
            std::cout << "This is the callback speaking! We got a changed SpatialObject!" << std::endl;

            typedef core::EntityEvent<entity::SpatialObject> event_t;
            sempr_rock::SpatialObject msg;
            msg.id = event->getEntity()->id();
            auto tf = event->getEntity()->geometry()->getCS()->transformationToRoot();
            msg.position = tf.translation();
            msg.orientation = tf.rotation();
            msg.type = event->getEntity()->type();
            msg.mod = (event->what() == event_t::REMOVED ? 
                       sempr_rock::Modification::REMOVE : sempr_rock::Modification::ADD);

            // remember to publish this some time as a bulk message...
            this->pendingVizUpdates_.push_back(msg);

            // also, publish this now, directly, single message.
            this->_objectUpdates.write(msg);
        }
    );


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
    sempr_->addModule(collectObjectUpdates);

    // load everything
    std::vector<Entity::Ptr> everything;
    storage->loadAll(everything);
    for (auto e : everything) {
        std::cout << "loaded " << e->id() << std::endl;
        e->loaded();
    }

    // setup object anchoring
    anchoring_ = new anchoring::SimpleAnchoring(sempr_);
    anchoring_->setVisualSim(new anchoring::VisualSim(sempr_, ""));
    anchoring_->setLogger(std::make_shared<anchoring::LoggerOStream>(&std::cout));
    anchoring_->getVisualSim()->setCameraPose(Eigen::Affine3d::Identity());

    // loading the RDFDocument is considered a real configuration, so it is done in the
    // configureHook.


    // try to get the LocalCS that stores the astronauts pose
    try {
       astronautPose_ =  storage->load<sempr::entity::LocalCS>("AstronautPose");
       astronautPose_->loaded();
    } catch (std::exception& e) {
        std::cout << "AstronautPose not loaded: " << e.what() << std::endl;
    }
}


    
void SEMPREnvironment::republish()
{
    auto objects = std::make_shared<sempr::query::ObjectQuery<sempr::entity::SpatialObject>>();
    sempr_->answerQuery(objects);

    // also, as batch. workaround for connection problems (unbuffered data connection...)
    std::vector<sempr_rock::SpatialObject> batch;
    batch.resize(objects->results.size());
    for (size_t i = 0; i < objects->results.size(); i++)
    {
        auto& obj = objects->results[i];
        batch[i].id = obj->id();
        auto tf = obj->geometry()->getCS()->transformationToRoot();
        batch[i].position = tf.translation();
        batch[i].orientation = tf.rotation();
        batch[i].type = obj->type();
    }

    _objectUpdatesBatch.write(batch);
}



SEMPREnvironment::~SEMPREnvironment()
{
    delete anchoring_;
    delete sempr_;
}



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


std::string SEMPREnvironment::explainTriple(const sempr_rock::Triple& triple, int32_t maxDepth, bool vertical)
{
    auto mod = sempr_->getModule<ReteReasonerModule>();
    sempr::entity::Triple t;
    t.subject = triple.subject_;
    t.predicate = triple.predicate_;
    t.object = triple.object_;

    return mod->explain(t, maxDepth, vertical);
}


std::vector<sempr_rock::Triple> SEMPREnvironment::listTriples(const std::string& subject, 
                                                              const std::string& predicate, 
                                                              const std::string& object)
{
    std::vector<sempr_rock::Triple> results;

    // need an object query that uses dynamic casts, because RDFEntity is just an interface
    auto rdfQuery = std::make_shared<sempr::query::ObjectQuery<sempr::entity::RDFEntity>>(nullptr, true);
    sempr_->answerQuery(rdfQuery);

    for (auto entity : rdfQuery->results)
    {
        for (auto triple : *entity)
        {
            if ((subject == "*" || subject == triple.subject) &&
                (predicate == "*" || predicate == triple.predicate) &&
                (object == "*"  || object == triple.object))
            {
                results.push_back({triple.subject, triple.predicate, triple.object});
            }
        }
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

    Eigen::Affine3d tf;
    // object not found? ... uhm... try finding a LocalCS with that id.
    if (query->results.empty())
    {
        auto queryCS = std::make_shared<ObjectQuery<LocalCS>>(
            [&arg0](LocalCS::Ptr cs) {
                return (cs->id() == arg0) || ((sempr::baseURI() + cs->id()) == arg0);
            }
        );
        sempr_->answerQuery(queryCS);

        if (queryCS->results.empty())
        {
            return base::Pose(); // TODO How to signal an error here?
        }
        else
        {
            // it is indeed a LocalCS, get its pose.
            tf = queryCS->results[0]->transformationToRoot();
        }
    }
    else
    {
        // it is a SpatialObject, get the pose of its associated LocalCS
        tf = query->results[0]->geometry()->getCS()->transformationToRoot();
    }

    // make it a base::Pose
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

    // get all objects of the type we search for
    auto tQuery = std::make_shared<SPARQLQuery>();
    tQuery->query = "SELECT * WHERE { ?obj rdf:type <" + type + "> . }";
    sempr_->answerQuery(tQuery);

    for (auto obj : soQuery->results)
    {
        // check if the object is of the specified type!
        bool found = false;
        if (type.empty()) found = true;
        else
        {
            for (auto typeResult : tQuery->results)
            {
                if (typeResult["obj"].second == sempr::baseURI() + obj->id())
                {
                    found = true;
                    break;
                }
            }
        }

        if (found) objects.push_back(obj->id());
        else std::cout << obj->id() << " is not of type " << type << ", but " << obj->type() << std::endl;
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

    auto conf = _anchoring_config.get();
    anchoring_->minScoreForPoseUpdate(conf.minScoreForPoseUpdate);
    anchoring_->maxTimesUnseen(conf.maxTimesUnseen);
    anchoring_->maxDurationUnseen(conf.maxDurationUnseen);
    anchoring_->requireFullyInViewToAdd(conf.requireFullyInViewToAdd);
    anchoring_->maxMatchingDistance(conf.maxMatchingDistance);
    anchoring_->getVisualSim()->setFrustum(
        {
            conf.frustumAlpha,
            conf.frustumBeta,
            conf.frustumMin,
            conf.frustumMax
        }
    );

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
 
    // update the astronaut pose
    base::samples::RigidBodyState rbs;
    auto status = _astronautPose.read(rbs);
    if (status != RTT::FlowStatus::NewData) return;

    if (astronautPose_)
    {
        astronautPose_->setTransform(rbs.getTransform());
        astronautPose_->changed();
    }
    else
    {
        // first time to get a pose -> create the object.
        astronautPose_ = std::make_shared<sempr::entity::LocalCS>(
            new sempr::core::PredefinedID("AstronautPose"));
        astronautPose_->setTransform(rbs.getTransform());

        // get the LocalCS_map and set it as a parent of the astronaut pose
        auto query = std::make_shared<sempr::query::LoadingQuery<sempr::entity::LocalCS>>("LocalCS_map");
        sempr_->answerQuery(query);
        if (!query->results.empty())
        {
            astronautPose_->setParent(query->results[0]);

            // announce it
            astronautPose_->created();
        }
    }
}


void SEMPREnvironment::detectionArrayTransformerCallback(const base::Time &ts, const ::mars::Detection3DArray &detections)
{
    // I believe that the fake object recognition is wrong.
    // There is a orientation offset defined at "Camera_Right" in MARS,
    // but I only get the transformation to "link_Camera_right", to which
    // "Camera_Right" is attached.
    auto camOffset = Eigen::Quaterniond(0.5, -0.5, 0.5, 0.5).inverse();

    // get the transformation from cam to map
    Eigen::Affine3d tf;
    if (!this->_camera2map.get(ts, tf, true)) 
    {
        std::cout << "detectionArrayTransformerCallback without a transformation!" << std::endl;
        return;
    }

    std::cout << "detectionArrayTransformerCallback with transform:" << std::endl
        << tf.matrix() << std::endl;

    // Update camera pose (relative to map)
    auto camZtoX = Eigen::Quaterniond(1, 0, 1, 0).normalized();
    std::cout << "camZtoX: x: " 
        << camZtoX.x() << ", y:"
        << camZtoX.y() << ", z:"
        << camZtoX.z() << ", w:"
        << camZtoX.w() << std::endl;
    anchoring_->getVisualSim()->setCameraPose(tf * camZtoX);

    anchoring::Detection3DArray darr;
    mars2sempr(detections, darr, _anchoring_config.get().fakeRecognition);

    // DEBUG: add a "gaze" object to see where the virtual camera (VisualSim)
    // is aiming at. Should be aiming at. ... 
    anchoring::Detection3D gaze;
    gaze.bbox.center = Eigen::Affine3d::Identity();
    gaze.bbox.size = Eigen::Vector3d(0,0,0);
    gaze.results.resize(1);
    gaze.results[0].id_str = "gaze";
    gaze.results[0].pose = Eigen::Affine3d::Identity();
    // VisualSim expects objects in the Z direction.
    gaze.results[0].pose.translation() = Eigen::Vector3d(0, 0, 2);
    // camZtoX is the transformation to fix this for rock cameras, which look into X direction.
    // (at least the frame in the urdf has X pointing forward.)
    // camOffset.inverse() is used to cancel the "camOffset" added to all detections from the
    // object recognition. It is added in the first place to compensate for the fact that
    // the fake object recognition gives us poses that are *not* in link_Camera_right,
    // but in "Camera_Right". But "Camera_Right" is not available in the transformer, as
    // it is not part of the URDF, but is the result of adding an internal position and
    // orientation offset to the camera *sensor* in mars.
    // (mars -> entity viewer -> sensors -> Camera_Right has a parent frame and the mentioned
    // orientation offset, which is canceled by camOffset.)
    gaze.results[0].pose = camOffset.inverse() * camZtoX * gaze.results[0].pose;
    gaze.results[0].score = 10.;

    std::cout << "gaze initial: " << std::endl << gaze.results[0].pose.matrix() << std::endl;

    darr.push_back(gaze);

    //tf = tf.inverse();

    // transform all detections into the map coordinate system
    for (auto& detection : darr)
    {
        detection.bbox.center = tf * camOffset * detection.bbox.center;

        // empty the source_cloud: let sempr-anchoring create a default point
        detection.source_cloud = pcl::PCLPointCloud2();
        for (auto& result : detection.results)
        {
            result.pose = tf * camOffset * result.pose;
        }
    }


    // TODO: get and process a pointcloud. This is not even implemented in
    //  the anchoring, so don't bother for now...
    pcl::PointCloud<pcl::PointXYZ> cloud;
    cloud.push_back(pcl::PointXYZ(0, 0, 0));
    pcl::PCLPointCloud2::Ptr cloudmsg(new pcl::PCLPointCloud2());
    pcl::toPCLPointCloud2(cloud, *cloudmsg);

    auto updatedObjects = anchoring_->anchoring(darr, cloudmsg);

    // after every processing of a detection array, the changes on
    // spatial objects are represented in pendingVizUpdates_.
    // (see the CallbackModule added to sempr)
    _objectUpdatesBatch.write(pendingVizUpdates_);


    // also, use this to publish a message with the latest updates in a format
    // compatible to the hybrit EnvMonitor
    hybrit_rock_msgs::EnvUpdate update;
    update.frame_id = "";
    update.time = base::Time::now();
    for (auto obj : pendingVizUpdates_)
    {
        update.updated_objects.push_back(sempr::baseURI() + obj.id);
    }

    this->_env_update.write(update);

    // clear the list of pending updates
    pendingVizUpdates_.clear();
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
