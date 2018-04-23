#ifndef SEMPR_ANCHORING_HPP_
#define SEMPR_ANCHORING_HPP_

#include <sempr/core/Core.hpp>

#include <sempr/entity/spatial/SpatialObject.hpp>
#include <sempr/entity/spatial/LocalCS.hpp>
#include <SpatialObject_odb.h>
#include <LocalCS_odb.h>

#include <sempr/query/ObjectQuery.hpp>
#include <sempr/query/SPARQLQuery.hpp>

#include <utility>
#include "base/Pose.hpp"

#include <set>

namespace sempr {


typedef std::pair<std::string, base::Pose> Detection;
/**
    Given a sempr-instance and a list detections, simplified to (type, pose), get matching
    SpatialObjects

    For a first start, connecting to the simulation instead of the object detection, I've made a
    few assumptions:
        - exactly 1 hypothesis per detection
        - entities do not disappear, and we always get every entity in one detection detectionArray
            (so we dont have to worry about handling (i.e., removing) unseen entities)
*/
void getMatchingObjects(sempr::core::Core* sempr,
                        std::vector<Detection> const & detections,
                        std::vector<entity::SpatialObject::Ptr>& matches)
{
    matches.clear();

    std::set<entity::SpatialObject::Ptr> usedEntities_;

    for (auto detection : detections)
    {
        std::string type = detection.first;
        base::Pose pose = detection.second;

        // query for all objects of the given type
        auto query = std::make_shared<query::ObjectQuery<entity::SpatialObject>>(
            [&type](entity::SpatialObject::Ptr obj) {
                return obj->type() == type;
            }
        );

        // remove all that have been used before
        for (auto it = query->results.begin(); it != query->results.end();)
        {
            if (usedEntities_.find(*it) != usedEntities_.end())
            {
                it = query->results.erase(it);
            } else {
                ++it;
            }
        }

        // find the closest one
        float minDist = std::numeric_limits<float>::max();
        entity::SpatialObject::Ptr closest;
        for (auto obj : query->results)
        {
            auto tf = obj->geometry()->getCS()->transformationToRoot();
            auto diff = tf.translation() - pose.toTransform().translation();
            auto dist = diff.transpose() * diff;

            if (dist < minDist) {
                minDist = dist;
                closest = obj;
            }
        }

        // save
        matches.push_back(closest);
    }
}



/**
    Get or create a root-frame with id "LocalCS_map".
*/
entity::LocalCS::Ptr getOrCreateRootFrame(sempr::core::Core* sempr)
{
    auto query = std::make_shared<query::ObjectQuery<entity::LocalCS>>(
        [](entity::LocalCS::Ptr cs) {
            return cs->id() == "LocalCS_map";
        }
    );
    sempr->answerQuery(query);

    if (query->results.empty())
    {
        entity::LocalCS::Ptr cs(new entity::LocalCS(new core::PredefinedID("LocalCS_map")));
        cs->created();
        cs->changed();  // workaround. (saved only on changed())
        return cs;
    } else {
        return query->results[0];
    }
}


/**
    Create a SpatialObject instance (plus localcs, and the root already set)
*/
entity::SpatialObject::Ptr createSpatialObject(sempr::core::Core* sempr)
{
    auto root = getOrCreateRootFrame(sempr);

    entity::LocalCS::Ptr objcs(new entity::LocalCS());
    objcs->setParent(root);
    objcs->created(); objcs->changed();

    entity::SpatialObject::Ptr obj(new entity::SpatialObject());
    obj->geometry()->setCS(objcs);
    obj->created();

    return obj;
}


/**
    Update the SpatialObject with the pose and type from the detection. TODO: geometry!
*/
void updateSpatialObject(entity::SpatialObject::Ptr obj, Detection const & detectionPair)
{
    obj->geometry()->geometry()->empty();
    const char* ptwkt = "POINT Z(0 0 0)";
    obj->geometry()->geometry()->importFromWkt(&ptwkt); // TODO

    obj->type(detectionPair.first, 1.0);    // TODO type confidence

    auto cs = std::dynamic_pointer_cast<entity::LocalCS>(obj->geometry()->getCS());
    auto pose = detectionPair.second;

    cs->setTranslation(pose.position);
    cs->setRotation(pose.orientation);

    cs->changed();
    obj->changed();
}


} /* sempr */


#endif /* end of include guard: SEMPR_ANCHORING_HPP_ */
