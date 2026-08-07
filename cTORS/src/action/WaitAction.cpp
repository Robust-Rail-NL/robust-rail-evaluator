#include "Action.h"
#include "State.h"

void WaitAction::Start(State* state) const {
	state->SetWaiting(GetShuntingUnit(), true);
}

void WaitAction::Finish(State* state) const {
	auto su = GetShuntingUnit();
	if(state->HasShuntingUnit(su))
		state->SetWaiting(su, false);
}

const string WaitAction::toString() const {
	return "Wait " + su->toString() + " for " + to_string(duration) + " seconds";
}

const Action* WaitActionGenerator::Generate(const State* state, const SimpleAction& action) const {
	auto su = InitialCheck(state, action);
	if (state->IsWaiting(su)) throw InvalidActionException("The ShuntingUnit is already waiting.");
	// A wait replayed from a plan states its own duration. Honour it: stretching it
	// to the next queued event instead would consume the time the plan reserved for
	// the actions that follow, and there need not be an event to wait for at all -
	// a unit that simply stays put until the scenario ends has none.
	auto wait = dynamic_cast<const Wait*>(&action);
	if (wait != nullptr && wait->HasDuration())
		return new WaitAction(su, wait->GetDuration());
	auto e = state->PeekEvent();
	if(e == nullptr || e->GetTime() == state->GetTime()) throw InvalidActionException("There is nothing to wait for.");
	int dif = e->GetTime() - state->GetTime();
	//if (dif > 30) dif = 30;
	return new WaitAction(su, dif);
}

void WaitActionGenerator::Generate(const State* state, list<const Action*>& out) const {
	if(state->GetTime()==state->GetEndTime()) return;
	auto e = state->PeekEvent();
	if (e == nullptr || e->GetTime() == state->GetTime()) return;
	for (const auto& [su, suState] : state->GetShuntingUnitStates()) {
		if (suState.waiting || suState.moving || suState.HasActiveAction()) continue;
		out.push_back(Generate(state, Wait(su)));
	}
}