#include "Action.h"
#include "State.h"

void MoveAction::Start(State* state) const {
	state->AddActiveAction(su, this);
	auto destination = GetDestinationTrack();
	auto& suState = state->GetShuntingUnitState(su);
	if(suState.inNeutral && suState.position->IsSameSide(suState.previous, destination))
		state->SwitchFrontTrain(su);
	auto previous = GetPreviousTrack();
	state->MoveShuntingUnit(su, destination, previous);
	state->ReserveTracks(GetReservedTracks());
	state->SetInNeutral(su, false);
}

void MoveAction::Finish(State* state) const {
	state->RemoveActiveAction(su, this);
	state->FreeTracks(GetReservedTracks());
}

const string MoveAction::toString() const {
	return "Move " + su->toString() + " to " + GetDestinationTrack()->toString();
}

MoveActionGenerator::MoveActionGenerator(const json& params, const Location* location) : ActionGenerator(params, location) {
	params.at("no_routing_duration").get_to(noRoutingDuration);
	params.at("constant_time").get_to(constantTime);
	params.at("default_time").get_to(defaultTime);
	params.at("norm_time").get_to(normTime);
	params.at("walk_time").get_to(walkTime);
}

void MoveActionGenerator::GenerateMovesFrom(const ShuntingUnit* su, const vector<const Track*> &tracks,
									const Track* previous, int duration, list<const Action*> &out) const {
	auto track = tracks.back();
	vector<const Track*> nexts;
	if (previous == nullptr)
		nexts = track->GetNeighbors();
	else
		nexts = track->GetNextTrackParts(previous);
	for (auto next : nexts) {
		int newDuration = duration + location->GetDurationByType(next);
		vector<const Track*> newTracks = tracks;
		newTracks.push_back(next);
		if (next->GetType() == TrackPartType::Railroad) {
			Action* a = new MoveAction(su, newTracks, newDuration);
			out.push_back(a);
		} else {
			GenerateMovesFrom(su, newTracks, track, newDuration, out);
		}
	}
}

void MoveActionGenerator::Generate(const State* state, list<const Action*>& out) const {
	if(state->GetTime()==state->GetEndTime()) return;
	auto& sus = state->GetShuntingUnits();
	for (auto su : sus) {
		auto& suState = state->GetShuntingUnitState(su);
		if (!suState.moving || suState.HasActiveAction()) continue;
		auto track = suState.position;
		auto previous = suState.inNeutral ? nullptr : suState.previous;
		vector<const Track*> tracks{ track };
		GenerateMovesFrom(su, tracks, previous, location->GetDurationByType(track), out);
	}
}