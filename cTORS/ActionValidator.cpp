#include "Action.h"
#include "BusinessRules.h"

#ifndef ADD_VALIDATOR
#define ADD_VALIDATOR(validator) \
if (config->IsBusinessRuleActive(#validator)) { \
	validators.push_back(new validator(config)); \
}
#endif

ActionValidator::~ActionValidator() {
	validators.clear();
}

void ActionValidator::AddValidators() {
	//Arrival and departure
	ADD_VALIDATOR(in_correct_time_rule)
	ADD_VALIDATOR(out_correct_time_rule)
	ADD_VALIDATOR(out_correct_order_rule)

	//Track occupation
	ADD_VALIDATOR(single_move_track_rule)
	ADD_VALIDATOR(length_track_rule)
	ADD_VALIDATOR(blocked_track_rule)
	ADD_VALIDATOR(blocked_first_track_rule)

	//Parking
	ADD_VALIDATOR(electric_track_rule)
	ADD_VALIDATOR(legal_on_parking_track_rule)
	ADD_VALIDATOR(legal_on_setback_track_rule)

	//Shunting
	ADD_VALIDATOR(electric_move_rule)
	ADD_VALIDATOR(setback_once_rule)
	ADD_VALIDATOR(setback_track_rule)

	//Service tasks
	ADD_VALIDATOR(correct_facility_rule)
	ADD_VALIDATOR(mandatory_service_task_rule)
	ADD_VALIDATOR(optional_service_task_rule)
	ADD_VALIDATOR(understaffed_rule)

	//Facility
	ADD_VALIDATOR(available_facility_rule)
	ADD_VALIDATOR(capacity_facility_rule)
	ADD_VALIDATOR(disabled_facility_rule)

	//Employee

	//Combine and Split

	//Matching
}

pair<bool, string> ActionValidator::IsValid(State* state, Action* action) const {
	for (auto rule : validators) {
		auto result = rule->IsValid(state, action);
		if (!result.first)
			return result;
	}
	return make_pair(true,"");
}

void ActionValidator::FilterValid(State* state, list<Action*>& actions) const {
	actions.remove_if([this, state](Action*& a) {
		auto result = IsValid(state, a);
		if (!result.first) {
			//if verbose:
			debug_out("Invalid action: " + a->toString() + " - " + result.second + ".");
			delete a;
			return true;
		}
		return false;
	});
}


