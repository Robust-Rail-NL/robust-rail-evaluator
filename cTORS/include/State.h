#pragma once
#include <list>
#include <map>
#include "Utils.h"
#include "Event.h"
#include "Action.h"
#include "EventQueue.h"
#include "Scenario.h"
#include "TrainGoals.h"
using namespace std;

struct ShuntingUnitState {
	const Track* position;
	const Track* previous;
	list<const Action*> activeActions;
	bool moving, waiting, inNeutral;
	const Train* frontTrain;


	ShuntingUnitState() = delete;
	ShuntingUnitState(const ShuntingUnitState& suState) : position(suState.position), previous(suState.previous),
		moving(suState.moving), waiting(suState.waiting), inNeutral(suState.inNeutral), frontTrain(suState.frontTrain) {
			for(auto action: suState.activeActions) activeActions.emplace_back(action->clone());
		}
	ShuntingUnitState(const Track* position, const Track* previous, const Train* frontTrain) 
		: position(position), previous(previous), moving(false), waiting(false), inNeutral(true), frontTrain(frontTrain) {}
	~ShuntingUnitState() {
		DELETE_LIST(activeActions);
	}
	bool HasActiveAction() const { return activeActions.size() > 0; }
};

struct TrackState {
	list<const ShuntingUnit*> occupations;
	bool reserved;
	TrackState() : reserved(false) {};
};

struct TrainState {
	vector<Task> tasks;
	list<const Task*> activeTasks;
};

class State
{
private:
	EventQueue events;
	int time, startTime, endTime;
	vector<const Incoming*> incomingTrains;
	vector<const Outgoing*> outgoingTrains;
	vector<const Employee*> employees;
	vector<const ShuntingUnit*> shuntingUnits;
	
	map<const ShuntingUnit*, ShuntingUnitState> shuntingUnitStates;
	map<const Track*, TrackState> trackStates;
	map<const Train*, TrainState> trainStates;

	bool changed;
public:
	State() = delete;
	State(const Scenario& scenario, const vector<Track*>& tracks);
	State(const State& state) = default;
	~State();

	//Events
	inline size_t GetNumberOfEvents() const { return events.size(); }
	const Event* PeekEvent() const;
	const Event* PopEvent();
	void AddEvent(const Incoming* in);
	void AddEvent(const Outgoing* out);
	void AddEvent(const Action* action);

	//Time
	void SetTime(int time);
	inline int GetTime() const { return time; };
	inline int GetEndTime() const { return endTime; }
	inline int GetStartTime() const { return startTime; }
	
	//Changed
	inline bool IsChanged() const { return changed; }
	inline void SetUnchanged() { changed = false; }
	
	//Apply action
	void StartAction(const Action* action);
	void FinishAction(const Action* action);

	//Getters
	inline const vector<const Incoming*>& GetIncomingTrains() const { return incomingTrains; }
	inline const vector<const Outgoing*>& GetOutgoingTrains() const { return outgoingTrains; }
	inline const Track* GetPosition(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).position; }
	inline const Track* GetPrevious(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).previous; }
	inline const list<const ShuntingUnit*>& GetOccupations(const Track* track) const { return trackStates.at(track).occupations; }
	int GetPositionOnTrack(const ShuntingUnit* su) const;
	inline size_t GetAmountOnTrack(const Track* track) const { return GetOccupations(track).size(); }
	const vector<const Train*> GetTrainUnitsInOrder(const ShuntingUnit* su) const;
	bool CanMoveToSide(const ShuntingUnit* su, const Track* side) const;
	inline bool IsMoving(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).moving; }
	inline bool IsReserved(const Track* track) const { return trackStates.at(track).reserved; }
	inline bool IsWaiting(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).waiting; }
	inline bool IsInNeutral(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).inNeutral; }
	inline const vector<const ShuntingUnit*> GetShuntingUnits() const { return shuntingUnits; }
	inline bool HasActiveAction(const ShuntingUnit* su) const { return GetActiveActions(su).size() > 0; }
	inline const list<const Action*> &GetActiveActions(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).activeActions; }
	bool IsActive() const;
	bool IsAnyInactive() const;
	inline const Train* GetFrontTrain(const ShuntingUnit* su) const { return shuntingUnitStates.at(su).frontTrain; }
	inline const vector<Task>& GetTasksForTrain(const Train* tu) const { return trainStates.at(tu).tasks; }
	inline const list<const Task*>& GetActiveTasksForTrain(const Train* tu) const { return trainStates.at(tu).activeTasks; }
	inline const ShuntingUnitState& GetShuntingUnitState(const ShuntingUnit* su) const { return shuntingUnitStates.at(su); }
	inline const map<const ShuntingUnit*, ShuntingUnitState>& GetShuntingUnitStates() const { return shuntingUnitStates; }

	//Setters and Adders
	inline void SetMoving(const ShuntingUnit* su, bool b) { shuntingUnitStates.at(su).moving = b; }
	inline void SetWaiting(const ShuntingUnit* su, bool b) { shuntingUnitStates.at(su).waiting = b; }
	inline void SetInNeutral(const ShuntingUnit* su, bool b) { shuntingUnitStates.at(su).inNeutral = b; }
	inline void SetPosition(const ShuntingUnit* su, const Track* track) { shuntingUnitStates.at(su).position = track; }
	inline void SetPrevious(const ShuntingUnit* su, const Track* track) { shuntingUnitStates.at(su).previous = track; }

	void addTasksToTrains(const map<const Train*, vector<Task>>& tasks);
	inline void AddTaskToTrain(const Train* tu, const Task& task) { trainStates.at(tu).tasks.push_back(task); }
	inline void AddActiveTaskToTrain(const Train* tu, const Task* task) { trainStates.at(tu).activeTasks.push_back(task); }
	inline void AddActiveAction(const ShuntingUnit* su, const Action* action) { shuntingUnitStates.at(su).activeActions.push_back(action->clone()); }
	void AddShuntingUnit(const ShuntingUnit* su, const Track* track, const Track* previous, const Train* frontTrain);
	inline void AddShuntingUnit(const ShuntingUnit* su, const Track* track, const Track* previous) { AddShuntingUnit(su, track, previous, su->GetTrains().front()); }
	inline void SetFrontTrain(const ShuntingUnit* su, const Train* frontTrain) { shuntingUnitStates.at(su).frontTrain = frontTrain; }
	
	//Track Reservation
	void ReserveTracks(const vector<const Track*>& tracks);
	void ReserveTracks(const list<const Track*>& tracks);
	inline void ReserveTrack(const Track* track) { trackStates.at(track).reserved = true; };
	void FreeTracks(const vector<const Track*>& tracks);
	void FreeTracks(const list<const Track*>& tracks);
	inline void FreeTrack(const Track* track) { trackStates.at(track).reserved = false; };

	//Moving
	void MoveShuntingUnit(const ShuntingUnit* su, const Track* to, const Track* previous);
	void OccupyTrack(const ShuntingUnit* su, const Track* park, const Track* previous);
	
	//Remove
	void RemoveIncoming(const Incoming* incoming);
	void RemoveOutgoing(const Outgoing* outgoing);
	void RemoveShuntingUnit(const ShuntingUnit* su);
	void RemoveActiveAction(const ShuntingUnit* su, const Action* action);
	void RemoveOccupation(const ShuntingUnit* su);
	void RemoveTaskFromTrain(const Train* tu, const Task& task);
	void RemoveActiveTaskFromTrain(const Train* tu, const Task* task);
};

