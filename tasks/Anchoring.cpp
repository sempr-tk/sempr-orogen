#include "Anchoring.hpp"
#include <pcl/conversions.h>
#include <pcl/point_representation.h>
#include <algorithm>

namespace sempr {

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
        sempr->answerQuery(query);

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
    obj->changed();

    return obj;
}


/**
    Update the SpatialObject with the pose and type from the detection. TODO: geometry!
*/
void updateSpatialObject(entity::SpatialObject::Ptr obj, Detection const & detectionPair)
{
    // set a trivial geometry -- everything is just a point for now. // TODO
    auto* geosGeom = dynamic_cast<geos::geom::GeometryCollection*>(
            entity::Geometry::importFromWKT("GEOMETRYCOLLECTION(POINTZ(0 0 0))")
    );
    obj->geometry()->setGeometry(geosGeom);

    obj->type(detectionPair.first, 1.0);    // TODO type confidence

    auto cs = std::dynamic_pointer_cast<entity::LocalCS>(obj->geometry()->getCS());
    auto pose = detectionPair.second;

    cs->setTranslation(pose.position);
    cs->setRotation(pose.orientation);

    cs->changed();
    obj->changed();
}


void mars2sempr(const mars::BoundingBox3D& in, anchoring::BoundingBox3D& out)
{
    out.center = in.center.toTransform();
    out.size = in.size;
}

void mars2sempr(const mars::Detection3D& in, anchoring::Detection3D& out)
{
    mars2sempr(in.bbox, out.bbox);
    out.results.resize(1);
    mars2sempr(in.results[0], out.results[0]);
    pcl::PointCloud<pcl::PointXYZRGB> cloud;

    bool use_color = in.source_cloud.colors.size() == in.source_cloud.points.size();
    auto& ic = in.source_cloud;
    for (size_t i = 0; i < in.source_cloud.points.size(); i++)
    {
        pcl::PointXYZRGB p;
        p.x = ic.points[i].x();
        p.y = ic.points[i].y();
        p.z = ic.points[i].z();
        if (use_color)
        {
            p.a = ic.colors[i][0];
            p.r = ic.colors[i][1];
            p.g = ic.colors[i][2];
            p.b = ic.colors[i][3];
        }
        cloud.push_back(p);
    }

    pcl::toPCLPointCloud2(cloud, out.source_cloud);
}

void mars2sempr(const mars::Detection3DArray& in, anchoring::Detection3DArray& out)
{
    out.resize(in.detections.size());
    for (size_t i = 0; i < in.detections.size(); i++)
    {
        mars2sempr(in.detections[i], out[i]);
    }
}

void mars2sempr(const mars::ObjectHypothesisWithPose& in, anchoring::ObjectHypothesis& out, bool addSimulationID)
{
    out.id = in.id;
    out.pose = in.pose.pose.toTransform();
    out.score = in.score;
    out.id_str = in.type;

    // hack in case the fake object recognition is used: add the property "<object> <http://trans.fit/simulationID> id" again
    if (addSimulationID)
    {
        out.extra_info["http://trans.fit/simulationID"] = std::to_string(in.id);
    }

    // if the detections are from the fake object recognition there is no prefix. In that case,
    // add one.
    const std::string http("http://");
    if (out.id_str.size() > http.size())
    {
        auto res = std::mismatch(http.begin(), http.end(), out.id_str.begin());
        if (res.first != http.end())
        {
            // no http:// prefix. so add http://trans.fit/ for now
            out.id_str = "http://trans.fit/" + out.id_str;
        }
    }

    // TODO: map numeric id to string (or vice versa)?
}

} // ns sempr
