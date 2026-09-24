#include "BusinessRules.h"
#include "Utils.h"

/**
 * Validates an Action for the given state. 
 * The Action is invalid iff
 * 1. The Action uses a Track that is reserved in the current State.
 * @return A pair describing 1) whether the action is valid, and 2) if not, why
 * 
 * For multi-move actions, this rule checks that the path is never blocked by other 
 * ShuntingUnits.
 */
pair<bool, string> blocked_track_rule::IsValid(const State* state, const Action* action) const {
	auto ress = action->GetReservedTracks();
	for (auto res : ress) {
		if (state->IsReserved(res)) {
			return make_pair(false, "Track " + res->toString() + " is reserved.");
		}
	}

	// Check that no tracks reserved by the action (except possibly the first and the last) are
	// occupied.
	// A saw move (the same track revisited two hops apart, e.g. [..,X,Y,X,..]) means the
	// ShuntingUnit reverses direction mid-route rather than fully traversing every reserved
	// track. This should be represented as a separate Reverse action instead of being
	// embedded in one Move - see doc/known-issue-plan-type.md. A plan declaring
	// schemaVersion >= REVERSE_REQUIRED_SCHEMA_VERSION is rejected outright for this; an
	// older (or unversioned) plan is only warned, since no real origin emits the explicit
	// shape yet and existing plans must keep evaluating exactly as before.
	for ( size_t i = 1; i + 1 < ress.size(); ++i ) {
		auto res = ress.at(i);
		if ( ress.at(i-1) == ress.at(i+1) ) {
			// This is (probably) a saw move. Delegate checking whether there's enough room on the track to the length_track_rule.
			if ( state->GetPlanSchemaVersion() >= REVERSE_REQUIRED_SCHEMA_VERSION ) {
				return make_pair(false, "Move action embeds an in-place reversal (saw) at position " + to_string(i) +
					", track " + res->toString() + ", id " + res->GetID() + ", with no explicit Reverse action. "
					"Plans declaring schemaVersion " + to_string(REVERSE_REQUIRED_SCHEMA_VERSION) + " or later must "
					"use an explicit Reverse action for a reversal - see doc/known-issue-plan-type.md.");
			}
			cerr << "WARNING: Move action embeds an in-place reversal (saw) at position " << i
				 << ", track " << res->toString() << ", id " << res->GetID() << ", with no explicit "
				 << "Reverse action. This plan shape is deprecated and will be rejected for any plan "
				 << "declaring schemaVersion " << REVERSE_REQUIRED_SCHEMA_VERSION << " or later - see "
				 << "doc/known-issue-plan-type.md." << endl;
			continue;
		}
		if ( state->GetOccupations(res).size() > 0 ) {
			cout << "Collission detected at position " << i << ", track " << res->toString() << ", id " << res->GetID()  << endl;
			return make_pair(false, "Track " + res->toString() + ", id " + res->GetID() + " is occupied.");
		}
	}
	return make_pair(true, "");
}

