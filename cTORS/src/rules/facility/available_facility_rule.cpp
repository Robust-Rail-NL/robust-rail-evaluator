#include "BusinessRules.h"

/**
 * Validates a ServiceAction for the given state. 
 * The ServiceAction is invalid iff
 * 1. Facility is not available at the current time up and until completion of the task.
 * @return A pair describing 1) whether the action is valid, and 2) if not, why
 */
pair<bool, string> available_facility_rule::IsValid(const State* state, const Action* action) const {
	if (auto sa = dynamic_cast<const ServiceAction*>(action)) {
		auto fa = sa->GetFacility();
		// GetDuration() here, not GetTask()->duration: for a member serviced
		// after others sharing the same coupled-unit visit, GetDuration() is
		// deliberately the running cumulative total, not this member's own
		// task duration (see ServiceAction's totalOccupiedDuration
		// constructor) - the facility needs to stay available for however
		// long the ShuntingUnit is actually occupying it, which is that total.
		if (!fa->IsAvailable(state->GetTime(), sa->GetDuration()))
			return make_pair(false, fa->toString() + " is not available from " + to_string(state->GetTime()) +
				" to " + to_string(state->GetTime() + sa->GetDuration()) + ".");
	}
	return make_pair(true, "");
}

