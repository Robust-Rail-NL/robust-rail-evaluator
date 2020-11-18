#include "Action.h"
#include "State.h"

void MoveAction::Start(State* state) const {
	state->AddActiveAction(su, this);
	auto track = GetDestinationTrack();
	auto previous = GetPreviousTrack();
	state->MoveShuntingUnit(su, track, previous);
	state->ReserveTracks(GetReservedTracks());
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
	auto& sus = state->GetShuntingUnits();
	for (auto su : sus) {
		if (!state->IsMoving(su) || state->HasActiveAction(su)) continue;
		auto track = state->GetPosition(su);
		auto previous = state->GetPrevious(su);
		vector<const Track*> tracks{ track };
		GenerateMovesFrom(su, tracks, previous, location->GetDurationByType(track), out);
	}
}