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
	// This works because saw moves are not a single multi-move action, but always have a walking
	// action in between. So all tracks we run through are fully traversed by the ShuntingUnit.
	for ( size_t i = 1; i + 1 < ress.size(); ++i ) {
		auto res = ress.at(i);
		if ( ress.at(i-1) == ress.at(i+1) ) {
			// This is (probably) a saw move. Delegate checking whether there's enough room on the track to the length_track_rule.
			cout << "Ignoring saw move at position " << i << ", track " << res->toString() << ", id " << res->GetID() << endl;
			continue;
		}
		if ( state->GetOccupations(res).size() > 0 ) {
			cout << "Collission detected at position " << i << ", track " << res->toString() << ", id " << res->GetID()  << endl;
			return make_pair(false, "Track " + res->toString() + ", id " + res->GetID() + " is occupied.");
		}
	}
	return make_pair(true, "");
}

