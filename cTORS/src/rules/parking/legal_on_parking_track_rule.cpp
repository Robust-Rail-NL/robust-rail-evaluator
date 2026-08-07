#include "BusinessRules.h"

/**
 * Validates an EndMoveAction or WaitAction for the given state.
 * The EndMoveAction or WaitAction is invalid iff
 * 1. The current Track of the ShuntingUnit does not allow for parking.
 *
 * A MoveAction is never rejected by this rule. Arriving somewhere is not the
 * same as parking there: if the ShuntingUnit stays, the EndMoveAction or
 * WaitAction that follows is checked at exactly the position this rule cares
 * about, and if it leaves again — a departure moves onto the gateway and then
 * Exits — nothing was parked at all.
 *
 * This branch used to reject a non-step MoveAction whose destination forbids
 * parking. That was introduced in ae37897 (2021-06-01) together with MultiMove,
 * and was dormant while plans were replayed as single-destination Move actions,
 * which the branch exempted. ac85e3c (2025-02-04) acted on the "TODO change to
 * multi-move" in Plan.cpp and started building every plan movement as a
 * MultiMove, which made every plan-supplied move subject to the check. Since a
 * departure's final move lands on the gateway, and a gateway is normally marked
 * parkingAllowed=false because it is the connection to the main line, no plan
 * could depart a train on such a location — no plan for e.g. KleineBinckhorst
 * could be valid, in any release from v1.0.0 onwards.
 *
 * @return A pair describing 1) whether the action is valid, and 2) if not, why
 */
pair<bool, string> legal_on_parking_track_rule::IsValid(const State* state, const Action* action) const {
	if(!instanceof<EndMoveAction>(action) && !instanceof<WaitAction>(action))
		return make_pair(true, "");
	auto su = action->GetShuntingUnit();
	auto position = state->GetPosition(su);
	if (!position->parkingAllowed)
		return make_pair(false, "Parking is not allowed on track " + position->toString() + ".");
	return make_pair(true, "");
}

