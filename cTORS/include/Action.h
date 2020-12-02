#pragma once
#include <list>
#include <vector>
#include "ShuntingUnit.h"
#include "TrainGoals.h"
#include "Track.h"
#include "Employee.h"
#include "Config.h"
#include "Location.h"

#ifndef ACTION_OVERRIDE
#define ACTION_OVERRIDE(name) \
	void Start(State* state) const override; \
	void Finish(State* state) const override; \
	inline name* clone() const override { return new name(*this); } \
	const string toString() const override;
#endif

using namespace std;

class ShuntingUnit;
class Track;
class BusinessRule;
class State;

class Action
{
private:
	static int newUID;
protected:
	const int uid;
	const ShuntingUnit* su;
	const int duration;
	const vector<const Employee*> employees;
	const vector<const Track*> reserved;
public:
	Action() = delete;
	Action(const ShuntingUnit* su, vector<const Track*> reserved, vector<const Employee*> employees, int duration) 
		: uid(newUID++), su(su), reserved(reserved), employees(employees), duration(duration) {};
	Action(const ShuntingUnit* su, const Employee* employee, int duration)
		: Action(su, {}, vector<const Employee*> {employee}, duration) {};
	Action(const ShuntingUnit* su, vector<const Track*> reserved, const Employee* employee, int duration) 
		: Action(su, reserved, vector<const Employee*> {employee}, duration) {};
	Action(const ShuntingUnit* su) : Action(su, {}, {}, -1) { };
	Action(const Action& action) = default;
	virtual ~Action() = default;
	inline int GetDuration() const { return duration; }
	inline const ShuntingUnit* GetShuntingUnit() const { return su; }
	inline const vector<const Employee*>& GetEmployees() const { return employees; }
	virtual void Start(State* state) const = 0;
	virtual void Finish(State* state) const = 0;
	virtual Action* clone() const = 0;
	virtual const string toString() const = 0;
	inline const bool IsEqual(const Action& a) const { return uid == a.uid; }
	inline virtual bool operator==(const Action& a) const { return IsEqual(a); }
	inline virtual bool operator!=(const Action& a) const { return !IsEqual(a); }
	inline const vector<const Track*>&  GetReservedTracks() const { return reserved; }
};

class ArriveAction : public Action { 
private:
	const Incoming* incoming;
public:
	ArriveAction() = delete;
	ArriveAction(const ShuntingUnit* su, int duration, const Incoming* incoming) 
		: Action(su, incoming->IsInstanding() ? vector<const Track*>() : vector<const Track*>({incoming->GetParkingTrack()}), 
				{}, duration), incoming(incoming) {};
	ArriveAction(const ShuntingUnit* su, const Incoming* incoming) : Action(su), incoming(incoming) {};
	inline const Track* GetDestinationTrack() const { return incoming->GetParkingTrack(); }
	inline const Incoming* GetIncoming() const { return incoming; }
	ACTION_OVERRIDE(ArriveAction)
};

class ExitAction : public Action {
private:
	const Outgoing* outgoing;
public:
	ExitAction() = delete;
	ExitAction(const ShuntingUnit* su, int duration, const Outgoing* outgoing) 
		: Action(su, outgoing->IsInstanding() ? vector<const Track*>() : vector<const Track*>({ outgoing->GetParkingTrack() }), 
			{}, duration), outgoing(outgoing) {};
	ExitAction(const ShuntingUnit* su, const Outgoing* outgoing) : Action(su), outgoing(outgoing) {};
	inline const Track* GetDestinationTrack() const { return outgoing->GetParkingTrack(); }
	inline const Outgoing* GetOutgoing() const { return outgoing; }
	ACTION_OVERRIDE(ExitAction)
};

class BeginMoveAction : public Action {
public:
	BeginMoveAction() = delete;
	BeginMoveAction(const ShuntingUnit* su, int duration) : Action(su, {}, duration) {};
	ACTION_OVERRIDE(BeginMoveAction)
};

class EndMoveAction : public Action {
public:
	EndMoveAction() = delete;
	EndMoveAction(const ShuntingUnit* su, int duration) : Action(su, {}, duration) {};
	ACTION_OVERRIDE(EndMoveAction)
};

class MoveAction : public Action {
private:
	const vector<const Track*> tracks;
public:
	MoveAction() = delete;
	MoveAction(const ShuntingUnit* su, const vector<const Track*> &tracks, int duration) : 
		Action(su, vector<const Track*>(tracks.begin()+1, tracks.end()), {}, duration), tracks(tracks) {};
	MoveAction(const ShuntingUnit* su, const vector<const Track*> &tracks) : MoveAction(su, tracks, 0) {};
	inline const Track* GetDestinationTrack() const { return tracks.back(); }
	inline const Track* GetPreviousTrack() const { return tracks[tracks.size()-2]; }
	inline const vector<const Track*>& GetTracks() const { return tracks; }
	ACTION_OVERRIDE(MoveAction)
};

class CombineAction : public Action {
private:
	const ShuntingUnit *rearSU, *combinedSU;
	bool inNeutral;
	int position;
public:
	CombineAction() = delete;
	CombineAction(const ShuntingUnit* frontSU, const ShuntingUnit* rearSU, const ShuntingUnit& combinedSU,
		const Track* track, int duration, bool inNeutral, int position) :
		Action(frontSU, {track}, {}, duration), rearSU(rearSU), combinedSU(new ShuntingUnit(combinedSU)), inNeutral(inNeutral), position(position) {};
	CombineAction(const CombineAction& ca) : CombineAction(ca.su, ca.rearSU, *ca.combinedSU, ca.reserved[0], ca.duration, ca.inNeutral, ca.position) {}
	~CombineAction() override;
	inline const ShuntingUnit* GetFrontShuntingUnit() const { return GetShuntingUnit(); }
	inline const ShuntingUnit* GetRearShuntingUnit() const { return rearSU; }
	inline const ShuntingUnit* GetCombinedShuntingUnit() const { return combinedSU; }
	ACTION_OVERRIDE(CombineAction)
};
class SplitAction : public Action {
private: 
	const ShuntingUnit *suA, *suB;
public:
	SplitAction() = delete;
	SplitAction(const ShuntingUnit* su, const Track* track, int duration, const ShuntingUnit& suA, const ShuntingUnit& suB) : 
		Action(su, {track}, {}, duration), suA(new ShuntingUnit(suA)), suB(new ShuntingUnit(suB)) {};
	SplitAction(const SplitAction& sa) : SplitAction(sa.su, sa.reserved[0], sa.duration, *sa.suA, *sa.suB) {}
	~SplitAction() override;
	inline const ShuntingUnit* GetASideShuntingUnit() const { return suA; }
	inline const ShuntingUnit* GetBSideShuntingUnit() const { return suB; }
	ACTION_OVERRIDE(SplitAction)
};

class ServiceAction : public Action {
private:
	const Train * train;
	const Facility* facility;
	const Task* task;
public:
	ServiceAction() = delete;
	ServiceAction(const ShuntingUnit* su, const Train* tu, const Task* ta, const Facility* fa, vector<const Employee*> employees) :
		Action(su, {}, employees, ta->duration), train(tu), task(ta), facility(fa) {}
	inline const Train* GetTrain() const { return train; }
	inline const Facility* GetFacility() const { return facility; }
	inline const Task* GetTask() const { return task; }
	ACTION_OVERRIDE(ServiceAction)
};

class SetbackAction : public Action {
public:
	SetbackAction() = delete;
	SetbackAction(const ShuntingUnit* su, vector<const Employee*> drivers, int duration) : Action(su, {}, drivers, duration) {}
	const inline vector<const Employee*>& GetDrivers() const { return GetEmployees(); }
	ACTION_OVERRIDE(SetbackAction)
};

class WaitAction : public Action {
public:
	WaitAction() = delete;
	WaitAction(const ShuntingUnit* su, int duration) : Action(su, {}, duration) {}
	ACTION_OVERRIDE(WaitAction)
};

class ActionValidator {
private:
	vector<BusinessRule*> validators;
	const Config* config;
	void AddValidators();
public:
	ActionValidator(const Config* config) : config(config) {
		AddValidators();
	}
	~ActionValidator();
	pair<bool, string> IsValid(const State* state, const Action* action) const;
	void FilterValid(const State* state, list<const Action*>& actions) const;
};

class ActionGenerator {
protected:
	const Location* location;
public:
	ActionGenerator() = delete;
	ActionGenerator(const ActionGenerator& am) = delete;
	ActionGenerator(const json& params, const Location* location) : location(location) {}
	~ActionGenerator() = default;
	virtual void Generate(const State* state, list<const Action*>& out) const = 0;
};

class ActionManager {
private:
	vector<ActionGenerator*> generators;
	const Config* config;
	const Location* location;
	void AddGenerators();
	void AddGenerator(string name, ActionGenerator* generator);
public:
	ActionManager() = delete;
	ActionManager(const ActionManager& am) = delete;
	ActionManager(const Config* config, const Location* location) : config(config), location(location) {
		AddGenerators();
	}
	~ActionManager();
	void Generate(const State* state, list<const Action*>& out) const;

};

#ifndef OVERRIDE_ACTIONGENERATOR
#define OVERRIDE_ACTIONGENERATOR(name) \
	name() = delete; \
	name(const name& n) = delete; \
	void Generate(const State* state, list<const Action*>& out) const override;
#endif

#ifndef DEFINE_ACTIONGENERATOR
#define DEFINE_ACTIONGENERATOR(name) \
class name : public ActionGenerator { \
public: \
	name(const json& params, const Location* location) : ActionGenerator(params, location) {}; \
	OVERRIDE_ACTIONGENERATOR(name) \
};
#endif

DEFINE_ACTIONGENERATOR(ArriveActionGenerator)
DEFINE_ACTIONGENERATOR(ExitActionGenerator)
DEFINE_ACTIONGENERATOR(BeginMoveActionGenerator)
DEFINE_ACTIONGENERATOR(EndMoveActionGenerator)
DEFINE_ACTIONGENERATOR(WaitActionGenerator)
DEFINE_ACTIONGENERATOR(ServiceActionGenerator)
DEFINE_ACTIONGENERATOR(SplitActionGenerator)
DEFINE_ACTIONGENERATOR(CombineActionGenerator)

class MoveActionGenerator : public ActionGenerator {
private:
	int noRoutingDuration, constantTime;
	bool defaultTime, normTime, walkTime;
	void GenerateMovesFrom(const ShuntingUnit* su, const vector<const Track*> &tracks,
			const Track* previous, int duration, list<const Action*> &out) const;
public:
	MoveActionGenerator(const json& params, const Location* location);
	OVERRIDE_ACTIONGENERATOR(MoveActionGenerator)
};

class SetbackActionGenerator : public ActionGenerator {
private:
	bool defaultTime, normTime, walkTime;
	int constantTime;
	int GetDuration(const State* state, const ShuntingUnit* su, int numDrivers) const;
public:
	SetbackActionGenerator(const json& params, const Location* location);
	OVERRIDE_ACTIONGENERATOR(SetbackActionGenerator)
};



