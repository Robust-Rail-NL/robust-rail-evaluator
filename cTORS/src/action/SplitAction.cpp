#include "Action.h"
#include "State.h"

void SplitAction::Start(State* state) const {
	auto su = GetShuntingUnit();
	auto& suState = state->GetShuntingUnitState(su);
	auto track = suState.position;
	auto previous = suState.previous;
	int positionOnTrack = state->GetPositionOnTrack(su);
	bool front = suState.frontTrain == su->GetTrains().front();
	bool fromASide = track->IsASide(previous);
	state->RemoveShuntingUnit(su);
	state->AddShuntingUnitOnPosition(suA, track, previous, front ? suA->GetTrains().front() : suA->GetTrains().back(), positionOnTrack);
	state->AddShuntingUnitOnPosition(suB, track, previous, front ? suB->GetTrains().front() : suB->GetTrains().back(), positionOnTrack + (fromASide ? 1 : 0));
	
	state->AddActiveAction(suA, this);
	state->AddActiveAction(suB, this);
}

void SplitAction::Finish(State* state) const {
	state->RemoveActiveAction(suA, this);
	state->RemoveActiveAction(suB, this);
}

const string SplitAction::toString() const {
	return "SplitAction " + GetShuntingUnit()->toString() + " into " + suA->GetTrainString() + " and " +suB->GetTrainString();
}

void SplitActionGenerator::Generate(const State* state, list<const Action*>& out) const {
	//TODO employees
	auto& sus = state->GetShuntingUnits();
	for (auto su : sus) {
		auto& suState = state->GetShuntingUnitState(su);
		auto size = su->GetTrains().size();
		if (size <= 1 || suState.moving || suState.waiting || suState.HasActiveAction()) continue;
		auto duration = suState.frontTrain->GetType()->splitDuration;
		for(int splitPosition = 1; splitPosition < size; splitPosition++) {
			ShuntingUnit* suA = new ShuntingUnit(su->GetID(), vector<const Train*>(su->GetTrains().begin(), su->GetTrains().begin() + splitPosition));
			ShuntingUnit* suB = new ShuntingUnit(su->GetID()+1, vector<const Train*>(su->GetTrains().begin()+splitPosition, su->GetTrains().end()));
			SplitAction* splitAction = new SplitAction(su, suState.position, duration, suA, suB);
			out.push_back(splitAction);
		}
	}
}