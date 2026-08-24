#include "doctest/doctest.h"
#include "Plan.h"

namespace cTORSTest
{
	/** Build one HIP-shaped Combine action as CreateRunResult produces them:
	 * the unit's ids land in both the action's trainUnitIds and the task's. */
	static PBAction MakeCombineAction(int startTime, const vector<string> &unitIDs)
	{
		PBAction action;
		action.set_suggestedstartingtime(startTime);
		action.set_suggestedfinishingtime(startTime);
		PBTaskAction *task = action.mutable_task();
		task->mutable_type()->set_predefined(PBPredefinedTaskType::Combine);
		for (auto &id : unitIDs)
		{
			action.add_trainunitids(id);
			task->add_trainunitids(id);
		}
		return action;
	}

	static vector<string> ActionIDs(const PBAction &action)
	{
		return vector<string>(action.trainunitids().begin(), action.trainunitids().end());
	}

	static vector<string> TaskIDs(const PBAction &action)
	{
		return vector<string>(action.task().trainunitids().begin(), action.task().trainunitids().end());
	}

	/** Build one HIP action for the given unit, as it appears in a plan. */
	static PB_HIP_Action MakeHipAction(PB_HIP_PredefinedTaskType type, const vector<uint64_t> &memberIDs)
	{
		PB_HIP_Action action;
		action.mutable_tasktype()->set_predefined(type);
		PB_HIP_ShuntingUnit *su = action.mutable_shuntingunit();
		su->set_id(1);
		for (auto id : memberIDs)
			su->add_memberids(id);
		return action;
	}

	TEST_CASE("A movement is followed by an Exit only if that unit's own next action is one") {
		// A plan interleaves every shunting unit's actions in time order, so the next
		// entry in the list usually belongs to a different unit. Testing the list
		// rather than the unit made a departure look like an ordinary movement, so an
		// EndMove was emitted onto the gateway and the parking rules rejected it.
		PB_HIP_ShuntingUnit departing;
		departing.set_id(1);
		departing.add_memberids(2601);

		SUBCASE("its Exit comes immediately next") {
			vector<PB_HIP_Action> actions = {
				MakeHipAction(PB_HIP_PredefinedTaskType::Move, {2601}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Exit, {2601}),
			};
			CHECK(RunResult::NextActionForUnitIsExit(actions, 0, departing));
		}

		SUBCASE("its Exit comes next for this unit, but not next in the list") {
			// This is the case that regressed: another train acts in between.
			vector<PB_HIP_Action> actions = {
				MakeHipAction(PB_HIP_PredefinedTaskType::Move, {2601}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Wait, {2801}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Exit, {2601}),
			};
			CHECK(RunResult::NextActionForUnitIsExit(actions, 0, departing));
		}

		SUBCASE("another unit's Exit does not count") {
			vector<PB_HIP_Action> actions = {
				MakeHipAction(PB_HIP_PredefinedTaskType::Move, {2601}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Exit, {2801}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Wait, {2601}),
			};
			CHECK(!RunResult::NextActionForUnitIsExit(actions, 0, departing));
		}

		SUBCASE("the unit goes on to do something else") {
			vector<PB_HIP_Action> actions = {
				MakeHipAction(PB_HIP_PredefinedTaskType::Move, {2601}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Wait, {2601}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Exit, {2601}),
			};
			CHECK(!RunResult::NextActionForUnitIsExit(actions, 0, departing));
		}

		SUBCASE("the movement is the last action in the plan") {
			// Reading one past the end here used to be undefined behaviour.
			vector<PB_HIP_Action> actions = {
				MakeHipAction(PB_HIP_PredefinedTaskType::Move, {2601}),
			};
			CHECK(!RunResult::NextActionForUnitIsExit(actions, 0, departing));
		}

		SUBCASE("a multi-unit shunting unit is matched on its whole membership") {
			PB_HIP_ShuntingUnit pair;
			pair.set_id(1);
			pair.add_memberids(2801);
			pair.add_memberids(2802);
			vector<PB_HIP_Action> actions = {
				MakeHipAction(PB_HIP_PredefinedTaskType::Move, {2801, 2802}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Exit, {2801}),
				MakeHipAction(PB_HIP_PredefinedTaskType::Exit, {2801, 2802}),
			};
			CHECK(RunResult::NextActionForUnitIsExit(actions, 0, pair));
		}
	}

	TEST_CASE("Merging a combine keeps its two operands apart") {
		// A cTORS Combine names two shunting units: the front one in the action's
		// own trainUnitIds and the rear one in the task's. HIP emits one action per
		// unit, so merging them has to put each unit in exactly one operand. Leaving
		// the first unit in both makes the rear operand span two shunting units,
		// which cannot resolve to one — it tripped an assertion in debug builds and
		// silently combined the wrong units in release builds.

		SUBCASE("two single-unit actions") {
			vector<PBAction> combine = {
				MakeCombineAction(1080, {"2801"}),
				MakeCombineAction(1080, {"2802"}),
			};
			PBAction merged = RunResult::MergeCombineActions(combine);

			CHECK(ActionIDs(merged) == vector<string>{"2801"});
			CHECK(TaskIDs(merged) == vector<string>{"2802"});
		}

		SUBCASE("an operand may itself cover several units") {
			vector<PBAction> combine = {
				MakeCombineAction(600, {"1", "2"}),
				MakeCombineAction(600, {"3", "4"}),
			};
			PBAction merged = RunResult::MergeCombineActions(combine);

			CHECK(ActionIDs(merged) == vector<string>{"1", "2"});
			CHECK(TaskIDs(merged) == vector<string>{"3", "4"});
		}

		SUBCASE("no unit appears in both operands") {
			vector<PBAction> combine = {
				MakeCombineAction(1080, {"2801"}),
				MakeCombineAction(1080, {"2802"}),
			};
			PBAction merged = RunResult::MergeCombineActions(combine);

			for (auto &front : ActionIDs(merged))
				for (auto &rear : TaskIDs(merged))
					CHECK(front != rear);
		}

		SUBCASE("more than two units combine into one") {
			vector<PBAction> combine = {
				MakeCombineAction(300, {"10"}),
				MakeCombineAction(300, {"11"}),
				MakeCombineAction(300, {"12"}),
			};
			PBAction merged = RunResult::MergeCombineActions(combine);

			CHECK(ActionIDs(merged) == vector<string>{"10"});
			CHECK(TaskIDs(merged) == vector<string>({"11", "12"}));
		}

		SUBCASE("the merged action keeps the timing of the combine") {
			vector<PBAction> combine = {
				MakeCombineAction(1080, {"2801"}),
				MakeCombineAction(1080, {"2802"}),
			};
			PBAction merged = RunResult::MergeCombineActions(combine);

			CHECK(merged.suggestedstartingtime() == 1080);
			CHECK(merged.task().type().predefined() == PBPredefinedTaskType::Combine);
		}
	}
}
