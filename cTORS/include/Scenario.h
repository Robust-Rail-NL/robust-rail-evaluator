/** \file Scenario.h
 * Describes the Scenario class
 */
#pragma once
#ifndef SCENARIO_H
#define SCENARIO_H
#include "Event.h"
#include "Location.h"
namespace fs = std::filesystem;
using namespace std;

/**
 * Traffic on the shunting yard that isn't a shunting movement (e.g. a passing
 * service train) but still reserves part of the infrastructure for a time window.
 * Parsed and stored, but not currently consumed by any rule/engine logic.
 */
struct NonServiceTrafficEntry {
	vector<int> members;	/**< The TrackPart ids reserved by this traffic */
	int arrival;			/**< Arrival time, in seconds since the epoch */
	int departure;			/**< Departure time, in seconds since the epoch */
	string id;				/**< The unique id of this entry */
};

/**
 * A TrackPart that is unavailable for use during a time window (e.g. for maintenance).
 * Parsed and stored, but not currently consumed by any rule/engine logic.
 */
struct DisabledTrackPartEntry {
	int trackPart;	/**< The TrackPart id that is unavailable */
	int arrival;	/**< Start of the unavailability window, in seconds since the epoch */
	int departure;	/**< End of the unavailability window, in seconds since the epoch */
};

/**
 * The Scenario class describes a session by its Incoming trains, Outgoing trains
 * Employee%s and Disturbance%s. An initial State is generated from a Scenario object.
 */
class Scenario
{
private:
	int startTime, endTime;
	EventQueue disturbances;
	vector<const Employee*> employees;
	vector<const Incoming*> incomingTrains;
	vector<const Outgoing*> outgoingTrains;
	vector<const Event*> disturbanceList;
	vector<NonServiceTrafficEntry> nonServiceTraffic;
	vector<DisabledTrackPartEntry> disabledTrackParts;

	void ImportEmployees(const PBScenario& pb_scenario, const Location& location);
	void ImportShuntingUnits(const PBScenario& pb_scenario, const Location& location);
	void Init(const PBScenario& pb_scenario, const Location& location);

	void ImportEmployees(const PB_HIP_Scenario& pb_scenario, const Location& location);
	void ImportShuntingUnits(const PB_HIP_Scenario& pb_scenario, const Location& location);
	void ImportAncillary(const PB_HIP_Scenario& pb_scenario);
	void Init(const PB_HIP_Scenario& pb_scenario, const Location& location);
public:
	/** Generate an empty scenario */
	Scenario();
	/** Generate a Scenario according to the (HIP-shaped, unified) JSON file on the given path for the provided Location */
	Scenario(string path, const Location& location);
	/** Generate a Scenario according to the legacy (non-HIP) protobuf object for the provided Location.
	 * Used only for TORS's internal Run round-trip format (see RunResult::CreateRunResult(const Location*, const PBRun&)) */
	Scenario(const PBScenario& pb_scenario, const Location& location);
	/** Generate a Scenario according to the HIP protobuf object for the provided Location */
	Scenario(const PB_HIP_Scenario& pb_scenario, const Location& location);
	/** Copy constructor */
	Scenario(const Scenario& scenario);
	/** Scenario destructor */
	~Scenario();
	/** Get the NonServiceTraffic entries in this Scenario (parsed but not currently acted on) */
	inline const vector<NonServiceTrafficEntry>& GetNonServiceTraffic() const { return nonServiceTraffic; }
	/** Get the DisabledTrackPart entries in this Scenario (parsed but not currently acted on) */
	inline const vector<DisabledTrackPartEntry>& GetDisabledTrackParts() const { return disabledTrackParts; }

	/** Print the Scenario Info */
	void PrintScenarioInfo() const;	
	/** Get the start time of this Scenario */
	inline int GetStartTime() const { return startTime; }
	/** Get the end time of this Scenario */
	inline int GetEndTime() const { return endTime; }
	/** Get the disturbances in this Scenario as an EventQueue */
	inline const EventQueue& GetDisturbances() const { return disturbances; }
	/** Get the disturbances in this Scenario as a vector*/
	inline const vector<const Event*>& GetDisturbanceVector() const { return disturbanceList; }
	/** Get all the Outgoing trains */
	inline const vector<const Outgoing*>& GetOutgoingTrains() const { return outgoingTrains; }
	/** Get all the Incoming trains */
	inline const vector<const Incoming*>& GetIncomingTrains() const { return incomingTrains; }
	/** Get all the employees (not yet implemented) */
	inline const vector<const Employee*>& GetEmployees() const { return employees; }
	/** Get the number of Train%s in this Scenario */
	const size_t GetNumberOfTrains() const;
	/** Get a Train by its id from this Scenario */
	const Train* GetTrainByID(int id) const;
	/** Get an Incoming train by its ShuntingUnit */
	const Incoming* GetIncomingBySU(const ShuntingUnit* su) const;
	/** Get an Outgoing train by its ShuntingUnit */
	const Outgoing* GetOutgoingBySU(const ShuntingUnit* su) const;
	/** Get an Incoming train by one of its Train%s */
	inline const Incoming* GetIncomingByTrain(const Train* train) const { return GetIncomingByTrainID(train->GetID()); }
	/** Get an Outgoing train by one of its Train%s */
	inline const Outgoing* GetOutgoingByTrain(const Train* train) const { return GetOutgoingByTrainID(train->GetID()); }
	/** Get an Incoming train by its Train ids */
	inline const Incoming* GetIncomingByTrainIDs(const vector<int>& ids) const { return GetIncomingByTrainID(ids.at(0)); }
	/** Get an Outgoing train by its Train ids */
	inline const Outgoing* GetOutgoingByTrainIDs(const vector<int>& ids) const { return GetOutgoingByTrainID(ids.at(0)); }
	/** Get an Incoming train by one of its Train ids */
	const Incoming* GetIncomingByTrainID(int id) const;
	/** Get an Outgoing train by one of its Train ids */
	const Outgoing* GetOutgoingByTrainID(int id) const;
	/** Get all the Task%s for a Train in this Scenario */
	const vector<Task> GetTasksForTrain(const Train* train) const { return GetIncomingByTrain(train)->GetTasks().at(train); }

	/** Set the start time of this scenario */
	inline void SetStartTime(int startTime) { this->startTime = startTime; }
	/** Set the end time of this scenario */
	inline void SetEndTime(int endTime) { this->endTime = endTime; }
	/** Set the list of Outgoing trains in this Scenario */
	inline void SetOutgoingTrains(vector<const Outgoing*> outgoingTrains) { this->outgoingTrains = outgoingTrains; }
	/** Set the list of Incoming trains in this Scenario */
	inline void SetIncomingTrains(vector<const Incoming*> incomingTrains) { this->incomingTrains = incomingTrains; }
	/** Set the list of Employee%s in this Scenario (not yet implemented) */
	inline void SetEmployees(vector<const Employee*> employees) { this->employees = employees; }
	/** Set the queue of Disturbance%s in this Scenario (not yet implemented) */
	inline void SetDisturbances(EventQueue disturbances) { this->disturbances = disturbances; }

	/** Add an Outgoing train to this Scenario */
	inline void AddOutgoingTrain(const Outgoing* outgoingTrain) { outgoingTrains.push_back(outgoingTrain); }
	/** Add an Incoming train to this Scenario */
	inline void AddIncomingTrain(const Incoming* incomingTrain) { incomingTrains.push_back(incomingTrain); }
	/** Add a Disturbance to this Scenario (not yet implemented) */
	inline void AddDisturbance(const Event* disturbance) { disturbances.push(disturbance); }
	/** Add an Employee to this Scenario (not yet implemented) */
	inline void AddEmployee(const Employee* employee) { employees.push_back(employee); }

	/** Serialize this object to a protobuf object */
	void Serialize(PBScenario* pb_scenario) const;

	/** Minimal check of Scenario's correctness: **/
	/** Tests if arrival train length does not exceed the length of the track (RailRoad) it arrives **/
	/** Tests if departure train length does not exceed the length of the track (RailRoad) it departs **/
	/** Tests if the arrival track is a type of RailRoad**/
	/** Tests if the last parking track is a type of RailRoad**/	
	void CheckScenarioCorrectness(const Location& location) const;
	
	mutable map<int, bool> track_exiting_trains;
	inline void UpdateTrackExitingTrains(int trainID, bool visited) const {track_exiting_trains[trainID] = visited;}

	string pathToStoreEval;
	inline void SetEvaluatiorStoragePath(string path) {pathToStoreEval = path;}
	inline const string GetEvaluatiorStoragePath() const {return pathToStoreEval;}

	mutable int departureDelay;
	inline void InitDepartureDelay(int _departureDelay) const {departureDelay = _departureDelay;}
	inline const int GetDepartureDelay() const {return departureDelay;}


};

#endif