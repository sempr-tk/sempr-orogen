#ifndef sempr_TYPES_HPP
#define sempr_TYPES_HPP

/* If you need to define types specific to your oroGen components, define them
 * here. Required headers must be included explicitly
 *
 * However, it is common that you will only import types from your library, in
 * which case you do not need this file
 */

namespace sempr {

/**
 * Configuration object for the object anchoring.
 */
struct AnchoringConfig {
    /** The minimum score of a detection to use its pose estimate 
        to update the object (or create a new one). If the score is
        too low, no object will be created or updated, but existing
        objects won't be deleted either as they are considered to be
        observed, just not good enough. */
    float minScoreForPoseUpdate = 1.f;

    /** How many consecutive times an object must be missing in the
        detection arrays to remove it */
    int maxTimesUnseen = 5;

    /** Number of seconds that an object must not have been seen
        before removal is allowed. */
    int maxDurationUnseen = 5;

    /** Whether an object (pointcloud data) must be fully in view of
        the (simulated) camera frustum to allow adding it to the
        environment representation */
    bool requireFullyInViewToAdd = false;

    /** How far a previously stored object may be away from a new detection
     *  before it is considered a different instance.
     */
    float maxMatchingDistance = 0.3;
    
    // camera settings.
    /** Aperture angles of the (simulated) camera (radian)
     */
    float frustumAlpha = 3.14159 / 4., frustumBeta = 3.14159 / 4.;
    
    /** Range of the (simulated) camera
     */
    float frustumMin = 0.2, frustumMax = 5.0;
    
    /** Whether the detections are from the fake_object_recognition or not.
     *  If true, the simulationID will be added as additional information
     *  to the object.
     */
    bool fakeRecognition = false;
};

}

#endif

