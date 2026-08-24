#include "Scenario.h"

Scenario::Scenario() : startTime(0), endTime(0) {}

Scenario::Scenario(string scenarioFileString, const Location &location)
{
	PB_HIP_Scenario pb_scenario;
	parse_json_to_pb(fs::path(scenarioFileString), &pb_scenario);

	if (!pb_scenario.IsInitialized() || pb_scenario.ByteSizeLong() == 0)
	{
		std::cerr << "Protobuf is empty or not initialized." << std::endl;
	}
	else
	{
		std::cout << "Protobuf has been initialized and may contain data." << std::endl;
	}
	Init(pb_scenario, location);


}

Scenario::Scenario(const PBScenario &pb_scenario, const Location &location)
{
	Init(pb_scenario, location);
}

Scenario::Scenario(const PB_HIP_Scenario &pb_scenario, const Location &location)
{
	Init(pb_scenario, location);
}

void Scenario::Init(const PBScenario &pb_scenario, const Location &location)
{
	try
	{
		ImportEmployees(pb_scenario, location);
		ImportShuntingUnits(pb_scenario, location);
		startTime = pb_scenario.starttime();
		endTime = pb_scenario.endtime();
	}
	catch (exception &e)
	{
		cout << "Error in loading scenario: " << e.what() << "\n";
		// Rethrow the original exception (preserving its dynamic type and message).
		// `throw e;` here would slice it down to a plain std::exception, losing
		// both the message and the type callers (e.g. main.cpp) catch on.
		throw;
	}
}

void Scenario::Init(const PB_HIP_Scenario &pb_scenario, const Location &location)
{
	try
	{
		ImportEmployees(pb_scenario, location);
		ImportShuntingUnits(pb_scenario, location);
		ImportAncillary(pb_scenario);
		startTime = pb_scenario.starttime();
		endTime = pb_scenario.endtime();
	}
	catch (exception &e)
	{
		cout << "Error in loading scenario: " << e.what() << "\n";
		// See comment in the other Init() overload above: rethrow, don't slice.
		throw;
	}
}

Scenario::Scenario(const Scenario &scenario) : startTime(scenario.startTime), endTime(scenario.endTime)
{
	disturbances = scenario.disturbances;
	for (auto inc : scenario.incomingTrains)
		incomingTrains.push_back(new Incoming(*inc));
	for (auto out : scenario.outgoingTrains)
		outgoingTrains.push_back(new Outgoing(*out));
	for (auto e : scenario.employees)
		employees.push_back(new Employee(*e));
	// TODO disturbances
	// TODO tasks
	for (auto out: scenario.outgoingTrains)
		track_exiting_trains[out->GetID()] = false;
}

Scenario::~Scenario()
{
	DELETE_VECTOR(incomingTrains)
	DELETE_VECTOR(outgoingTrains)
	DELETE_VECTOR(employees)
	// TODO  disturbances
	// TODO tasks
}

const size_t Scenario::GetNumberOfTrains() const
{
	size_t res = 0;
	for (auto inc : incomingTrains)
		res += inc->GetShuntingUnit()->GetNumberOfTrains();
	return res;
}

const Train *Scenario::GetTrainByID(int id) const
{
	for (auto inc : incomingTrains)
	{
		for (auto &t : inc->GetShuntingUnit()->GetTrains())
		{
			if (t.GetID() == id)
				return &t;
		}
	}
	return nullptr;
}

const Incoming *Scenario::GetIncomingBySU(const ShuntingUnit *su) const
{
	for (auto inc : incomingTrains)
	{
		if (inc->GetShuntingUnit()->MatchesShuntingUnit(su))
			return inc;
	}
	return nullptr;
}

const Outgoing *Scenario::GetOutgoingBySU(const ShuntingUnit *su) const
{
	for (auto out : outgoingTrains)
	{
		if (out->GetShuntingUnit()->MatchesShuntingUnit(su))
			return out;
	}
	return nullptr;
}

const Incoming *Scenario::GetIncomingByTrainID(int id) const
{
	for (auto inc : incomingTrains)
	{
		if (inc->GetShuntingUnit()->GetTrainByID(id) != nullptr)
			return inc;
	}
	return nullptr;
}

const Outgoing *Scenario::GetOutgoingByTrainID(int id) const
{
	for (auto out : outgoingTrains)
	{
		if (out->GetShuntingUnit()->GetTrainByID(id) != nullptr)
			return out;
	}
	return nullptr;
}

void Scenario::ImportEmployees(const PBScenario &pb_scenario, const Location &location)
{
	for (auto &pb_e : pb_scenario.workers())
	{
		Employee *e = new Employee(pb_e);
		string start = to_string(pb_e.startlocationid());
		string end = to_string(pb_e.endlocationid());
		e->AssignTracks(location.GetTrackByID(start), location.GetTrackByID(end));
		employees.push_back(e);
		debug_out("Imported Employee " << e->toString());
	}
	debug_out("finished loading employees from JSON");
}

void Scenario::ImportEmployees(const PB_HIP_Scenario &pb_scenario, const Location &location)
{
	for (auto &pb_e : pb_scenario.workers())
	{
		Employee *e = new Employee(pb_e);
		string start = to_string(pb_e.startlocationid());
		string end = to_string(pb_e.endlocationid());
		e->AssignTracks(location.GetTrackByID(start), location.GetTrackByID(end));
		employees.push_back(e);
		debug_out("Imported Employee " << e->toString());
	}
	debug_out("finished loading employees from JSON");
}

template <class PBTrainGoal>
TrainGoal *ImportTrainGoal(const Location &location, const PBTrainGoal &m, bool in, bool standing)
{

	TrainGoal *g = in ? static_cast<TrainGoal *>(new Incoming(m, standing)) : new Outgoing(m, standing);
	string park = to_string(m.parkingtrackpart());
	string side = to_string(m.sidetrackpart());
	g->assignTracks(location.GetTrackByID(park), location.GetTrackByID(side));
	return g;
}

Incoming *ImportIncomingTrainGoal(const Location &location, const PB_HIP_TrainGoal &m, bool standing)
{
	Incoming *g = new Incoming(m, standing);
	string park = to_string(m.firstparkingtrackpart());
	string side = to_string(m.entrytrackpart());
	g->assignTracks(location.GetTrackByID(park), location.GetTrackByID(side));
	return g;
}

Outgoing *ImportOutgoingTrainGoal(const Location &location, const PB_HIP_TrainRequest &m, bool standing)
{
	Outgoing *g = new Outgoing(m, standing);
	string park = to_string(m.lastparkingtrackpart());
	string side = to_string(m.leavetrackpart());
	g->assignTracks(location.GetTrackByID(park), location.GetTrackByID(side));
	return g;
}

void Scenario::ImportShuntingUnits(const PBScenario &pb_scenario, const Location &location)
{
	for (auto &pb_train_type : pb_scenario.trainunittypes())
	{
		if (TrainUnitType::types.find(pb_train_type.displayname()) == TrainUnitType::types.end())
		{
			TrainUnitType *tt = new TrainUnitType(pb_train_type);
			TrainUnitType::types[tt->displayName] = tt;
		}
	}

	for (auto &pb_in : pb_scenario.in())
	{
		incomingTrains.push_back(dynamic_cast<Incoming *>(ImportTrainGoal(location, pb_in, true, false)));
	}

	for (auto &pb_in : pb_scenario.instanding())
		incomingTrains.push_back(dynamic_cast<Incoming *>(ImportTrainGoal(location, pb_in, true, true)));

	for (auto &pb_out : pb_scenario.out())
		outgoingTrains.push_back(dynamic_cast<Outgoing *>(ImportTrainGoal(location, pb_out, false, false)));

	for (auto &pb_out : pb_scenario.outstanding())
		outgoingTrains.push_back(dynamic_cast<Outgoing *>(ImportTrainGoal(location, pb_out, false, true)));

	debug_out("finished loading ShuntingUnits from JSON");

	for (auto out: outgoingTrains)
		track_exiting_trains[out->GetID()] = false;
}

void Scenario::ImportShuntingUnits(const PB_HIP_Scenario &pb_scenario, const Location &location)
{
	for (auto &pb_train_type : pb_scenario.trainunittypes())
	{
		auto key = make_pair(pb_train_type.typeprefix(), (int)pb_train_type.carriages());
		if (TrainUnitType::typesByPrefixAndCarriages.find(key) == TrainUnitType::typesByPrefixAndCarriages.end())
		{
			TrainUnitType *tt = new TrainUnitType(pb_train_type);
			TrainUnitType::typesByPrefixAndCarriages[key] = tt;
		}
	}

	for (auto &pb_in : pb_scenario.in())
		incomingTrains.push_back(ImportIncomingTrainGoal(location, pb_in, false));

	for (auto &pb_in : pb_scenario.instanding())
		incomingTrains.push_back(ImportIncomingTrainGoal(location, pb_in, true));

	for (auto &pb_out : pb_scenario.out())
		outgoingTrains.push_back(ImportOutgoingTrainGoal(location, pb_out, false));

	for (auto &pb_out : pb_scenario.outstanding())
		outgoingTrains.push_back(ImportOutgoingTrainGoal(location, pb_out, true));

	debug_out("finished loading ShuntingUnits from JSON");

	for (auto out: outgoingTrains)
		track_exiting_trains[out->GetID()] = false;
}

void Scenario::ImportAncillary(const PB_HIP_Scenario &pb_scenario)
{
	for (auto &pb_nst : pb_scenario.nonservicetraffic())
	{
		NonServiceTrafficEntry entry;
		entry.members = vector<int>(pb_nst.memberids().begin(), pb_nst.memberids().end());
		entry.arrival = pb_nst.arrival();
		entry.departure = pb_nst.departure();
		entry.id = pb_nst.id();
		nonServiceTraffic.push_back(entry);
	}

	for (auto &pb_dtp : pb_scenario.disabledtrackparts())
	{
		DisabledTrackPartEntry entry;
		entry.trackPart = pb_dtp.trackpart();
		entry.arrival = pb_dtp.arrival();
		entry.departure = pb_dtp.departure();
		disabledTrackParts.push_back(entry);
	}
}

void Scenario::Serialize(PBScenario *pb_scenario) const
{
	for (auto inc : incomingTrains)
	{
		inc->Serialize(
			inc->IsInstanding() ? pb_scenario->add_instanding() : pb_scenario->add_in());
	}
	for (auto out : outgoingTrains)
	{
		out->Serialize(
			out->IsInstanding() ? pb_scenario->add_outstanding() : pb_scenario->add_out());
	}
	pb_scenario->set_starttime(GetStartTime());
	pb_scenario->set_endtime(GetEndTime());
	for (auto &[name, type] : TrainUnitType::types)
	{
		type->Serialize(pb_scenario->add_trainunittypes());
	}
}

void Scenario::PrintScenarioInfo() const
{
	cout << "|---------------------------|" << endl;
	cout << "|   Scenario                |" << endl;
	cout << "|---------------------------|" << endl;
	cout << "Start time: " << startTime << ", end time: " << endTime << endl
		 << endl;
	if (incomingTrains.size() == 0)
		cout << "No arrivals" << endl
			 << endl;
	else
	{
		cout << "Arrivals:" << endl;
		for (auto inc : incomingTrains)
		{
			cout << "\tT" << inc->GetTime() << ": \t" << inc->GetShuntingUnit() << " (" << inc->GetShuntingUnit()->GetTrainString() << ") at "
				 << inc->GetParkingTrack() << " from " << inc->GetSideTrack();
			if (inc->IsInstanding())
				cout << " (instanding)";
			cout << endl;
		}
		cout << endl;
	}

	if (outgoingTrains.size() == 0)
		cout << "No departures" << endl
			 << endl;
	else
	{
		cout << "Departures:" << endl;
		for (auto out : outgoingTrains)
		{
			cout << "\tT" << out->GetTime() << ": \t" << out->GetShuntingUnit() << " (" << out->GetShuntingUnit()->GetTrainString() << ") at "
				 << out->GetParkingTrack() << " to " << out->GetSideTrack();
			if (out->IsInstanding())
				cout << " (outstanding ix-" << to_string(out->GetStandingIndex()) << ")";
			cout << endl;
		}
		cout << endl;
	}
}

void Scenario::CheckScenarioCorrectness(const Location &location) const
{
	vector<Track *> locationTracks = location.GetTracks();

	// Train types are keyed by (displayName, carriages) - displayName alone is not
	// unique, since two carriage variants of one family (e.g. "SLT") can coexist.
	// Contains the number of (incoming) trains per train type e.g., (SLT,4) : 7; (SLT,6) : 9
	map<pair<string,int>, int> incomingTrainTypes;
	// Contains the number of (outgoing) trains per train type e.g., (SLT,4) : 7; (SLT,6) : 9
	map<pair<string,int>, int> outgoingTrainTypes;

	// Contains the number of (inStanding) trains per train type e.g., (SLT,4) : 7; (SLT,6) : 9
	map<pair<string,int>, int> inStandingTrainTypes;
	// Contains the number of (outStanding) trains per train type e.g., (SLT,4) : 7; (SLT,6) : 9
	map<pair<string,int>, int> outStandingTrainTypes;

	int totalTaskTime = 0;
	for (const Incoming *train : incomingTrains)
	{
		// Test if arrival train length does not exceed the length of the track (RailRoad) it arrives
		double shuntingUnitLength = 0.0;
		double arrivalTrackLength = 0.0;
		Track *arrivalTrack;

		const ShuntingUnit *shuntingUnit = train->GetShuntingUnit();
		shuntingUnitLength = shuntingUnit->GetLength();
		arrivalTrack = location.GetTrackByID(train->GetParkingTrack()->GetID());

		// Type is railroad ? - if not then the train cannot arrive to this infrastructure
		if (arrivalTrack->GetType() != TrackPartType::Railroad)
		{
			throw invalid_argument("The arrival track [" + arrivalTrack->GetID() + "]: type is " + to_string(arrivalTrack->GetType()) + " and it should be RailRoad");
		}

		arrivalTrackLength = arrivalTrack->GetLength();

		if (shuntingUnitLength > arrivalTrackLength)
		{
			throw invalid_argument("The length of the arrival train [" + to_string(train->GetID()) + "]: " + to_string(shuntingUnitLength) + " is greater than the length of the track's [" + arrivalTrack->GetID() + "] it arrives:" + to_string(arrivalTrackLength));
		}

		if (!train->IsInstanding())
		{
			// Calculates the number of arrival train types, this will be compared with the number of departing train types
			for (Train trainUnit : shuntingUnit->GetTrains())
			{
				auto trainType = make_pair(trainUnit.GetType()->displayName, trainUnit.GetType()->carriages);
				if (incomingTrainTypes.find(trainType) != incomingTrainTypes.end())
					incomingTrainTypes[trainType] = 0;
				incomingTrainTypes[trainType] += 1;
			}
		}
		else
		{
			// Calculates the train types which were already on the shunting yard, this will be compared with the number of departing train types
			for (Train trainUnit : shuntingUnit->GetTrains())
			{
				auto trainType = make_pair(trainUnit.GetType()->displayName, trainUnit.GetType()->carriages);
				if (inStandingTrainTypes.find(trainType) != inStandingTrainTypes.end())
					inStandingTrainTypes[trainType] = 0;

				inStandingTrainTypes[trainType] += 1;
			}
		}

		// Get the tasks per train, a train here reflects to a train unit within the shunting unit
		const unordered_map<const Train *, vector<Task>, TrainHash, TrainEquals> trainTasks = train->GetTasks();

		for (const auto &[trainPtr, tasks] : trainTasks)
		{
			for (Task task : tasks)
			{
				totalTaskTime = totalTaskTime + task.duration;
			}
		}
	}

	if (totalTaskTime > GetEndTime())
	{
		throw invalid_argument("The total time to finish all the tasks is [" + to_string(totalTaskTime) + "] > than the end time of the scenario [" + to_string(GetEndTime()) + "]");

		// cout << "Total Task time : " << totalTaskTime << endl;
	}
	cout << "Arrival trains fit to the arrival tracks" << endl;

	for (const Outgoing *train : outgoingTrains)
	{
		// Test if departure train length does not exceed the length of the track (RailRoad) it arrives
		double shuntingUnitLength = 0.0;
		double departureTrackLength = 0.0;
		Track *departureTrack;

		const ShuntingUnit *shuntingUnit = train->GetShuntingUnit();
		shuntingUnitLength = shuntingUnit->GetLength();
		departureTrack = location.GetTrackByID(train->GetParkingTrack()->GetID());
		departureTrackLength = departureTrack->GetLength();

		// Type is railroad ? - if not the then the train cannot depart from this infrastructure
		if (departureTrack->GetType() != TrackPartType::Railroad)
		{
			throw invalid_argument("The departure track [" + departureTrack->GetID() + "]: type is " + to_string(departureTrack->GetType()) + " and it should be RailRoad");
		}

		if (shuntingUnitLength > departureTrackLength)
		{
			throw invalid_argument("The length of the departure train [" + to_string(train->GetID()) + "]: " + to_string(shuntingUnitLength) + " is greater than the length of the track's [" + departureTrack->GetID() + "] it arrives:" + to_string(departureTrackLength));
		}

		unordered_map<const Train *, string, TrainHash, TrainEquals> trainsTypes;
		if (!train->IsInstanding())
		{
			// Calculates the number of departing train types, this will be compared with the number of arriving train
			// types and instanding train types
			for (Train trainUnit : shuntingUnit->GetTrains())
			{
				string trainType = trainUnit.GetType()->displayName;
				auto typeKey = make_pair(trainType, trainUnit.GetType()->carriages);
				if (outgoingTrainTypes.find(typeKey) != outgoingTrainTypes.end())
					outgoingTrainTypes[typeKey] = 0;

				outgoingTrainTypes[typeKey] += 1;

				trainsTypes[&trainUnit] = trainType;
			}
		}
		else
		{

			// Calculates the train types which will stay on the shunting yard after the end of the scenarion, this will be compared with the number of departing train types
			for (Train trainUnit : shuntingUnit->GetTrains())
			{

				string trainType = trainUnit.GetType()->displayName;
				auto typeKey = make_pair(trainType, trainUnit.GetType()->carriages);
				if (outStandingTrainTypes.find(typeKey) != outStandingTrainTypes.end())
					outStandingTrainTypes[typeKey] = 0;

				outStandingTrainTypes[typeKey] += 1;

				trainsTypes[&trainUnit] = trainType;

			}
			cout << "Detected outStanding Trains" << endl;

		}

		// Cheks if the shunting unit contains trains of the same type (SLT, SNG, etc)
		// It is not possible to combine trains of different types
		auto firstTrainUnit = trainsTypes.begin();
		string typeFirstTrainUnitLong = firstTrainUnit->second;
		size_t position = typeFirstTrainUnitLong.find('-');						// find position to remove part with "-", e.g., from SLT-6 only SLT part is needed
		string typeFirstTrainUnit = typeFirstTrainUnitLong.substr(0, position); // removes "-X" part

		for (auto it = next(trainsTypes.begin()); it != trainsTypes.end(); it++)
		{
			string typeCurrentTrainUnitLong = it->second;
			size_t position = typeCurrentTrainUnitLong.find("-"); // removes "-X" part
			string typeCurrentTrainUnit = typeCurrentTrainUnitLong.substr(0, position);

			// If one of the train unit type is different from another, then the train combination cannot be done
			if (typeCurrentTrainUnit != typeFirstTrainUnit)
			{
				throw invalid_argument("Train units with different types cannot be combined: Train unit [" + typeFirstTrainUnit + "] is not copatible with Train unit [" + typeCurrentTrainUnit + "]");
			}
		}
	}
	cout << "Departure trains fit to the departure tracks" << endl;

	cout << "Train combination per type is correct" << endl;

	for (const auto &[trainType, count] : incomingTrainTypes)
	{
		string trainTypeLabel = trainType.first + "-" + to_string(trainType.second);
		if (outgoingTrainTypes[trainType] > count + inStandingTrainTypes[trainType] - outStandingTrainTypes[trainType])
		{
			throw invalid_argument("The number of departure trains of type: [" + trainTypeLabel + "] : " + to_string(outgoingTrainTypes[trainType]) + "does not match the number of arrived trains of type[" + trainTypeLabel + "] : " + to_string(count) + "plus the number of instanding trains of type [" + trainTypeLabel + "] : " + to_string(inStandingTrainTypes[trainType]) + "or the required number of outstanding trains of type [" + trainTypeLabel + "] : " + to_string(outStandingTrainTypes[trainType]) + "is too high compared to the instanding, incoming and outgoing trains");
		}
	}

	cout << "Departure train types correspond to the Arrival and Instanding train types" << endl;
}