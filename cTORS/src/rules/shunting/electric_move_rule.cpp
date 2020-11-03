#include "BusinessRules.h"

/*

Rule that verifies that shunting units which need electricity park only on electrified tracks.

*/

pair<bool, string> electric_move_rule::IsValid(State* state, Action* action) const {
	auto su = action->GetShuntingUnit();
	if (!su->NeedsElectricity()) return make_pair(true, "");
	Track* destination = nullptr;
	if (MoveAction* ma = dynamic_cast<MoveAction*>(action)) {
		destination = ma->GetDestinationTrack();
	} else if (ArriveAction* aa = dynamic_cast<ArriveAction*>(action)) {
		destination = aa->GetDestinationTrack();
	} else if (ExitAction* ea = dynamic_cast<ExitAction*>(action)) {
		destination = ea->GetDestinationTrack();
	} else {
		return make_pair(true, "");
	}
	if (!destination->isElectrified)
		return make_pair(false, "Destination track " + destination->toString() + " is not electrified.");
	return make_pair(true, "");
}

