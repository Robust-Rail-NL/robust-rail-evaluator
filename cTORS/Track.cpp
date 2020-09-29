#include "Track.h"

Track::Track() : 
	id(""), type(TrackPartType::Railroad), length(0), name(""),
	sawMovementAllowed(false), parkingAllowed(false), 
	isElectrified(false), standingAllowed(false) {
}

Track::Track(const string& id, TrackPartType type, double length, const string& name, bool sawMovementAllowed,
	bool parkingAllowed, bool isElectrified, bool standingAllowed) :
	id(id), type(type), length(length),	name(name), sawMovementAllowed(sawMovementAllowed),
	parkingAllowed(parkingAllowed), isElectrified(isElectrified),
	standingAllowed(standingAllowed) {
}

Track::Track(const Track& track) :
	id(track.id), type(track.type), length(track.length),
	name(track.name), sawMovementAllowed(track.sawMovementAllowed),
	parkingAllowed(track.parkingAllowed), isElectrified(track.isElectrified),
	standingAllowed(track.standingAllowed), aSides(track.aSides),
	bSides(track.bSides), next(track.next) {
}

Track::~Track() { }

void Track::AssignNeighbors(vector<Track*> aside, vector<Track*> bside)
{
	if (type == TrackPartType::Railroad && (aside.size() != 1 || bside.size() != 1))
		throw InvalidLocationException("Track " + toString() + "(" + id + "): Railroad must have one track at each side");
	if (type == TrackPartType::Bumper && (aside.size() != 1 ||  bside.size() != 0))
		throw InvalidLocationException("Track " + toString() + "(" + id + "): Bumper must have one track at at most one side");
	//write error functions for Switch, EnglishSwitch, HalfEnglishSwitch, InterSection, Building
	aSides = aside;
	bSides = bside;
	if (type == TrackPartType::HalfEnglishSwitch) {
		next[aside[0]] = bside;
		next[aside[1]] = vector<Track*> { bside[1] };
	} else if (type == TrackPartType::InterSection) {
		next[aside[0]] = vector<Track*>{ bside[1] };
		next[aside[1]] = vector<Track*>{ bside[0] };
	} else {
		for (Track* t : aside)
			next[t] = bside;
	}
	if (type == TrackPartType::HalfEnglishSwitch) {
		next[bside[0]] = aside;
		next[bside[1]] = vector<Track*>{ aside[1] };
	} else if (type == TrackPartType::InterSection) {
		next[bside[0]] = vector<Track*>{ aside[1] };
		next[bside[1]] = vector<Track*>{ aside[0] };
	} else {
		for (Track* t : bside)
			next[t] = aside;
	}
}

bool Track::IsASide(Track* t) const {
	auto it = next.find(t);
	if(it != next.end())
		return it->second == aSides;
	return false; //Or should it check for tracks that are equal, but don't share the same reference?
}

bool Track::IsBSide(Track* t) const {
	auto it = next.find(t);
	if (it != next.end())
		return it->second == bSides;
	return false; //Or should it check for tracks that are equal, but don't share the same reference?
}

//Direction Track::GetDirectionByPrevious(Track* previous) const {
//	if (IsASide(previous))
//		return Direction::B;
//	return Direction::A;
//}

void from_json(const json& j, Track& t) {
	j.at("id").get_to(t.id);
	j.at("type").get_to(t.type);
	j.at("length").get_to(t.length);
	j.at("name").get_to(t.name);
	j.at("sawMovementAllowed").get_to(t.sawMovementAllowed);
	j.at("parkingAllowed").get_to(t.parkingAllowed);
	j.at("isElectrified").get_to(t.isElectrified);
	j.at("standingAllowed").get_to(t.standingAllowed);
}