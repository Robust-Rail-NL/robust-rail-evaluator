#include "BusinessRules.h"

/**
 * Validates an ExitAction for the given state. 
 * The ExitAction is invalid if
 * 1. The assigned ShuntingUnit does not match the demans of the ShuntingUnit in the Outgoing attribute.
 * 
 * See ShuntingUnit::MatchesShuntingUnit(const ShuntingUnit*) const
 *
 * Additionally, for a scenario of schemaVersion HEAD_FIRST_SCHEMA_VERSION or later and an Outgoing that
 * is not standing, the units must leave in the requested order from the head (index 0 of the list in
 * the scenario file is the head, see ShuntingUnit::MatchesHeadFirst); leaving in the mirrored order is
 * invalid. An older scenario is read as before, with the order accepted from either end.
 * @return A pair describing 1) whether the action is valid, and 2) if not, why
 */
pair<bool, string> out_correct_order_rule::IsValid(const State* state, const Action* action) const {
	if (auto ea = dynamic_cast<const ExitAction*>(action)) {
		if(!ea->GetOutgoing()->GetShuntingUnit()->MatchesShuntingUnit(ea->GetShuntingUnit()))
			return make_pair(false, "Leaving shunting unit " + ea->GetShuntingUnit()->toString() + " did not match the train unit order of outgoing train " + ea->GetOutgoing()->GetShuntingUnit()->toString());
		if(state->HeadFirstUnitOrder() && !ea->GetOutgoing()->IsInstanding()) {
			auto physical = state->GetTrainUnitsInOrder(ea->GetShuntingUnit());
			if(!ea->GetOutgoing()->GetShuntingUnit()->MatchesHeadFirst(physical))
				return make_pair(false, "Leaving shunting unit " + ea->GetShuntingUnit()->toString() + " matches outgoing train "
					+ ea->GetOutgoing()->GetShuntingUnit()->toString() + " only when read from the tail: from the head the units leave in the "
					"opposite order. The scenario (schemaVersion " + to_string(HEAD_FIRST_SCHEMA_VERSION) + " or later) lists the units of a "
					"departing train from its head, so they must leave in that order (use a Reverse, or split and combine).");
		}
	}
	return make_pair(true, "");
}

