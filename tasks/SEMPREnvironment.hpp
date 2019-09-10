/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef SEMPR_SEMPRENVIRONMENT_TASK_HPP
#define SEMPR_SEMPRENVIRONMENT_TASK_HPP

#include "sempr/SEMPREnvironmentBase.hpp"
#include "mars/objectDetectionTypes.hpp"
#include "sempr_rock/ObjectMessages.hpp"
#include <vector>

#include <sempr-anchoring/SimpleAnchoring.hpp>

// forward declarations
namespace sempr {
    namespace core {
        class Core;
    } /* core */
} /* sempr */

namespace sempr {

    /*! \class SEMPREnvironment
     * \brief The environment representation, essentially a wrapper for SEMPR. This task accepts messages from the object recognition to update the internal state (via some object anchoring mechanism), and provides operations to answer sparql-queries and assert facts.
     */
    class SEMPREnvironment : public SEMPREnvironmentBase
    {
	friend class SEMPREnvironmentBase;

    // a list of modifications made since the last time publishing on the batch update port
    std::vector<sempr_rock::SpatialObject> pendingVizUpdates_;

    protected:

        /**
            A SEMPR-instance.
        */
        sempr::core::Core* sempr_;

        /**
            Methods for object anchoring
        */
        anchoring::SimpleAnchoring* anchoring_;

        /**
            Initialize the sempr instance: Database, modules, ...
        */
        void initializeSEMPR();


        /**
            Publish a (visualization) update for an object
        */
        void publishUpdateFor(sempr::entity::SpatialObject::Ptr object);


        
        /**
            Process detection messages, aligned with a transformation from camera to map
        */
        virtual void detectionArrayTransformerCallback(const base::Time &ts, const ::mars::Detection3DArray &detectionArray_sample) override;


        /* Add an assertion to an object.
        Adds an rdf triple that is bound to the given object (will be removed together with the
        object)
        returns false if the object does not exist
         */
        virtual bool addObjectAssertion(::sempr_rock::ObjectAssertion const & arg0) override;


        /**
            Add a triple to the "global" set of knowledge, not bound to a specific (observed)
            object.
        */
        virtual bool addTriple(::sempr_rock::Triple const & arg0) override;


        /* SPARQL-Query
        argument must be a valid SPARQL query (e.g.: SELECT ?a WHERE {?a rdf:type sempr:CoffeeMug.}).
        A few namespaces are already predefined: rdf, rdfs, owl, sempr, xsd
         */
        virtual ::sempr_rock::SPARQLResult answerQuery(::std::string const & arg0) override;

        /*  Returns a dot string with the explanation for the given triple.
            maxDepth limits the depth of the explanation, e.g.
              maxDepth = 1 only lists rules that were used to get the triple,
              maxDepth = 2 also shows the triples needed for those rules,
              maxDepth = 3 shows the rules those triples originated from,
              etc.
            vertical sets changes to a vertical graph layout
         */
        virtual ::std::string explainTriple(::sempr_rock::Triple const & triple, boost::int32_t maxDepth, bool vertical) override;


        /* Returns the pose of an object given it's id in the environment representation.
         */
        virtual ::base::Pose getObjectPose(::std::string const & arg0) override;

        /**
            Try to remove a triple from the given RDFEntity. returns false if the entity
            couldn't be found.
        */
        virtual bool removeTriple(::std::string const & entity, ::sempr_rock::Triple const & triple) override;

        /**
            Publish a message for every SpatialObject on the objectUpdates port
        */
        void republish() override;

        /**
            Returns all objects of a given type in the specified cone
         */
        virtual ::std::vector< ::std::string > getObjectsInCone(::base::Pose const & direction, float length, float angle, ::std::string const & type) override;

        /* Simplified query, returns whole triples with a given pattern.
           In contrast to "answerQuery" (sparql), this returns the full string representation of
           the triple-parts. So a URI resource will include pointy brackets, like "<http://.../foo#bar>",
           etc. Just as the triples are stored internally.
           For every part of the triple you can either fully specify its content (including brackets etc),
           or use "*" as a wildcard. (Don't mix it! Either "*" or "<fully-specified-thing>",
           but **not** "<http://something/Fo*>"

           This is a relatively expensive operation as it traverses *all* triples without utilizing an index.
           May contain duplicates.
         */
        virtual ::std::vector< ::sempr_rock::Triple > listTriples(::std::string const & subject, ::std::string const & predicate, ::std::string const & object) override;



    public:
        /** TaskContext constructor for SEMPREnvironment
         * \param name Name of the task. This name needs to be unique to make it identifiable via nameservices.
         * \param initial_state The initial TaskState of the TaskContext. Default is Stopped state.
         */
        SEMPREnvironment(std::string const& name = "sempr::SEMPREnvironment");

        /** TaskContext constructor for SEMPREnvironment
         * \param name Name of the task. This name needs to be unique to make it identifiable for nameservices.
         * \param engine The RTT Execution engine to be used for this task, which serialises the execution of all commands, programs, state machines and incoming events for a task.
         *
         */
        SEMPREnvironment(std::string const& name, RTT::ExecutionEngine* engine);

        /** Default deconstructor of SEMPREnvironment
        */
        ~SEMPREnvironment();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         \verbatim
         task_context "TaskName" do
           needs_configuration
           ...
         end
         \endverbatim
         */
        bool configureHook() override;

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook() override;

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called.
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states.
         *
         * In the first case, updateHook() is still called, and recover() allows
         * you to go back into the Running state.  In the second case, the
         * errorHook() will be called instead of updateHook(). In Exception, the
         * component is stopped and recover() needs to be called before starting
         * it again. Finally, FatalError cannot be recovered.
         */
        void updateHook() override;

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook() override;

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook() override;

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook() override;
    };
}

#endif
