#include "BusinessRules.h"

/*

Rule that verifies that a setback action is not performed on a shunting unit 
which is already in a neutral state. A shunting unit is in a neutral state if a 
setback or service action is performed.

*/

pair<bool, string> setback_once_rule::IsValid(State* state, Action* action) const {
	if (SetbackAction* sa = dynamic_cast<SetbackAction*>(action)) {
		auto su = action->GetShuntingUnit();
		auto dir = state->GetPrevious(su);
		if(dir == nullptr)
			return make_pair(false, "Shunting Unit " + su->toString() + " is already set back.");
	}
	return make_pair(true, "");
}

