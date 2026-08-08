/** \file Plan.h
 * Describes the POSPlan (Partial Order Schedule Plan), POSAction, POSMatch and POSPrecedenceConstraint class
 */
#ifndef PLAN_H
#define PLAN_H
#include "Engine.h"
#include "Utils.h"
namespace fs = std::filesystem;


class Engine;
class LocationEngine;

/**
 * A POSAction is an action in a Partial Order Schedule
 * 
 * This class is used for serializing actions to protobuf
 */
class POSAction {
private:
    static int newUID;
    int id;
    int suggestedStart, suggestedEnd, minDuration;
    const SimpleAction* action;
public:
    POSAction() = delete;
    /** Construct a POSAction from the given parameters */
    POSAction(int suggestedStart, int suggestedEnd, int minDuration, const SimpleAction* action) :
        id(newUID++), suggestedStart(suggestedStart), suggestedEnd(suggestedEnd), minDuration(minDuration), action(action) {}
    /** Copy constructor */
    POSAction(const POSAction& pa) : id(pa.id), suggestedStart(pa.suggestedStart), suggestedEnd(pa.suggestedEnd),
        minDuration(pa.minDuration), action(pa.action->Clone()) {}
    /** POSAction destructor */
    ~POSAction() { delete action; }
    /** Assignment operator */
    POSAction& operator=(const POSAction& pa);
    /** Get the unique id of the action */
    inline int GetID() const { return id; }
    /** Get the suggested start time of the action */
    inline int GetSuggestedStart() const { return suggestedStart; }
    /** Get the suggested end time of the action */
    inline int GetSuggestedEnd() const { return suggestedEnd; }
    /** Get the suggested minimum duration time of the action */
    inline int GetMinimumDuration() const { return minDuration; }
    /** Get the SimpleAction of this POSAction */
    inline const SimpleAction* GetAction() const { return action; }
    /** Serialize this POSAction to a protobuf object */
    void Serialize(const LocationEngine& engine, const State* state, PBAction* pb_action) const;
    /** Construct a POSAction from a protobuf action */
    static POSAction CreatePOSAction(const Location* location, const Scenario* scenario, const PBAction& pb_action);
   
};

/**
 * A POSMatch describes a match between Outgoing trains and Train%s (not yet implemented)
 */
class POSMatch {
private:
    const Train* train;
    const Outgoing* out;
    int position;
};

/**
 * A POSPrecedenceConstraint describes a precedence constraint between two POSAction%s (not yet implemented)
 */
class POSPrecedenceConstraint {
    const POSAction *firstAction, *secondAction;
    int minimumTimeLag;
};

/**
 * A POPlan describes a Partial Order Schedule
 * 
 * A POSPlan consists of a list of POSAction%s, POSMatch%es and POSPrecedenceConstraint%s
 */
class POSPlan {
private:
    vector<POSAction> actions;
    vector<POSMatch> matching;
    vector<POSPrecedenceConstraint> graph;
    bool feasible;
public:
    /** Construct an empty POSPlan */
    POSPlan() = default;
    /** Construct a POSPlan based on the list of POSAction%s */
    POSPlan(vector<POSAction> actions) : actions(actions) {}
    /** Get the list of POSAction%s */
    inline const vector<POSAction>& GetActions() const { return actions; }
    /** Add a POSAction to the list of POSAction%s */
    inline void AddAction(const POSAction& action) { actions.push_back(action); }
    /** Serialize this plan to a protobuf object */
    void Serialize(LocationEngine& engine, const Scenario& scenario, PBPOSPlan* pb_plan) const;
    /** Serialize this plan to a protobuf file */
    void SerializeToFile(LocationEngine& engine, const Scenario& scenario, const string& outfile) const;
    /** Construct a POSPlan from a protobuf object */
    static POSPlan CreatePOSPlan(const Location* location, const Scenario* scenario, const PBPOSPlan& pb_plan);
};

/**
 * A RunResult describes a TORS session
 * 
 * A TORS session is run at a Location, given a certain Scenario. 
 * In this context a POSPlan will be, or was run
 */
class RunResult {
private:
    Scenario scenario;
    POSPlan plan;
    string location;
    bool feasible;
public:
    RunResult() = delete;
    /** Construct a RunResult for a location and a Scenario with an empty plan */
    RunResult(const string& location, const Scenario& scenario) : location(location), scenario(scenario), feasible(false) {}
    /** Construct a RunResult for a location and a Scenario and a plan */
    RunResult(const string& location, const Scenario& scenario, const POSPlan& plan, bool feasible)
        : location(location), scenario(scenario), plan(plan), feasible(feasible) {}
    /** Default copy constructor */
    RunResult(const RunResult& rr) = default;
    /** Default destructor */
    ~RunResult() = default;
    /** Get the actions in the plan */
    inline const vector<POSAction>& GetActions() const { return plan.GetActions(); }
    /** Add a POSAction to the plan */
    inline void AddAction(const POSAction& action) { plan.AddAction(action); }
    /** Get the Scenario for this run */
    inline const Scenario& GetScenario() const { return scenario; }
    /** Get the POSPlan for this run */
    inline const POSPlan& GetPlan() const { return plan; }
    /** Get the location string for this run */
    inline const string& GetLocation() const { return location; }
    /** Serialize this object to a protobuf object */
    void Serialize(LocationEngine& engine, PBRun* pb_run) const;
    /** Serialize this object to a protobuf file */
    void SerializeToFile(LocationEngine& engine, const string& outfile) const;
    /** Construct a RunResult from a protobuf object by using the provided Engine */
    static RunResult* CreateRunResult(const Engine& engine, const PBRun& pb_run);
    /** Construct a RunResult from a protobuf object by using the provided Location */
    static RunResult* CreateRunResult(const Location* location, const PBRun& pb_run);
    /** Construct a RunResult from a HIP protobuf object by using the provided Location
     * Added on 27 of February 2025, by R.G. Kromes - extentions for HIP - cTORS compatibility*/
    static RunResult* CreateRunResult(const PB_HIP_Plan& pb_hip_plan, string scenarioFileString, const Location *location, const string& pathToStoreEval = "", int departureDelay = 0);
    
    /**
     * Merge the Combine actions of one combine into the single two-operand action
     * that cTORS expects.
     *
     * HIP emits one Combine action per participating shunting unit, while a POS
     * Combine carries both operands at once: the action's own trainUnitIds are the
     * front unit and the task's trainUnitIds the rear one. Both lists arrive holding
     * the first action's units, so the rear operand has to be rebuilt from the
     * remaining actions rather than appended to what is already there.
     *
     * @param combineActions the Combine actions of a single combine, front unit first
     */
    static PBAction MergeCombineActions(const std::vector<PBAction> &combineActions);

    static PBAction CreateBeginMoveAction(PB_HIP_Action &pb_hip_action);

    static PBAction CreateEndMoveAction(PB_HIP_Action &pb_hip_action);

};
    /** Read Plan from JSON and create protobuf fromat plan (run result)*/
    /** Added on 27 January 2025, by R.G. Kromes - extentions for HIP - cTORS compatibility*/
    void GetRunResultProto(string planFileString, PBRun &pb_runResult);

    /** Read pure HIP Plan from JSON and create a HIP protobuf fromat plan*/
    /** It is basically a JSON protobuf parser */
    /** Added on 26 February 2025, by R.G. Kromes - extentions for HIP - cTORS compatibility*/
    void ParseHIP_PlanFromJson(string planFileString, PB_HIP_Plan &pb_hip_plan);

#endif