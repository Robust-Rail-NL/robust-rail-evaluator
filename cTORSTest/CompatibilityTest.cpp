#include "doctest/doctest.h"
#include "Engine.h"


#include <cstdlib>
#include <google/protobuf/util/json_util.h>

namespace cTORSTest
{
	/**
	 * Evaluating a plan produced by HIP is the path the whole pipeline runs on:
	 * plan JSON, through RunResult::CreateRunResult, into EvaluatePlan. Nothing
	 * exercised it automatically, which is how a rule that made departure
	 * impossible on any location with a non-parking gateway survived every
	 * release from v1.0.0 onwards.
	 *
	 * The fixture is a KleineBinckhorst scenario chosen because one plan covers an
	 * unusual amount of that path at once: an inStanding train (StandIn), an
	 * outStanding train (StandOut), ordinary arrivals and departures, movements,
	 * waits, and a custom servicing task. It is a plan the pipeline reports as
	 * valid, so a regression on any of those turns this red.
	 *
	 * It contains no Combine — its two-unit train arrives already coupled — so the
	 * combine conversion is covered by PlanConversionTest instead, directly on
	 * RunResult::MergeCombineActions.
	 *
	 * This is the same Kleine Binckhorst data as the repo's top-level
	 * example_kleine_binckhorst/ (the README/CLI demo fixture) - reused
	 * directly rather than duplicated under cTORSTest/fixtures/, so there is
	 * exactly one copy of it.
	 */
	static const string HIP_FIXTURE = TORS_EXAMPLE_DIR;

	TEST_CASE("A valid HIP plan evaluates as valid")
	{
		LocationEngine engine(HIP_FIXTURE);
		const Location &location = engine.GetLocation();
		auto &scenario = engine.GetScenario(HIP_FIXTURE + "/scenario.json");

		// The scenario has to be self-consistent before a plan for it means anything.
		CHECK_NOTHROW(scenario.CheckScenarioCorrectness(location));

		PB_HIP_Plan pb_hip_plan;
		ParseHIP_PlanFromJson(HIP_FIXTURE + "/plan.json", pb_hip_plan);

		auto runResult = RunResult::CreateRunResult(pb_hip_plan, HIP_FIXTURE + "/scenario.json", &location);
		REQUIRE(runResult != nullptr);
		CHECK(engine.EvaluatePlan(runResult->GetScenario(), runResult->GetPlan()));

		delete runResult;
	}

	TEST_CASE("The fixture plan covers the actions this path is most likely to break on")
	{
		// Guards the fixture itself: if it is ever regenerated from a different
		// scenario and quietly loses its inStanding or outStanding train, the test
		// above keeps passing while covering considerably less.
		PB_HIP_Plan pb_hip_plan;
		ParseHIP_PlanFromJson(HIP_FIXTURE + "/plan.json", pb_hip_plan);

		set<PB_HIP_PredefinedTaskType> present;
		for (auto &action : pb_hip_plan.actions())
			if (action.tasktype().has_predefined())
				present.insert(action.tasktype().predefined());

		CHECK(present.count(PB_HIP_PredefinedTaskType::StandIn) == 1);
		CHECK(present.count(PB_HIP_PredefinedTaskType::StandOut) == 1);
		CHECK(present.count(PB_HIP_PredefinedTaskType::Arrive) == 1);
		CHECK(present.count(PB_HIP_PredefinedTaskType::Exit) == 1);
		CHECK(present.count(PB_HIP_PredefinedTaskType::Move) == 1);
		CHECK(present.count(PB_HIP_PredefinedTaskType::Wait) == 1);

		// And at least one custom (Other) task type, which takes a different branch
		// through the conversion than the predefined ones.
		bool hasCustomTask = false;
		for (auto &action : pb_hip_plan.actions())
			if (action.tasktype().has_other())
				hasCustomTask = true;
		CHECK(hasCustomTask);
	}

}
