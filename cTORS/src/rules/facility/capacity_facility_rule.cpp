#include "BusinessRules.h"

/**
 * Validates a ServiceAction for the given state. 
 * The ServiceAction is invalid iff
 * 1. The servicing of this Train would make the facility exceed its capacity.
 * Capacity counts shunting units, not ServiceActions: the members of one
 * ShuntingUnit are serviced one after the other, so the unit is a single user
 * of the facility however many of its members have an active ServiceAction.
 * @return A pair describing 1) whether the action is valid, and 2) if not, why
 */
pair<bool, string> capacity_facility_rule::IsValid(const State* state, const Action* action) const {
	if (auto sa = dynamic_cast<const ServiceAction*>(action)) {
		auto fa = sa->GetFacility();
		auto& sus = state->GetShuntingUnits();
		// The action under validation is not active yet, so its own unit is
		// counted up front (once, even if it is already using the facility).
		int count = 1;
		for (auto su : sus) {
			if (su == sa->GetShuntingUnit()) continue;
			auto& actions = state->GetActiveActions(su);
			for (auto a : actions) {
				auto _sa = dynamic_cast<const ServiceAction*>(a);
				if (_sa && _sa->GetFacility() == fa) {
					count++;
					break;
				}
			}
		}
		if (count > fa->GetCapacity())
			return make_pair(false, fa->toString() + " is at its capacity and cannot service train " + sa->GetTrain()->toString() + " from " + sa->GetShuntingUnit()->toString()+".");
	}
	return make_pair(true, "");
}
