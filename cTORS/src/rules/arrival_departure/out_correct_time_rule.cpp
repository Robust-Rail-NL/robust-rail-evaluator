#include "BusinessRules.h"

/**
 * Validates an ExitAction for the given state.
 * The ExitAction is invalid if
 * 1. for a departing ShuntingUnit, the time of the action is not the same as the
 *    time described by the Outgoing attribute;
 * 2. for an outStanding ShuntingUnit, the scenario has not reached its end yet.
 *
 * An outStanding request describes a ShuntingUnit that stays in the shunting yard
 * after the scenario ends, so it has no departure time of its own — its Outgoing
 * carries time 0. Requiring the clock to equal that would contradict
 * ExitActionGenerator, which only offers the exit once the clock has reached the
 * end of the scenario, so an outStanding unit could never exit validly. Nothing
 * exercised this while HIP serialised an outStanding train's final task as a bare
 * Wait with no Exit at all.
 * @return A pair describing 1) whether the action is valid, and 2) if not, why
 */
pair<bool, string> out_correct_time_rule::IsValid(const State* state, const Action* action) const {
	if (auto ea = dynamic_cast<const ExitAction*>(action)) {
		auto outgoing = ea->GetOutgoing();
		if (outgoing->IsInstanding()) {
			if (state->GetTime() < state->GetEndTime())
				return make_pair(false, "Shunting unit " + outgoing->GetShuntingUnit()->toString()
					+ " stays in the yard, so it cannot leave before the end of the scenario at time "
					+ to_string(state->GetEndTime()));
		}
		else if(state->GetTime() != outgoing->GetTime())
			return make_pair(false, "Shunting unit " + outgoing->GetShuntingUnit()->toString() + " should leave at time " + to_string(outgoing->GetTime()) );
	}
	return make_pair(true, "");
}

