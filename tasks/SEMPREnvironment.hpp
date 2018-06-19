/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef SEMPR_SEMPRENVIRONMENT_TASK_HPP
#define SEMPR_SEMPRENVIRONMENT_TASK_HPP

#include "sempr/SEMPREnvironmentBase.hpp"
#include "mars/objectDetectionTypes.hpp"

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
    protected:

        /**
            A SEMPR-instance.
        */
        sempr::core::Core* sempr_;

        /**
            Initialize the sempr instance: Database, modules, ...
        */
        void initializeSEMPR();

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


        /* Returns the pose of an object given it's id in the environment representation.
         */
        virtual ::base::Pose getObjectPose(::std::string const & arg0) override;

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
        // ~SEMPREnvironment();

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
