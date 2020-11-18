#include "State.h"

State::State(const Scenario& scenario, const vector<Track*>& tracks) {
	time = scenario.GetStartTime();
	startTime = scenario.GetStartTime();
	endTime = scenario.GetEndTime();
	incomingTrains = scenario.GetIncomingTrains();
	outgoingTrains = scenario.GetOutgoingTrains();
	employees = scenario.GetEmployees();
	for (auto in : incomingTrains)
		AddEvent(in);
	for (auto out : outgoingTrains)
		AddEvent(out);
	changed = true;
	for(auto track : tracks) trackStates[track];
}

State::~State() {
	DELETE_VECTOR(shuntingUnits);
}

void State::SetTime(int time) {
	if (time != this->time) {
		changed = true;
		this->time = time;
	}
}

const Event* State::PeekEvent() const
{
	if (events.size() == 0)
		return nullptr;
	return events.top();
}

const Event* State::PopEvent()
{
	auto evnt = events.top();
	events.pop();
	return evnt;
}

void State::AddEvent(const Incoming *in) {
	Event* event = new Event(in->GetTime(), nullptr, EventType::IncomingTrain);
	events.push(event);
	debug_out("Push event " << event->toString() << " at T=" << to_string(event->GetTime()));
}

void State::AddEvent(const Outgoing *out) {
	Event* event = new Event(out->GetTime(), nullptr, EventType::OutgoingTrain);
	events.push(event);
	debug_out("Push event " << event->toString() << " at T=" << to_string(event->GetTime()));
}

void State::AddEvent(const Action* action) {
	Event* event = new Event(time + action->GetDuration(), action, EventType::ActionFinish);
	events.push(event);
	debug_out("Push event " << event->toString() << " at T=" << to_string(event->GetTime()));
}

void State::StartAction(const Action* action) {
	if (action == nullptr) return;
	changed = true;
	action->Start(this);
	AddEvent(action);
}

void State::FinishAction(const Action* action) {
	if (action == nullptr) return;
	changed = true;
	action->Finish(this);
}

void State::AddShuntingUnit(const ShuntingUnit* su, const Track* track, const Track* previous, const Train* frontTrain) {
	shuntingUnits.push_back(su);
	shuntingUnitStates.emplace(su, ShuntingUnitState(track, previous, frontTrain));
	for(auto train: su->GetTrains()) trainStates[train];
	OccupyTrack(su, track, previous);
}

void State::MoveShuntingUnit(const ShuntingUnit* su, const Track* to, const Track* previous) {
	RemoveOccupation(su);
	OccupyTrack(su, to, previous);
}

void State::RemoveOccupation(const ShuntingUnit* su) {
	const Track* current = GetPosition(su);
	auto& occ = trackStates[current].occupations;
	auto it = find(occ.begin(), occ.end(), su);
	if (it != occ.end()) {
		occ.remove(*it);
	}
}

void State::OccupyTrack(const ShuntingUnit* su, const Track* track, const Track* previous) {
	if(track->IsASide(previous))
		trackStates[track].occupations.push_front(su);
	else
		trackStates[track].occupations.push_back(su);
	SetPosition(su, track);
	SetPrevious(su, previous);
}

void State::ReserveTracks(const vector<const Track*>& tracks) {
	for (auto t : tracks)
		ReserveTrack(t);
}

void State::ReserveTracks(const list<const Track*>& tracks) {
	for (auto t : tracks)
		ReserveTrack(t);
}

void State::FreeTracks(const vector<const Track*>& tracks) {
	for (auto t : tracks)
		FreeTrack(t);
}

void State::FreeTracks(const list<const Track*>& tracks) {
	for (auto t : tracks)
		FreeTrack(t);
}

void State::RemoveIncoming(const Incoming* incoming) {
	auto it = find(incomingTrains.begin(), incomingTrains.end(), incoming);
	if(it != incomingTrains.end())
		incomingTrains.erase(it);
}

void State::RemoveOutgoing(const Outgoing* outgoing) {
	auto it = find(outgoingTrains.begin(), outgoingTrains.end(), outgoing);
	if (it != outgoingTrains.end())
		outgoingTrains.erase(it);
}

void State::RemoveShuntingUnit(const ShuntingUnit* su) {
	RemoveOccupation(su);
	shuntingUnitStates.erase(su);
	for(auto train: su->GetTrains()) trainStates.erase(train);
	auto it = find(shuntingUnits.begin(), shuntingUnits.end(), su);
	if (it != shuntingUnits.end())
		shuntingUnits.erase(it);
}

bool State::IsActive() const {
	for (auto& su : shuntingUnits) {
		if (HasActiveAction(su) || IsWaiting(su)) return true;
	}
	return false;
}

bool State::IsAnyInactive() const {
	if (shuntingUnits.size() == 0) return false;
	for (auto& su : shuntingUnits) {
		if (!HasActiveAction(su) && !IsWaiting(su)) return true;
	}
	return false;
}

void State::RemoveActiveAction(const ShuntingUnit* su, const Action* action) {
	auto& lst = shuntingUnitStates.at(su).activeActions;
	auto it = find_if(lst.begin(), lst.end(), [action](const Action* a) -> bool { return *a == *action; } );
	if (it != lst.end()) {
		delete* it;
		lst.remove(*it);
	}
}

void State::addTasksToTrains(const map<const Train*, vector<Task>>& tasks) {
	for (auto& it : tasks) {
		for (auto& task : it.second) {
			AddTaskToTrain(it.first, task);
		}
	}
}

void State::RemoveTaskFromTrain(const Train* tu, const Task& task) {
	auto& lst = trainStates.at(tu).tasks;
	auto it = find(lst.begin(), lst.end(), task);
	if (it != lst.end())
		lst.erase(it);
}

void State::RemoveActiveTaskFromTrain(const Train* tu, const Task* task) {
	auto& lst = trainStates.at(tu).activeTasks;
	auto it = find(lst.begin(), lst.end(), task);
	if (it != lst.end())
		lst.erase(it);
}

int State::GetPositionOnTrack(const ShuntingUnit* su) const {
	auto& sus = GetOccupations(GetPosition(su));
	auto it = find(sus.begin(), sus.end(), su);
	return static_cast<int>(distance(sus.begin(), it));
}

bool State::CanMoveToSide(const ShuntingUnit* su, const Track* side) const {
	auto park = GetPosition(su);
	auto& sus = GetOccupations(park);
	if (park->IsASide(side))
		return sus.front() == su;
	return sus.back() == su;
}

const vector<const Train*> State::GetTrainUnitsInOrder(const ShuntingUnit* su) const {
	auto trains = su->GetTrains();
	auto previous = GetPrevious(su);
	auto current = GetPosition(su);
	if (previous == nullptr || current->IsASide(previous))
		return trains;
	vector<const Train*>reverse (trains.rbegin(), trains.rend());
	return reverse;
}

void State::SwitchFrontTrain(const ShuntingUnit* su) {
	auto front = su->GetTrains().front();
	auto back = su->GetTrains().back();
	SetFrontTrain(su, GetFrontTrain(su) == front ? back : front);
}