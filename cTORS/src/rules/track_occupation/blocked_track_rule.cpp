#include "BusinessRules.h"

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
	// track. This should be represented as a separate Setback/Walking action instead of being
	// embedded in one Move - see doc/known-issue-plan-type.md - but no real producer emits that
	// shape yet, so this is tolerated for now with a warning rather than rejected outright.
	for ( size_t i = 1; i + 1 < ress.size(); ++i ) {
		auto res = ress.at(i);
		if ( ress.at(i-1) == ress.at(i+1) ) {
			// This is (probably) a saw move. Delegate checking whether there's enough room on the track to the length_track_rule.
			cerr << "WARNING: Move action embeds an in-place reversal (saw) at position " << i
				 << ", track " << res->toString() << ", id " << res->GetID() << ", with no explicit "
				 << "Setback/Walking action. This plan shape is deprecated and may be rejected in a "
				 << "future version - see doc/known-issue-plan-type.md." << endl;
			continue;
		}
		if ( state->GetOccupations(res).size() > 0 ) {
			cout << "Collission detected at position " << i << ", track " << res->toString() << ", id " << res->GetID()  << endl;
			return make_pair(false, "Track " + res->toString() + ", id " + res->GetID() + " is occupied.");
		}
	}
	return make_pair(true, "");
}

