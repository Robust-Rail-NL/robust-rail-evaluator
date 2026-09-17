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

	TEST_CASE("Scenario correctness: per-type train counts accumulate past 1")
	{
		// Self-contained fixture under cTORSTest/fixtures/per_type_count_accumulation_test:
		// 1 TT-1 train arrives, but 2 TT-1 trains depart (in two separate outgoing
		// shunting units). The per-type counters used to be reset to 0 every time a
		// type was seen again, capping every count at 1 regardless of how many
		// trains actually had that type - so an imbalance like this (1 in, 2 out)
		// was never detected as long as both sides had at least one train of the
		// type. Must be rejected as infeasible.
		Location location(TORS_DATA_DIR "/per_type_count_accumulation_test", true);
		Scenario scenario(TORS_DATA_DIR "/per_type_count_accumulation_test/scenario.json", location);
		CHECK_THROWS_AS(scenario.CheckScenarioCorrectness(location), std::invalid_argument);
	}
}
