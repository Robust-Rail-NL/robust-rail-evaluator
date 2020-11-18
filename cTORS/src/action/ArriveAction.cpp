#include "Action.h"
#include "State.h"

void ArriveAction::Start(State* state) const {
	const Track* parkingTrack = incoming->GetParkingTrack();
	const Track* sideTrack = incoming->GetSideTrack();
	const ShuntingUnit* su = GetShuntingUnit();

	state->AddShuntingUnit(su, parkingTrack, sideTrack);
	if (!incoming->IsInstanding())
		state->ReserveTracks(vector({ parkingTrack, sideTrack }));
	state->addTasksToTrains(incoming->GetTasks());
	state->RemoveIncoming(incoming);
	state->AddActiveAction(su, this);
	state->SetWaiting(su, false);
}

void ArriveAction::Finish(State* state) const {
	state->RemoveActiveAction(su, this);
	if (!incoming->IsInstanding())
		state->FreeTracks( vector({incoming->GetParkingTrack(), incoming->GetSideTrack() }));
}

const string ArriveAction::toString() const {
	return "Arrive " + su->toString() + " at " + incoming->GetParkingTrack()->toString() + " at T" + to_string(incoming->GetTime());
}

void ArriveActionGenerator::Generate(const State* state, list<const Action*>& out) const {
	auto incoming = state->GetIncomingTrains();
	if (state->GetTime() == 0) { //First handle Instanding Units
		int min = 1024;
		for (auto in : incoming) {
			if (in->IsInstanding() && in->GetTime() == 0 && in->GetStandingIndex() < min) {
				min = in->GetStandingIndex();
			}
		}
		for (auto in : incoming) {
			if (in->IsInstanding() && in->GetTime() == 0 && in->GetStandingIndex() == min) {
				Action* a = new ArriveAction(in->GetShuntingUnit(), 0, in);
				out.push_back(a);
			}
		}
	} 
	// Then handle the rest
	for (auto in : incoming) {
		if (!in->IsInstanding() && state->GetTime() >= in->GetTime()) {
			Action* a = new ArriveAction(in->GetShuntingUnit(), 0, in);
			out.push_back(a);
		}
	}
}