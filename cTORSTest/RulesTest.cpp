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

	TEST_CASE("Length rule uses exact millimetre lengths") {
		// 24.1 + 24.1 + 24.1 > 72.3 in double arithmetic (72.30000000000001), but fills the track exactly in decimal
		Scenario scenario;
		Track r0("r0", TrackPartType::Railroad, 100, "rail0", false, false, false);
		Track s0("s0", TrackPartType::Switch, 50, "switch0", false, false, false);
		Track exact("exact", TrackPartType::Railroad, 72.3, "exact", false, false, false);
		Track shortBy1mm("short", TrackPartType::Railroad, 72.299, "short", false, false, false);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, false);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, false);
		Track b2("b2", TrackPartType::Bumper, 10, "bumper2", false, false, false);
		r0.AssignNeighbors({&b0}, {&s0});
		exact.AssignNeighbors({&b1}, {&s0});
		shortBy1mm.AssignNeighbors({&b2}, {&s0});
		s0.AssignNeighbors({&r0}, {&exact, &shortBy1mm});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&exact}, {});
		b2.AssignNeighbors({&shortBy1mm}, {});
		vector<Track*> tracks = {&r0, &s0, &exact, &shortBy1mm, &b0, &b1, &b2};
		State state(scenario, tracks);

		TrainUnitType type("LenType", 1, 24.1, 100, 100, 100, 100, 50, 100, "LT", false, false, false);
		CHECK(type.lengthMm == 24100);
		CHECK(exact.GetLengthMm() == 72300);
		CHECK(shortBy1mm.GetLengthMm() == 72299);
		CHECK(exact.GetLength() == 72.3);

		ShuntingUnit* su0 = new ShuntingUnit(0, {Train(0, &type)});
		ShuntingUnit* su1 = new ShuntingUnit(1, {Train(1, &type)});
		ShuntingUnit* mover = new ShuntingUnit(2, {Train(2, &type)});
		state.AddShuntingUnit(su0, &exact, &b1);
		state.AddShuntingUnit(su1, &exact, &b1);
		state.AddShuntingUnit(mover, &r0, &b0);
		CHECK(mover->GetLengthMm() == 24100);

		Config config;
		length_track_rule ltr(&config);
		MoveAction toExact(mover, {&r0, &s0, &exact}, 0, false);
		CHECK(ltr.IsValid(&state, &toExact).first);

		// Same layout, one millimetre too short
		ShuntingUnit* su3 = new ShuntingUnit(3, {Train(3, &type)});
		ShuntingUnit* su4 = new ShuntingUnit(4, {Train(4, &type)});
		state.AddShuntingUnit(su3, &shortBy1mm, &b2);
		state.AddShuntingUnit(su4, &shortBy1mm, &b2);
		MoveAction toShort(mover, {&r0, &s0, &shortBy1mm}, 0, false);
		auto res = ltr.IsValid(&state, &toShort);
		CHECK(!res.first);
		CHECK(res.second.find("72.300000 > 72.299000") != string::npos);
	}

	TEST_CASE("MetresToMm rounds half away from zero") {
		CHECK(MetresToMm(247.2, "t") == 247200);
		CHECK(MetresToMm(0.0006, "t") == 1);
		CHECK(MetresToMm(247.2004, "t") == 247200);
		CHECK(MetresToMm(0, "t") == 0);
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

	TEST_CASE("Facility capacity counts shunting units, not their members (issue #38)") {
		Scenario scenario;

		Track r0("r0", TrackPartType::Railroad, 1000, "rail0", false, false, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		r0.AssignNeighbors({&b0}, {&b1});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&r0}, {});

		vector<Track*> tracks = {&r0, &b0, &b1};
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train t1(1, &testType), t2(2, &testType), t3(3, &testType), t4(4, &testType), t5(5, &testType);
		ShuntingUnit suA(1, {t1, t2}), suB(2, {t3, t4}), suC(3, {t5});
		for (auto su : {&suA, &suB, &suC})
			state.AddShuntingUnit(su, &r0, &b0);

		Facility facility(1, "Clean", {"clean"}, 2, 0, 100000);
		Config config;
		capacity_facility_rule rule(&config);
		Task task("clean", false, 100, {});

		// Start a service of every member of the (state's own copy of the) unit
		auto serviceMembers = [&](int suID) {
			vector<const ServiceAction*> actions;
			auto su = state.GetShuntingUnitByID(suID);
			for (auto& train : su->GetTrains()) {
				auto action = new ServiceAction(su, &train, task, &facility, {});
				state.AddActiveAction(su, action);
				actions.push_back(action);
			}
			return actions;
		};
		auto serviceOf = [&](int suID, int trainID) {
			auto su = state.GetShuntingUnitByID(suID);
			return ServiceAction(su, su->GetTrainByID(trainID), task, &facility, {});
		};

		SUBCASE("a unit with several members being serviced is one user") {
			serviceMembers(1);
			// B's first member, then B's second while A's members are still active
			auto b1First = serviceOf(2, 3);
			CHECK(rule.IsValid(&state, &b1First).first);
			state.AddActiveAction(state.GetShuntingUnitByID(2), new ServiceAction(b1First));
			auto b1Second = serviceOf(2, 4);
			CHECK(rule.IsValid(&state, &b1Second).first);
		}

		SUBCASE("a facility with capacity 0 refuses every service") {
			Facility closed(2, "Clean", {"clean"}, 0, 0, 100000);
			auto su = state.GetShuntingUnitByID(3);
			ServiceAction action(su, su->GetTrainByID(5), task, &closed, {});
			CHECK(!rule.IsValid(&state, &action).first);
		}

		SUBCASE("a third unit is refused when two units are using the facility") {
			serviceMembers(2);
			state.AddActiveAction(state.GetShuntingUnitByID(3), new ServiceAction(serviceOf(3, 5)));
			// Units B (two members) and C are active: the facility is full
			auto a1 = serviceOf(1, 1);
			CHECK(!rule.IsValid(&state, &a1).first);
		}

		SUBCASE("the second of two single-member units fits, the third does not") {
			state.AddActiveAction(state.GetShuntingUnitByID(3), new ServiceAction(serviceOf(3, 5)));
			auto b1First = serviceOf(2, 3);
			CHECK(rule.IsValid(&state, &b1First).first);
			state.AddActiveAction(state.GetShuntingUnitByID(2), new ServiceAction(b1First));
			auto a1 = serviceOf(1, 1);
			CHECK(!rule.IsValid(&state, &a1).first);
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

	TEST_CASE("A multi-hop move replayed from a plan finishes on the plan's own duration (issue #18)") {
		// TORS's own search has no plan to consult, so MoveActionGenerator falls back to
		// summing a fixed duration per track type crossed (Location::GetDistance with
		// byType=true). That sum is independent of whatever duration a replayed plan
		// actually declared for this move, and over a long enough multi-hop route it can
		// exceed the plan's own window - the action then stays "active" past the plan's
		// next scheduled action for the same unit, which fails with a misleading "shunting
		// unit is already active" error. Trust the plan's own duration instead, the same
		// way Wait and Arrive already do above.
		Location location(TORS_DATA_DIR "/scenario_unification_test", true);
		Scenario scenario;

		vector<Track*> tracks = location.GetTracks();
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train train(1, &testType);
		ShuntingUnit su(1, {train});
		auto previous = location.GetTrackByID("1");
		auto position = location.GetTrackByID("2");
		state.AddShuntingUnit(&su, position, previous);

		json params = json::object();
		params["no_routing_duration"] = 100;
		params["constant_time"] = 0;
		params["default_time"] = true;
		params["norm_time"] = true;
		params["walk_time"] = true;
		MoveActionGenerator generator(params, &location);

		vector<string> routeIDs = {"1", "2"};
		vector<const Track*> route = {previous, position};
		auto fixedTypeSum = location.GetDistance(route);

		SUBCASE("a planned multi-hop move finishes on the plan's duration, not the fixed per-track-type sum") {
			int planDuration = fixedTypeSum + 12345;
			MultiMove planned({1}, routeIDs, planDuration);
			const Action* action = generator.Generate(&state, planned);
			CHECK(action->GetDuration() == planDuration);
			delete action;
		}

		SUBCASE("an unplanned multi-hop move (TORS's own search) still uses the fixed per-track-type sum") {
			MultiMove unplanned({1}, routeIDs);
			const Action* action = generator.Generate(&state, unplanned);
			CHECK(action->GetDuration() == fixedTypeSum);
			delete action;
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

	TEST_CASE("--departure_delay relaxes the exit time rule") {
		Track r0("r0", TrackPartType::Railroad, 200, "rail0", false, true, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		r0.AssignNeighbors({&b0}, {&b1});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&r0}, {});
		vector<Track*> tracks = {&r0, &b0, &b1};

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train train(1, &testType);
		ShuntingUnit su(1, {train});
		Outgoing departing(2, new ShuntingUnit(1, {train}), &r0, &b0, 1800, false, 0);

		Config config;
		out_correct_time_rule octr(&config);

		// Whether the exit at `time` is valid in a State built from a Scenario with `delay`.
		auto exitValid = [&](int delay, int time) {
			Scenario scenario;
			scenario.SetEndTime(3600);
			scenario.InitDepartureDelay(delay);
			State state(scenario, tracks);
			state.AddShuntingUnit(&su, &r0, &b0);
			ExitAction exitAction(state.GetShuntingUnitByID(1), 0, &departing);
			state.SetTime(time);
			return octr.IsValid(&state, &exitAction).first;
		};

		SUBCASE("a Scenario starts without any delay") {
			Scenario scenario;
			CHECK(scenario.GetDepartureDelay() == 0);
		}

		SUBCASE("delay 0 demands an exact match") {
			CHECK(exitValid(0, 1800));
			CHECK(!exitValid(0, 1801));
			CHECK(!exitValid(0, 1799));
		}

		SUBCASE("a late exit is valid up to the delay") {
			CHECK(exitValid(5, 1800));
			CHECK(exitValid(5, 1805));
			CHECK(!exitValid(5, 1806));
		}

		SUBCASE("an early exit is valid up to the delay") {
			CHECK(exitValid(5, 1795));
			CHECK(!exitValid(5, 1794));
		}

		SUBCASE("the delay survives copying the Scenario") {
			// RunResult holds its Scenario by value, so the Scenario that gets
			// evaluated is always a copy of the one the delay was set on.
			Scenario scenario;
			scenario.InitDepartureDelay(42);
			Scenario copy(scenario);
			CHECK(copy.GetDepartureDelay() == 42);
			State state(copy, tracks);
			CHECK(state.GetDepartureDelay() == 42);
		}

		SUBCASE("an outStanding unit ignores the delay") {
			Outgoing outstanding(1, new ShuntingUnit(1, {train}), &r0, &b0, 0, true, 0);
			Scenario scenario;
			scenario.SetEndTime(3600);
			scenario.InitDepartureDelay(90);
			State state(scenario, tracks);
			state.AddShuntingUnit(&su, &r0, &b0);
			ExitAction exitAction(state.GetShuntingUnitByID(1), 0, &outstanding);
			state.SetTime(3550);
			CHECK(!octr.IsValid(&state, &exitAction).first);
		}
	}

	TEST_CASE("A move may not drive through a parked unit, but may stop next to one (issue #20)") {
		// The tracks a MoveAction reserves exclude the unit's own track, so the
		// first of them is already one it drives onto and through. Only the last,
		// the destination, is somewhere it stops, and sharing that is left to the
		// length rule.
		Scenario scenario;

		// b0 == r0 == r1 == r2 == r3 == b1
		Track r0("r0", TrackPartType::Railroad, 1000, "rail0", false, true, true);
		Track r1("r1", TrackPartType::Railroad, 1000, "rail1", false, true, true);
		Track r2("r2", TrackPartType::Railroad, 1000, "rail2", false, true, true);
		Track r3("r3", TrackPartType::Railroad, 1000, "rail3", false, true, true);
		Track b0("b0", TrackPartType::Bumper, 10, "bumper0", false, false, true);
		Track b1("b1", TrackPartType::Bumper, 10, "bumper1", false, false, true);
		r0.AssignNeighbors({&b0}, {&r1});
		r1.AssignNeighbors({&r0}, {&r2});
		r2.AssignNeighbors({&r1}, {&r3});
		r3.AssignNeighbors({&r2}, {&b1});
		b0.AssignNeighbors({&r0}, {});
		b1.AssignNeighbors({&r3}, {});

		vector<Track*> tracks = {&r0, &r1, &r2, &r3, &b0, &b1};
		State state(scenario, tracks);

		TrainUnitType testType("TestType", 1, 100, 100, 100, 100, 100, 50, 100, "TT", false, false, false);
		Train moverTrain(1, &testType), parkedTrain(2, &testType);
		ShuntingUnit mover(1, {moverTrain}), parked(2, {parkedTrain});
		state.AddShuntingUnit(&mover, &r0, &b0);

		Config config;
		blocked_track_rule rule(&config);

		auto moveAcrossFourTracks = [&]() {
			vector<const Track*> path = {&r0, &r1, &r2, &r3};
			MoveAction move(state.GetShuntingUnitByID(1), path, 100, false);
			return rule.IsValid(&state, &move).first;
		};

		SUBCASE("a free path is allowed") {
			CHECK(moveAcrossFourTracks());
		}

		SUBCASE("a unit parked on the first track of the path blocks it") {
			state.AddShuntingUnit(&parked, &r1, &b0);
			CHECK(!moveAcrossFourTracks());
		}

		SUBCASE("a unit parked on a middle track of the path blocks it") {
			state.AddShuntingUnit(&parked, &r2, &b0);
			CHECK(!moveAcrossFourTracks());
		}

		SUBCASE("a unit parked on the destination does not block the move") {
			state.AddShuntingUnit(&parked, &r3, &b0);
			CHECK(moveAcrossFourTracks());
		}

		SUBCASE("a unit parked on the destination of a one-track move does not block it") {
			state.AddShuntingUnit(&parked, &r1, &b0);
			vector<const Track*> path = {&r0, &r1};
			MoveAction move(state.GetShuntingUnitByID(1), path, 100, false);
			CHECK(rule.IsValid(&state, &move).first);
		}
	}

	TEST_CASE("A facility without a timeWindow is always available (issue #47)") {
		PBFacility pb;
		pb.set_id(1);
		pb.set_type("Clean");
		pb.set_simultaneoususagecount(1);

		SUBCASE("absent window is unbounded") {
			REQUIRE(!pb.has_timewindow());
			Facility facility(pb);
			CHECK(facility.IsAvailable(0, 100));
			CHECK(facility.IsAvailable(1620, 300));
			CHECK(facility.IsAvailable(10000000, 10000000));
		}

		SUBCASE("a present window keeps its meaning") {
			pb.mutable_timewindow()->set_start(100);
			pb.mutable_timewindow()->set_end(200);
			Facility facility(pb);
			CHECK(facility.IsAvailable(100, 50));
			CHECK(!facility.IsAvailable(50, 50));
			CHECK(!facility.IsAvailable(150, 100));
		}

		SUBCASE("the window is closed: a service may end exactly at its end") {
			pb.mutable_timewindow()->set_start(100);
			pb.mutable_timewindow()->set_end(200);
			Facility facility(pb);
			CHECK(facility.IsAvailable(100, 100));
			CHECK(facility.IsAvailable(150, 50));
			CHECK(!facility.IsAvailable(150, 51));
		}

		SUBCASE("an explicit window of [0, 0) stays never available") {
			pb.mutable_timewindow();
			REQUIRE(pb.has_timewindow());
			Facility facility(pb);
			CHECK(!facility.IsAvailable(0, 10));
		}
	}
}
