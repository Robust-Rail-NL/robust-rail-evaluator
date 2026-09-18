#include "doctest/doctest.h"
#include "Engine.h"

#include <google/protobuf/util/json_util.h>

namespace cTORSTest
{
	TEST_CASE("Scenario unification: reads the unified (HIP-shaped) scenario JSON")
	{
		// Self-contained fixture under cTORSTest/fixtures/scenario_unification_test.
		// Does not require env vars.
		Location location(TORS_DATA_DIR "/scenario_unification_test", true);
		Scenario scenario(TORS_DATA_DIR "/scenario_unification_test/scenario.json", location);

		REQUIRE(scenario.GetIncomingTrains().size() == 1);
		auto incoming = scenario.GetIncomingTrains().front();
		CHECK(incoming->GetID() == 10);
		CHECK(incoming->GetTime() == 100); // IncomingTrain.arrival
		CHECK(incoming->GetParkingTrack()->GetID() == "1"); // firstParkingTrackPart
		CHECK(incoming->GetSideTrack()->GetID() == "2"); // entryTrackPart

		auto& incomingTrains = incoming->GetShuntingUnit()->GetTrains();
		REQUIRE(incomingTrains.size() == 2);

		// Regression test for the (typePrefix, carriages) disambiguation fix: two
		// train units share typePrefix "TT" but have different carriage counts, and
		// must resolve to two distinct TrainUnitType objects, not collapse into one.
		auto train101 = incoming->GetShuntingUnit()->GetTrainByID(101);
		auto train102 = incoming->GetShuntingUnit()->GetTrainByID(102);
		REQUIRE(train101 != nullptr);
		REQUIRE(train102 != nullptr);
		CHECK(train101->GetType()->displayName == "TT-1");
		CHECK(train101->GetType()->carriages == 1);
		CHECK(train102->GetType()->displayName == "TT-2");
		CHECK(train102->GetType()->carriages == 2);
		CHECK(train101->GetType() != train102->GetType());

		auto tasks = scenario.GetTasksForTrain(train101);
		REQUIRE(tasks.size() == 1);
		CHECK(tasks.front().optional == false); // "optional": false in the fixture
		CHECK(tasks.front().duration == 50);

		// Regression test (issue #25): train102 has "tasks": [] in the fixture, so it
		// never gets a key in the Incoming's tasks map (only trains with at least one
		// task do). GetTasksForTrain must return an empty list for it rather than
		// throwing unordered_map::at.
		CHECK(scenario.GetTasksForTrain(train102).size() == 0);

		REQUIRE(scenario.GetOutgoingTrains().size() == 1);
		auto outgoing = scenario.GetOutgoingTrains().front();
		CHECK(outgoing->GetID() == 20); // derived from TrainRequest.displayName, which has no separate id field
		CHECK(outgoing->GetTime() == 600); // TrainRequest.departure
		CHECK(outgoing->GetParkingTrack()->GetID() == "1"); // lastParkingTrackPart
		CHECK(outgoing->GetSideTrack()->GetID() == "3"); // leaveTrackPart

		auto& outgoingTrains = outgoing->GetShuntingUnit()->GetTrains();
		REQUIRE(outgoingTrains.size() == 2);
		CHECK(outgoing->GetShuntingUnit()->GetTrainByID(101)->GetType()->carriages == 1);
		CHECK(outgoing->GetShuntingUnit()->GetTrainByID(102)->GetType()->carriages == 2);

		CHECK(scenario.GetStartTime() == 0);
		CHECK(scenario.GetEndTime() == 1000);

		// CheckScenarioCorrectness's per-type count balancing is keyed on
		// (displayName, carriages) - this fixture's in/out counts are balanced
		// per carriage variant (1 TT/1 and 1 TT/2 each way), so this must not throw.
		CHECK_NOTHROW(scenario.CheckScenarioCorrectness(location));
	}

	TEST_CASE("Scenario correctness: task time is scoped per shunting unit, not summed across the whole scenario (issue #12)")
	{
		// Self-contained fixture under cTORSTest/fixtures/parallel_facility_task_time_test.
		Location location(TORS_DATA_DIR "/parallel_facility_task_time_test", true);

		// Two shunting units each needing 2000s of service time (4000s combined)
		// against a 3600s scenario used to be rejected outright, because
		// CheckScenarioCorrectness summed task duration across every shunting unit
		// in the scenario instead of scoping it per unit - ignoring that they can be
		// serviced in parallel on separate facilities. Reproduces the exact numbers
		// from the issue's report ([4000] > [3600]).
		Scenario scenario(TORS_DATA_DIR "/parallel_facility_task_time_test/scenario.json", location);
		CHECK_NOTHROW(scenario.CheckScenarioCorrectness(location));

		// Guards against the fix over-correcting into a no-op: a single shunting
		// unit's own tasks (4000s here) still can't exceed the scenario's end time
		// (3600s), since one unit genuinely can't be in two places at once.
		Scenario overage(TORS_DATA_DIR "/parallel_facility_task_time_test/scenario_single_unit_overage.json", location);
		CHECK_THROWS_AS(overage.CheckScenarioCorrectness(location), std::invalid_argument);
	}

	TEST_CASE("A Service action services every coupled member sharing the task, sequentially (issue #26)")
	{
		// Self-contained fixture under cTORSTest/fixtures/multi_member_service_test:
		// both members of the incoming shunting unit independently need the same
		// mandatory "Clean" task (50s each).
		LocationEngine engine(TORS_DATA_DIR "/multi_member_service_test");
		auto &scenario = engine.GetScenario(TORS_DATA_DIR "/multi_member_service_test/scenario.json");

		auto state = engine.StartSession(scenario);
		engine.Step(state);

		auto incoming = scenario.GetIncomingTrains().front();
		engine.ApplyActionAndStep(state, Arrive(incoming));

		auto su = state->GetShuntingUnitByID(10);
		REQUIRE(su != nullptr);
		auto train101 = su->GetTrainByID(101);
		auto train102 = su->GetTrainByID(102);
		REQUIRE(train101 != nullptr);
		REQUIRE(train102 != nullptr);

		auto &facilities = state->GetShuntingUnitState(su).position->GetFacilities();
		REQUIRE(facilities.size() == 1);

		int timeBefore = state->GetTime();
		// Only one member is named on the SimpleAction, as a replayed plan action
		// would - the plan doesn't record which specific coupled member the task
		// was "for" (issue #25).
		auto task101 = state->GetTasksForTrain(train101).front();
		engine.ApplyActionAndStep(state, Service(su, task101, *train101, facilities.front()));

		CHECK(state->GetTasksForTrain(train101).size() == 0);
		CHECK(state->GetTasksForTrain(train102).size() == 0);
		// Sequential, not parallel: 50 (train101) + 50 (train102), matching the
		// solver's own duration model behind issue #26 rather than finishing both
		// at once after only 50s.
		CHECK(state->GetTime() - timeBefore == 100);

		// That total comes from the second member's ServiceAction carrying the
		// running cumulative duration (100), not its own task's duration (50) -
		// see ServiceAction's totalOccupiedDuration constructor. Documented here
		// so this deliberate GetDuration()-isn't-the-task's-own-duration choice
		// doesn't read as a bug to a future reader.
		// (recorded[0] is the preceding Arrive; the two Service actions follow.)
		auto &recorded = engine.GetResult(state)->GetActions();
		REQUIRE(recorded.size() == 3);
		CHECK(recorded[1].GetMinimumDuration() == 50);
		CHECK(recorded[2].GetMinimumDuration() == 100);
	}

	TEST_CASE("A Service action throws when the unit isn't at any facility")
	{
		LocationEngine engine(TORS_DATA_DIR "/multi_member_service_test");
		auto &scenario = engine.GetScenario(TORS_DATA_DIR "/multi_member_service_test/scenario.json");
		auto &location = engine.GetLocation();

		auto state = engine.StartSession(scenario);
		engine.Step(state);

		// Place the unit directly on a bumper track (no facility there), rather
		// than arriving onto the fixture's parking track (which does have one),
		// so the Service branch's "no facility at this position" guard is the
		// thing actually exercised.
		auto incoming = scenario.GetIncomingTrains().front();
		auto su = incoming->GetShuntingUnit();
		state->AddShuntingUnit(su, location.GetTrackByID("2"), location.GetTrackByID("3"));
		state->AddTasksToTrains(incoming->GetTasks());

		auto train101 = su->GetTrainByID(101);
		auto task101 = state->GetTasksForTrain(train101).front();
		auto facility = location.GetFacilityByID(1);

		CHECK_THROWS_AS(engine.ApplyActionAndStep(state, Service(su, task101, *train101, facility)), InvalidActionException);
	}
}
