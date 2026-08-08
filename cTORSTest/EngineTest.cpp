#include "doctest/doctest.h"
#include "Engine.h"

#include <google/protobuf/util/json_util.h>

namespace cTORSTest
{
	TEST_CASE("Scenario unification: reads the unified (HIP-shaped) scenario JSON")
	{
		// Self-contained fixture under data/Demo/scenario_unification_test, copied into the
		// test binary directory by cTORSTest/CMakeLists.txt. Does not require env vars.
		Location location(TORS_DATA_DIR "/Demo/scenario_unification_test", true);
		Scenario scenario(TORS_DATA_DIR "/Demo/scenario_unification_test/scenario.json", location);

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
}
