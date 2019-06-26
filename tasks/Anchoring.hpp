#ifndef SEMPR_ANCHORING_HPP_
#define SEMPR_ANCHORING_HPP_

#include <sempr/core/Core.hpp>

#include <sempr/entity/spatial/SpatialObject.hpp>
#include <sempr/entity/spatial/reference/LocalCS.hpp>
#include <SpatialObject_odb.h>
#include <LocalCS_odb.h>

#include <sempr/query/ObjectQuery.hpp>
#include <sempr/query/SPARQLQuery.hpp>

#include <utility>
#include "base/Pose.hpp"

#include <set>

#include <sempr-anchoring/BoundingBox3D.hpp>
#include <sempr-anchoring/Detection3D.hpp>
#include <sempr-anchoring/ObjectHypothesis.hpp>

#include "mars/objectDetectionTypes.hpp"

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
                        std::vector<entity::SpatialObject::Ptr>& matches);


/**
    Get or create a root-frame with id "LocalCS_map".
*/
entity::LocalCS::Ptr getOrCreateRootFrame(sempr::core::Core* sempr);



/**
    Create a SpatialObject instance (plus localcs, and the root already set)
*/
entity::SpatialObject::Ptr createSpatialObject(sempr::core::Core* sempr);


/**
    Update the SpatialObject with the pose and type from the detection. TODO: geometry!
*/
void updateSpatialObject(entity::SpatialObject::Ptr obj, Detection const & detectionPair);


/**
    Conversion functions mars <-> sempr-anchoring
*/
void mars2sempr(const mars::BoundingBox3D& in, anchoring::BoundingBox3D& out);
void mars2sempr(const mars::Detection3D& in, anchoring::Detection3D& out, bool addSimulationID = false);
void mars2sempr(const mars::Detection3DArray& in, anchoring::Detection3DArray& out, bool addSimulationID = false);
void mars2sempr(const mars::ObjectHypothesisWithPose& in, anchoring::ObjectHypothesis& out, bool addSimulationID = false);

} /* sempr */


#endif /* end of include guard: SEMPR_ANCHORING_HPP_ */
