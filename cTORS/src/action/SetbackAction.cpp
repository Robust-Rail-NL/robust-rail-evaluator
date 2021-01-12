#include "Action.h"
#include "State.h"

void SetbackAction::Start(State* state) const {
	const ShuntingUnit* su = GetShuntingUnit();
	const ShuntingUnitState& suState = state->GetShuntingUnitState(su);
	state->SetPrevious(su, suState.position->GetOppositeSide(suState.previous));
	state->SwitchFrontTrain(su);
	for (auto e : GetDrivers()) {
		//TODO
	}
	state->AddActiveAction(su, this);
}

void SetbackAction::Finish(State* state) const {
	state->RemoveActiveAction(su, this);
}

const string SetbackAction::toString() const {
	return "Setback " + GetShuntingUnit()->toString();
}


SetbackActionGenerator::SetbackActionGenerator(const json& params, const Location* location) : ActionGenerator(params, location) {
	params.at("constant_time").get_to(constantTime);
	params.at("default_time").get_to(defaultTime);
	params.at("norm_time").get_to(normTime);
	params.at("walk_time").get_to(walkTime);
}

int SetbackActionGenerator::GetDuration(const State* state, const ShuntingUnit* su, int numDrivers) const {
	if(defaultTime) return su->GetSetbackTime(state->GetFrontTrain(su), true, numDrivers < 2);
	return su->GetSetbackTime(state->GetFrontTrain(su), normTime, walkTime, constantTime);
}

void SetbackActionGenerator::Generate(const State* state, list<const Action*>& out) const {
	if(state->GetTime()==state->GetEndTime()) return;
	auto& sus = state->GetShuntingUnits();
	bool driver_mandatory = false;//TODO get value from config
	for (auto su : sus) {
		const ShuntingUnitState& suState = state->GetShuntingUnitState(su);
		if (!suState.moving || suState.waiting || suState.HasActiveAction() || suState.inNeutral) continue;
		vector<const Employee*> drivers;
		if (driver_mandatory) {
			//TODO
		}
		int duration = GetDuration(state, su, drivers.size());
		Action* a = new SetbackAction(su, drivers, duration);
		out.push_back(a);		
	}
}