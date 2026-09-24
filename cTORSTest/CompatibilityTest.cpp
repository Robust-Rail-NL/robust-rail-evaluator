#include "doctest/doctest.h"
#include "Engine.h"


#include <cstdlib>
#include <fstream>
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

	TEST_CASE("A train that cannot move off the gateway right away still evaluates (solver#13)")
	{
		// solver#13: a train that arrives but whose route into the yard is not free yet
		// used to be serialised as an instantaneous Arrive followed by a Wait on
		// the arrival track - and the arrival track here (as on the real
		// KleineBinckhorst scenarios the issue was filed against) is the gateway,
		// which forbids parking, so legal_on_parking_track_rule rejected the Wait
		// and the plan failed. The fix is for the plan to give the Arrive itself
		// that duration instead (robust-rail-solver's side) and for the Arrive to
		// actually honour it (this repo's side, exercised here).
		//
		// Widen shunting unit 0's Arrive (originally instantaneous at T300) by
		// 50s, and push the Move that follows it back by the same 50s, as if
		// the gateway route had only become free at T350 instead of T300 - the
		// shape a fixed solver will produce, with no trailing Wait.
		LocationEngine engine(HIP_FIXTURE);
		const Location &location = engine.GetLocation();

		PB_HIP_Plan pb_hip_plan;
		ParseHIP_PlanFromJson(HIP_FIXTURE + "/plan.json", pb_hip_plan);

		PB_HIP_Action *arrive = nullptr, *move = nullptr;
		for (auto &action : *pb_hip_plan.mutable_actions())
		{
			if (action.shuntingunit().id() != 0)
				continue;
			if (arrive == nullptr && action.tasktype().predefined() == PB_HIP_PredefinedTaskType::Arrive)
				arrive = &action;
			else if (arrive != nullptr && move == nullptr && action.tasktype().predefined() == PB_HIP_PredefinedTaskType::Move)
				move = &action;
		}
		REQUIRE(arrive != nullptr);
		REQUIRE(move != nullptr);
		// Guard the fixture assumption that `move` is the task immediately
		// following `arrive` in shunting unit 0's chain, not some later,
		// unrelated Move for the same unit.
		REQUIRE(move->starttime() == arrive->endtime());

		const uint64_t gap = 50;
		arrive->set_endtime(arrive->starttime() + gap);
		move->set_starttime(move->starttime() + gap);
		move->set_endtime(move->endtime() + gap);

		auto runResult = RunResult::CreateRunResult(pb_hip_plan, HIP_FIXTURE + "/scenario.json", &location);
		REQUIRE(runResult != nullptr);
		CHECK(engine.EvaluatePlan(runResult->GetScenario(), runResult->GetPlan()));

		delete runResult;
	}

	TEST_CASE("ParseHIP_PlanFromJson throws on JSON that doesn't correspond to a HIP plan")
	{
		// Valid JSON, parses with an ok status (ignore_unknown_fields drops the unrecognized
		// field), but leaves the PB_HIP_Plan empty - should be a loud failure, not a silently
		// empty plan handed on to evaluation.
		fs::path tmp = fs::temp_directory_path() / "compatibility_test_mismatched_plan.json";
		{
			std::ofstream out(tmp);
			out << R"({"thisFieldDoesNotExistOnAHipPlan": true})";
		}
		PB_HIP_Plan pb_hip_plan;
		CHECK_THROWS_AS(ParseHIP_PlanFromJson(tmp.string(), pb_hip_plan), std::runtime_error);
		fs::remove(tmp);
	}

	TEST_CASE("ParseHIP_PlanFromJson throws on a HIP plan with no actions")
	{
		// Non-empty overall (schemaVersion is set), but has nothing to evaluate - should be
		// rejected up front rather than silently evaluated as a plan that does nothing.
		fs::path tmp = fs::temp_directory_path() / "compatibility_test_actionless_plan.json";
		{
			std::ofstream out(tmp);
			out << R"({"schemaVersion": 1})";
		}
		PB_HIP_Plan pb_hip_plan;
		CHECK_THROWS_AS(ParseHIP_PlanFromJson(tmp.string(), pb_hip_plan), std::invalid_argument);
		fs::remove(tmp);
	}

	/** Parse a one-action HIP plan whose action has the given taskType JSON, then
	 * convert it with CreateRunResult and return the invalid_argument message it
	 * throws (empty if it throws none). */
	static string ConversionErrorForTaskType(const string &name, const string &taskTypeJson)
	{
		fs::path tmp = fs::temp_directory_path() / ("compatibility_test_" + name + ".json");
		{
			std::ofstream out(tmp);
			out << R"({"schemaVersion": 1, "actions": [{"startTime": 300, "endTime": 360, "taskType": )"
				<< taskTypeJson
				<< R"(, "shuntingUnit": {"id": 7, "memberIDs": [2401]}, "location": 15, "resources": []}]})";
		}
		PB_HIP_Plan pb_hip_plan;
		ParseHIP_PlanFromJson(tmp.string(), pb_hip_plan);
		fs::remove(tmp);

		LocationEngine engine(HIP_FIXTURE);
		string error;
		try
		{
			delete RunResult::CreateRunResult(pb_hip_plan, HIP_FIXTURE + "/scenario.json", &engine.GetLocation());
		}
		catch (const std::invalid_argument &e)
		{
			error = e.what();
		}
		return error;
	}

	TEST_CASE("A plan action with an unknown task type name is rejected, not dropped (issue #28)")
	{
		// "Setback" is what the reversal task type was called before it became
		// "Reverse". ignore_unknown_fields makes protobuf skip the unknown enum
		// name too, leaving the taskType oneof unset; the conversion used to drop
		// such an action silently, so the plan was evaluated as if it had no
		// reversal at all and failed on an unrelated-looking parking rule.
		string error = ConversionErrorForTaskType("unknown_task_type", R"({"predefined": "Setback"})");

		CHECK(error.find("Plan action 0") != string::npos);
		CHECK(error.find("start time 300") != string::npos);
		CHECK(error.find("shunting unit 7") != string::npos);
		CHECK(error.find("no task type") != string::npos);
	}

	TEST_CASE("A plan action with no task type at all is rejected (issue #28)")
	{
		string error = ConversionErrorForTaskType("missing_task_type", "{}");

		CHECK(error.find("Plan action 0") != string::npos);
		CHECK(error.find("no task type") != string::npos);
	}

	TEST_CASE("A plan action with a predefined task type the conversion does not support is rejected (issue #28)")
	{
		// Break is a valid PredefinedTaskType, but the Solver-format conversion has
		// no case for it; it used to fall through to a silent `default: break;`.
		string error = ConversionErrorForTaskType("unsupported_task_type", R"({"predefined": "Break"})");

		CHECK(error.find("Plan action 0") != string::npos);
		CHECK(error.find("Break") != string::npos);
		CHECK(error.find("does not support") != string::npos);
	}
}
