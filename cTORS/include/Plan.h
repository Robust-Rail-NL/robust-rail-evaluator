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
/**
 * An origin's own verdict on whether its submitted POSPlan satisfies all
 * hard constraints - not a claim about whether some other plan for the same
 * Scenario might exist. Mirrors PB_HIP_Feasibility (see Proto.h), kept as a
 * plain enum here so POSPlan's public interface doesn't have to expose a
 * protobuf type.
 */
enum class Feasibility { Unknown, Feasible, Infeasible };

class POSPlan {
private:
    vector<POSAction> actions;
    vector<POSMatch> matching;
    vector<POSPrecedenceConstraint> graph;
    bool feasible;
    // The plan's own declared feasibility/origin/cost metadata (see
    // PB_HIP_Plan). Defaults match what an absent field means on the wire:
    // Unknown feasibility, no origin/cost/explanation given.
    Feasibility feasibility = Feasibility::Unknown;
    string origin = "";
    double cost = 0.0;
    string costDetails = "";
public:
    /** Construct an empty POSPlan */
    POSPlan() = default;
    /** Construct a POSPlan based on the list of POSAction%s */
    POSPlan(vector<POSAction> actions) : actions(actions) {}
    /** Get the list of POSAction%s */
    inline const vector<POSAction>& GetActions() const { return actions; }
    /** Add a POSAction to the list of POSAction%s */
    inline void AddAction(const POSAction& action) { actions.push_back(action); }
    /** Get the origin's declared feasibility verdict for this specific plan (Unknown if none) */
    inline Feasibility GetFeasibility() const { return feasibility; }
    /** Set the origin's declared feasibility verdict for this specific plan */
    inline void SetFeasibility(Feasibility f) { feasibility = f; }
    /** Get the free-text identification of what produced this plan ("" if none) */
    inline const string& GetOrigin() const { return origin; }
    /** Set the free-text identification of what produced this plan */
    inline void SetOrigin(const string& o) { origin = o; }
    /** Get the origin's declared total cost for this plan (0.0 if none) */
    inline double GetCost() const { return cost; }
    /** Set the origin's declared total cost for this plan */
    inline void SetCost(double c) { cost = c; }
    /** Get the free-form cost breakdown for this plan ("" if none) */
    inline const string& GetCostDetails() const { return costDetails; }
    /** Set the free-form cost breakdown for this plan */
    inline void SetCostDetails(const string& c) { costDetails = c; }
    /** Serialize this plan to a protobuf object */
    void Serialize(LocationEngine& engine, const Scenario& scenario, PBPOSPlan* pb_plan) const;
    /** Serialize this plan to a protobuf file */
    void SerializeToFile(LocationEngine& engine, const Scenario& scenario, const string& outfile) const;
    /** Construct a POSPlan from a protobuf object */
    static POSPlan CreatePOSPlan(const Location* location, const Scenario* scenario, const PBPOSPlan& pb_plan);
};

/**
 * One outgoing train whose IDs match an Exit action, but whose declared departure
 * time fell outside the action's suggested time window - the caller has already
 * filtered by ID match (via ShuntingUnit::MatchesTrainIDs, tested on its own in
 * VehicleTest.cpp); this only ever describes a genuine time mismatch.
 */
struct ExitCandidate {
    int id;
    int departureTime;
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

    /**
     * Whether the next action belonging to this shunting unit, after the one at
     * `index`, is an Exit. False if the unit has no further action in the plan.
     *
     * A plan interleaves the actions of every shunting unit in time order, so the
     * next entry in the list usually belongs to a different unit. Asking about the
     * list rather than the unit is what produced a spurious EndMove before a
     * departure.
     */
    static bool NextActionForUnitIsExit(const std::vector<PB_HIP_Action> &actions, int index,
                                        const PB_HIP_ShuntingUnit &unit);

    static PBAction CreateBeginMoveAction(PB_HIP_Action &pb_hip_action);

    static PBAction CreateEndMoveAction(PB_HIP_Action &pb_hip_action);

    /**
     * Build the diagnostic reported when an Exit action's suggested time window
     * matches no outgoing train's declared departure time.
     *
     * Takes only ID-matching candidates (see ExitCandidate). Falls back to a
     * single "no such outgoing train" message when the caller found no ID
     * match at all.
     *
     * @param candidates outgoing trains whose IDs matched the action, but whose
     *   time didn't
     * @param actionStart the Exit action's suggested start time
     * @param actionEnd the Exit action's suggested end time
     * @param trainIDs the train IDs the Exit action was for, used only in the
     *   no-ID-match fallback message
     */
    static string FormatExitMismatchError(const std::vector<ExitCandidate> &candidates,
                                          int actionStart, int actionEnd, const std::vector<int> &trainIDs);

};
    /** Read Plan from JSON and create protobuf fromat plan (run result)*/
    /** Added on 27 January 2025, by R.G. Kromes - extentions for HIP - cTORS compatibility*/
    void GetRunResultProto(string planFileString, PBRun &pb_runResult);

    /** Read pure HIP Plan from JSON and create a HIP protobuf fromat plan*/
    /** It is basically a JSON protobuf parser */
    /** Added on 26 February 2025, by R.G. Kromes - extentions for HIP - cTORS compatibility*/
    void ParseHIP_PlanFromJson(string planFileString, PB_HIP_Plan &pb_hip_plan);

#endif
