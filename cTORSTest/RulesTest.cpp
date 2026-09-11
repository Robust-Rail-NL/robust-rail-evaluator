#include "doctest/doctest.h"
#include "Engine.h"
#include "BusinessRules.h"

namespace cTORSTest
{
	TEST_CASE("Rules Test") {
		Scenario scenario;
		
		/* Set up a small track
		b0==r0===s0==r1=======b1
		           \=r2=============b2
		*/
		Track r0("r0", TrackPartType::Railroad, 100, "rail0", false, false, false);
		Track r1("r1", TrackPartType::Railroad, 200, "rail1", false, false, true);
		Track r2("r2", TrackPartType::Railroad, 300, "rail2", false, false, true);
		Track s0("s0", TrackPartType::Switch, 50, "switch0", false, false, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		Track b2("b2", TrackPartType::Bumper, 10, "bumper2", false, false, true);
		r0.AssignNeighbors({&b0}, {&s0});
		r1.AssignNeighbors({&b1}, {&s0});
		r2.AssignNeighbors({&b2}, {&s0});
		s0.AssignNeighbors({&r0}, {&r1,&r2});
		b0.AssignNeighbors({&r0},{});
		b1.AssignNeighbors({&r1},{});
		b2.AssignNeighbors({&r2},{});

		vector<Track*> tracks = {&r0, &r1, &r2, &s0, &b0, &b1, &b2};
		State state(scenario, tracks);

		//Setup a two test trains
		TrainUnitType elecTrainType("ElecTrainType", 1, 100, 100, 100, 100, 100, 50, 100, "ETT", false, false, true);
		Train elecTrain = Train(0, &elecTrainType);
		ShuntingUnit* elecSU = new ShuntingUnit(0, {elecTrain});

		TrainUnitType nonElecTrainType("NonElecTrainType", 1, 100, 100, 100, 100, 100, 50, 100, "NETT", false, false, false);
		Train nonElecTrain = Train(0, &nonElecTrainType);
		ShuntingUnit* nonElecSU = new ShuntingUnit(1, {nonElecTrain});
		
		state.AddShuntingUnit(elecSU, &r1, &b1);
		state.AddShuntingUnit(nonElecSU, &r0, &b0);

		vector<const Track*> elecMove = {&r1, &s0, &r0};
		vector<const Track*> nonElecMove = {&r0, &s0, &r2};
		MoveAction elecMoveAction(elecSU, elecMove, 0, false);
		MoveAction nonElecMoveAction(nonElecSU, nonElecMove, 0, false);

		Config config;
		electric_move_rule emr(&config);
		CHECK(!emr.IsValid(&state, &elecMoveAction).first);
		CHECK(emr.IsValid(&state, &nonElecMoveAction).first);
		length_track_rule ltr(&config);
		CHECK(!ltr.IsValid(&state, &elecMoveAction).first);
		CHECK(ltr.IsValid(&state, &nonElecMoveAction).first);
		cout << " Executed all tests "  << endl;
	}

	TEST_CASE("Service task rules test") {
		Scenario scenario;

		Track r0("r0", TrackPartType::Railroad, 200, "rail0", false, false, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		r0.AssignNeighbors({&b0}, {&b1});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&r0}, {});

		vector<Track*> tracks = {&r0, &b0, &b1};
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train train(1, &testType);
		ShuntingUnit su(1, {train});
		state.AddShuntingUnit(&su, &r0, &b0);

		const ShuntingUnit* stateSU = state.GetShuntingUnitByID(1);
		const Train* stateTrain = stateSU->GetTrainByID(1);
		// Outgoing owns (and deletes) its ShuntingUnit, so it needs its own copy,
		// distinct from stateSU which is owned by state.
		Outgoing outgoing(1, new ShuntingUnit(1, {train}), &r0, &b0, 0, false, 0);
		ExitAction exitAction(stateSU, 0, &outgoing);

		Config config;
		mandatory_service_task_rule mstr(&config);
		optional_service_task_rule ostr(&config);

		SUBCASE("no unfinished tasks") {
			CHECK(mstr.IsValid(&state, &exitAction).first);
			CHECK(ostr.IsValid(&state, &exitAction).first);
		}

		SUBCASE("unfinished mandatory task blocks the mandatory rule only") {
			state.AddTaskToTrain(stateTrain, Task("clean", false, 100, {}));
			CHECK(!mstr.IsValid(&state, &exitAction).first);
			CHECK(ostr.IsValid(&state, &exitAction).first);
		}

		SUBCASE("unfinished optional task blocks the optional rule only") {
			state.AddTaskToTrain(stateTrain, Task("clean", true, 100, {}));
			CHECK(mstr.IsValid(&state, &exitAction).first);
			CHECK(!ostr.IsValid(&state, &exitAction).first);
		}
	}

	TEST_CASE("A wait replayed from a plan lasts as long as the plan says") {
		// Waiting until the next event is the only sensible bound while searching:
		// there is no plan to consult, and the search decides again at every event.
		// Replaying a plan is different — the plan states the duration — and taking
		// the next event instead consumed the time the plan had reserved for what
		// followed, pushing the rest of that unit's schedule late by exactly that
		// much.
		Scenario scenario;
		scenario.SetEndTime(7200);

		Track r0("r0", TrackPartType::Railroad, 200, "rail0", false, true, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		r0.AssignNeighbors({&b0}, {&b1});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&r0}, {});

		vector<Track*> tracks = {&r0, &b0, &b1};
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train train(1, &testType);
		ShuntingUnit su(1, {train});
		state.AddShuntingUnit(&su, &r0, &b0);

		json params = json::object();
		WaitActionGenerator generator(params, nullptr);

		SUBCASE("the plan's duration wins over the distance to the next event") {
			// An outgoing event further away than the plan's wait: taking the event
			// would overshoot, which is the bug.
			Outgoing outgoing(1, new ShuntingUnit(1, {train}), &r0, &b0, 3600, false, 0);
			state.AddEvent(&outgoing);

			Wait planned({1}, 900);
			const Action* action = generator.Generate(&state, planned);
			CHECK(action->GetDuration() == 900);
			delete action;
		}

		SUBCASE("a wait with no duration still runs to the next event") {
			Outgoing outgoing(1, new ShuntingUnit(1, {train}), &r0, &b0, 3600, false, 0);
			state.AddEvent(&outgoing);

			Wait unplanned({1});
			const Action* action = generator.Generate(&state, unplanned);
			CHECK(action->GetDuration() == 3600);
			delete action;
		}

		SUBCASE("a planned wait needs no event to wait for") {
			// A unit that simply stays put until the scenario ends has no next event,
			// which used to make the wait impossible to express at all.
			Wait planned({1}, 600);
			const Action* action = generator.Generate(&state, planned);
			CHECK(action->GetDuration() == 600);
			delete action;
		}

		SUBCASE("a wait with no duration and no event is still an error") {
			Wait unplanned({1});
			CHECK_THROWS_AS(generator.Generate(&state, unplanned), InvalidActionException);
		}
	}

	TEST_CASE("An arrive replayed from a plan spans the gap the plan gives it (solver#13)") {
		// A train that reaches its arrival track before its route into the yard is
		// free needs to occupy that track for a while. Giving the Arrive itself
		// that duration - instead of following it with a Wait - keeps a train that
		// arrives on a non-parking track (e.g. a gateway) off
		// legal_on_parking_track_rule, which Arrive is exempt from but Wait isn't.
		Location location(TORS_DATA_DIR "/scenario_unification_test", true);
		Scenario scenario(TORS_DATA_DIR "/scenario_unification_test/scenario.json", location);
		State state(scenario, location.GetTracks());

		REQUIRE(scenario.GetIncomingTrains().size() == 1);
		auto incoming = scenario.GetIncomingTrains().front();

		json params = json::object();
		ArriveActionGenerator generator(params, &location);

		SUBCASE("a planned arrive occupies the track for the plan's duration") {
			Arrive planned(incoming, 50);
			const Action* action = generator.Generate(&state, planned);
			CHECK(action->GetDuration() == 50);
			delete action;
		}

		SUBCASE("an unplanned arrive is instantaneous") {
			Arrive unplanned(incoming);
			const Action* action = generator.Generate(&state, unplanned);
			CHECK(action->GetDuration() == 0);
			delete action;
		}

		SUBCASE("a nonzero-duration arrive keeps the active-action flag set, which is what stops ApplyWaitAllUntil from synthesizing a Wait for it") {
			Arrive planned(incoming, 50);
			const Action* action = generator.Generate(&state, planned);
			auto su = action->GetShuntingUnit();

			state.StartAction(action);
			CHECK(state.HasActiveAction(su));

			state.FinishAction(action);
			CHECK(!state.HasActiveAction(su));
		}
	}

	TEST_CASE("Parking is about standing still, not about passing through") {
		// A gateway forbids parking because it is the connection to the main line,
		// but every departure has to move onto it before leaving. Rejecting a
		// movement because its destination forbids parking therefore made departure
		// impossible on any such location, and so no plan could ever be valid.
		Scenario scenario;

		// b0 == y0 (parking) == g0 (gateway, no parking) == b1
		Track y0("y0", TrackPartType::Railroad, 200, "yard0", false, true, true);
		Track g0("g0", TrackPartType::Railroad, 200, "gateway", false, false, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		y0.AssignNeighbors({&b0}, {&g0});
		g0.AssignNeighbors({&y0}, {&b1});
		b0.AssignNeighbors({&y0}, {});
		b1.AssignNeighbors({&g0}, {});

		vector<Track*> tracks = {&y0, &g0, &b0, &b1};
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train train(1, &testType);
		ShuntingUnit su(1, {train});
		state.AddShuntingUnit(&su, &y0, &b0);

		Config config;
		legal_on_parking_track_rule lptr(&config);

		SUBCASE("a movement onto a non-parking track is allowed") {
			vector<const Track*> toGateway = {&y0, &g0};
			// stepMove=false: this is how a movement replayed from a plan is built.
			MoveAction departureMove(&su, toGateway, 0, false);
			CHECK(lptr.IsValid(&state, &departureMove).first);
		}

		SUBCASE("waiting on a non-parking track is still rejected") {
			// The unit stops on the gateway rather than passing over it, which is
			// what the rule is actually for.
			state.RemoveShuntingUnit(&su);
			state.AddShuntingUnit(&su, &g0, &y0);
			WaitAction waitOnGateway(state.GetShuntingUnitByID(1), 60);
			CHECK(!lptr.IsValid(&state, &waitOnGateway).first);
		}

		SUBCASE("waiting on a parking track is allowed") {
			WaitAction waitInYard(state.GetShuntingUnitByID(1), 60);
			CHECK(lptr.IsValid(&state, &waitInYard).first);
		}
	}

	TEST_CASE("An outStanding unit exits at the end of the scenario") {
		// An outStanding request describes a unit that stays in the yard once the
		// scenario ends, so it has no departure time of its own and its Outgoing
		// carries 0. Demanding the clock equal that contradicted the exit generator,
		// which only offers the exit once the scenario has ended, so an outStanding
		// unit could never exit validly.
		Scenario scenario;
		scenario.SetEndTime(3600);

		Track r0("r0", TrackPartType::Railroad, 200, "rail0", false, true, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		r0.AssignNeighbors({&b0}, {&b1});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&r0}, {});

		vector<Track*> tracks = {&r0, &b0, &b1};
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train train(1, &testType);
		ShuntingUnit su(1, {train});
		state.AddShuntingUnit(&su, &r0, &b0);
		const ShuntingUnit* stateSU = state.GetShuntingUnitByID(1);

		Config config;
		out_correct_time_rule octr(&config);

		SUBCASE("outStanding: valid once the scenario has ended") {
			Outgoing outstanding(1, new ShuntingUnit(1, {train}), &r0, &b0, 0, true, 0);
			ExitAction exitAction(stateSU, 0, &outstanding);
			state.SetTime(3600);
			CHECK(octr.IsValid(&state, &exitAction).first);
		}

		SUBCASE("outStanding: not before the scenario has ended") {
			Outgoing outstanding(1, new ShuntingUnit(1, {train}), &r0, &b0, 0, true, 0);
			ExitAction exitAction(stateSU, 0, &outstanding);
			state.SetTime(1800);
			CHECK(!octr.IsValid(&state, &exitAction).first);
		}

		SUBCASE("departing: still has to leave at exactly its own time") {
			Outgoing departing(2, new ShuntingUnit(1, {train}), &r0, &b0, 1800, false, 0);
			ExitAction exitAction(stateSU, 0, &departing);
			state.SetTime(1800);
			CHECK(octr.IsValid(&state, &exitAction).first);
			state.SetTime(1860);
			CHECK(!octr.IsValid(&state, &exitAction).first);
		}
	}
}
