#pragma once
#include <string>
#include "Track.h"
#include "ShuntingUnit.h"
using namespace std;
using json = nlohmann::json;

class TrainGoal {
protected:
	int id;
	Track* parkingTrack;
	Track* sideTrack;
	ShuntingUnit* shuntingUnit;
	int time;
	int standingIndex;
	bool isInstanding;
	map<Train*, vector<Task>> tasks;
public:
	TrainGoal() = default;
	TrainGoal(int id, ShuntingUnit* su, Track* parkingTrack, Track* sideTrack, int time, bool isInstanding, int standingIndex, map<Train*, vector<Task>> tasks) :
		id(id), shuntingUnit(su), parkingTrack(parkingTrack), sideTrack(sideTrack), time(time), isInstanding(isInstanding), standingIndex(standingIndex), tasks(tasks) {};
	TrainGoal(const TrainGoal& traingoal);
	~TrainGoal();
	void fromJSON(const json& j);
	void assignTracks(Track* park, Track* side);
	inline void setInstanding(bool b) { isInstanding = b; }
	inline bool IsInstanding() const { return isInstanding; }
	inline int GetTime() const { return time; }
	inline int GetStandingIndex() const { return standingIndex; }
	inline ShuntingUnit* GetShuntingUnit() const { return shuntingUnit; }
	inline Track* GetSideTrack() const { return sideTrack; }
	inline Track* GetParkingTrack() const { return parkingTrack; }
	const inline map<Train*, vector<Task>>& GetTasks() { return tasks; }
	virtual string toString() const = 0;
};

class Incoming : public TrainGoal {
public:
	Incoming() = default;
	Incoming(int id, ShuntingUnit* su, Track* parkingTrack, Track* sideTrack, int time, bool isInstanding, int standingIndex, map<Train*, vector<Task>> tasks) :
		TrainGoal(id, su, parkingTrack, sideTrack, time, isInstanding, standingIndex, tasks) {}
	Incoming(const Incoming& incoming) : TrainGoal(incoming) {}
	~Incoming() = default;
	void fromJSON(const json& j);
	inline string toString() const override { return "Incoming " + GetShuntingUnit()->toString() + " at " + GetParkingTrack()->toString() + " at " + to_string(GetTime()); }
};

class Outgoing : public TrainGoal {
public:
	Outgoing() = default;
	Outgoing(int id, ShuntingUnit* su, Track* parkingTrack, Track* sideTrack, int time, bool isInstanding, int standingIndex) :
		TrainGoal(id, su, parkingTrack, sideTrack, time, isInstanding, standingIndex, map<Train*, vector<Task>> {}) {}
	Outgoing(const Outgoing& outgoing) : TrainGoal(outgoing) {}
	~Outgoing() = default;
	void fromJSON(const json& j);
	inline string toString() const override { return "Outgoing " + GetShuntingUnit()->toString() + " from " + GetParkingTrack()->toString() + " at " + to_string(GetTime()); }
};

void from_json(const json& j, Incoming& in);
void from_json(const json& j, Outgoing& out);