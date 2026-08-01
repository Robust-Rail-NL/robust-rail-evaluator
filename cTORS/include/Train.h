/** \file Train.h
 * Describes the Train class, the TrainUnitType struct and the Task struct
 */
#pragma once
#ifndef TRAIN_H
#define TRAIN_H
#include "Utils.h"
#include <utility>
using namespace std;

/**
 * The TrainUnitType describes the type of the Train
 */
struct TrainUnitType {
	static map<string,TrainUnitType*> types; /**< a static map containing all the initialized types, keyed by the legacy (non-HIP) combined displayName string */
	static map<pair<string,int>,TrainUnitType*> typesByPrefixAndCarriages; /**< a static map containing all the HIP-path-initialized types, keyed by (typePrefix, carriages) since typePrefix alone is not unique */
	const string displayName;		/**< The name of the train unit type */
	const int carriages;			/**< The number of carriages */
	const double length;			/**< The length of this train unit, in meters */
	const int combineDuration;		/**< The time it takes to combine this type of train unit with another */
	const int splitDuration;		/**< The time it takes to split this type of train unit */
	const int backNormTime;			/**< the back norm time */
	const int backAdditionTime;		/**< The back addition time */
	const int setbackTime;			/**< The time needed to setback this type of train unit, calculated as #carriages * #backAdditionTime */
	const int travelSpeed;			/**< The travel speed of the train */
	const int startUpTime;			/**< The time it takes for this type of train unit to start up / shut down */
	const string typePrefix;		/**< The prefix for this type of train unit, for example "SLT" or "VIRM" */
	const bool needsLoco;			/**< Whether or not this type of train unit needs a locomotive to move */
	const bool isLoco;				/**< Whether or not this type of train unit is a locomotive */
	const bool needsElectricity;	/**< Whether or not this type of train unit can only drive over electrified tracks */

	TrainUnitType() = delete;
	/**
	 * Construct a TrainUnitType from the given parameters.
	 * The setback time is equal to the number of carriages of this type times the back addition time
	 */
	TrainUnitType(const string& displayName, int carriages, double length, int combineDuration, int splitDuration, int backNormTime,
		int backAdditionTime, int travelSpeed, int startUpTime, const string& typePrefix, bool needsLoco, bool isLoco, bool needsElectricity) :
		displayName(displayName), carriages(carriages), length(length), combineDuration(combineDuration), splitDuration(splitDuration),
		backNormTime(backNormTime), backAdditionTime(backAdditionTime), setbackTime(carriages*backAdditionTime), travelSpeed(travelSpeed),
		startUpTime(startUpTime), typePrefix(typePrefix), needsLoco(needsLoco), isLoco(isLoco), needsElectricity(needsElectricity) {}
	/** Construct a TrainUnitType from the given legacy (non-HIP) protobuf object */
	TrainUnitType(const PBTrainUnitType& pb_tt) : TrainUnitType(pb_tt.displayname(), pb_tt.carriages(), pb_tt.length(), pb_tt.combineduration(),
		pb_tt.splitduration(), pb_tt.backnormtime(), pb_tt.backadditiontime(), pb_tt.travelspeed(), pb_tt.startuptime(), pb_tt.typeprefix(),
		pb_tt.needsloco(), pb_tt.isloco(), pb_tt.needselectricity()) {}
	/** Construct a TrainUnitType from the given HIP protobuf object */
	TrainUnitType(const PB_HIP_TrainUnitType& pb_tt) : TrainUnitType(pb_tt.typeprefix(), pb_tt.carriages(), pb_tt.length(), pb_tt.combineduration(),
		pb_tt.splitduration(), pb_tt.backnormtime(), pb_tt.backadditiontime(), pb_tt.travelspeed(), pb_tt.startuptime(), pb_tt.typeprefix(),
		pb_tt.needsloco(), pb_tt.isloco(), pb_tt.needselectricity()) {}
	/** Get a string representation of this TrainUnitType */
	const string& toString() const { return displayName; }
	/** Returns true iff the two TrainUnitType%s have the same name */
	bool operator==(const TrainUnitType& t) const { return (displayName == t.displayName); }
	/** Returns true iff the two TrainUnitType%s do not have the same name */
	bool operator!=(const TrainUnitType& t) const { return !(*this == t); }
	/** Serialize this TrainUnitType to a protobuf object */
	void Serialize(PBTrainUnitType* pb_tt) const;
};

//!\cond NO_DOC
inline string ConvertPBTaskType(const PBTaskType& pb_task_type) {
	return pb_task_type.other();
}

inline string ConvertPBTaskType(const PB_HIP_TaskType& pb_task_type) {
	return pb_task_type.other();
}

inline vector<string> ConvertPBTaskTypes(const PBList<PBTaskType>& pb_task_types) {
	vector<string> out(pb_task_types.size());
	for(int i=0; i<pb_task_types.size(); i++)
		out[i] = ConvertPBTaskType(pb_task_types[i]);
	return out;
}
//!\endcond

/**
 * A Task struct describing a task for a Train
 */
struct Task {
	string taskType;		/**< the type of this Task */
	bool optional;			/**< whether this Task is optional (false = mandatory: must be completed before the train may exit) */
	int duration;			/**< the duration of this Task in seconds */
	list<string> skills;	/**< the skills required to execute this task (not yet implemented) */

	Task() = delete;
	/** Construct a Task object given the parameters */
	Task(const string& taskType, bool optional, int duration, list<string> skills) :
		taskType(taskType), optional(optional), duration(duration), skills(skills) {}
	/** Construct a Task object given the legacy (non-HIP) protobuf object */
	Task(const PBTask& pb_task) :
		Task(ConvertPBTaskType(pb_task.type()), pb_task.priority() != 0, pb_task.duration(), PBToStringList(pb_task.requiredskills())) {}
	/** Construct a Task object given the HIP protobuf object */
	Task(const PB_HIP_Task& pb_task) :
		Task(ConvertPBTaskType(pb_task.type()), pb_task.optional(), pb_task.duration(), PBToStringList(pb_task.requiredskills())) {}
	/** Two Task%s are equal if they have the same task type, optional flag and duration */
	bool operator==(const Task& t) const { return (taskType == t.taskType && optional == t.optional && duration == t.duration); }
	/** Two Task%s are different if they have a different task type, optional flag or duration */
	bool operator!=(const Task& t) const { return !(*this == t); }
	/** Get a string representation of the Task */
	const string &toString() const { return taskType; }
	/** Serialize this Task to a legacy (non-HIP) protobuf object */
	void Serialize(PBTask* pb_task) const;
};

/**
 * A description of a Train unit.
 * 
 * A Train unit is an atomic Train in the shunting yard. It can consist of several carriages,
 * but those carriages cannot be split. Trains can be combined with other Trains to form ShuntingUnit%s
 */
class Train {
private:
	int id;
	TrainUnitType *type;
public:
	Train() = delete;
	/** Construct a Train from the given parameters */
	Train(int id, TrainUnitType *type) : id(id), type(type) {}
	/** Construct a Train from a legacy (non-HIP) protobuf object */
	Train(const PBTrainUnit& pb_train);
	/** Construct a Train from a HIP protobuf object */
	Train(const PB_HIP_TrainUnit& pb_train);
	/** The default copy constructor */
	Train(const Train &train) = default;
	/** The default desctructor */
	~Train() = default;
	/** Get the type of the train */
	inline const TrainUnitType* GetType() const { return type; }
	/** Two trains are equal if they have the same id (provided it is not -1) or if they are the same object */
	bool operator==(const Train& train) const;
	/** Two trains are different if they have a different id or if one id is -1, and they are not the same object */
	bool operator!=(const Train& t) const { return !(*this == t); }
	/** Get a string representation of the Train */
	const string toString() const;
	/** Get the id of the Train */
	inline int GetID() const { return id; }
	/** Set the id (if the id is yet not set (-1)) */
	void SetID(int id) { if(this->id==-1) this->id = id; }
	/** Serialize this Train to a protobuf object */
	void Serialize(PBTrainUnit* pb_t) const;
};

//!\cond NO_DOC
struct TrainHash {
	std::size_t operator()(const Train* const & k) const {
		return std::hash<int>()(k->GetID());
	}
};

struct TrainEquals {
	bool operator()(const Train* const & lhs, const Train* const & rhs) const {
		return lhs->GetID() == rhs->GetID();
	}
};

STREAM_OPERATOR(Train);
STREAM_OPERATOR(Task);
STREAM_OPERATOR(TrainUnitType);

//!\endcond



#endif