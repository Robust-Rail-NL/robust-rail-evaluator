#include "BusinessRules.h"

/*

Rule that verifies that performing a setback action on a shunting unit is 
allowed on the track where the shunting unit is at.

*/

pair<bool, string> setback_track_rule::IsValid(State* state, Action* action) const {
	if (SetbackAction* sa = dynamic_cast<SetbackAction*>(action)) {
		auto su = action->GetShuntingUnit();
		auto track = state->GetPosition(su);
		if (!track->sawMovementAllowed)
			return make_pair(false, "Saw movements are not allowed on track " + track->toString()+".");
	}
	return make_pair(true, "");
}

