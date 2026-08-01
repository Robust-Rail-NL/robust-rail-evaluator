#include "ShuntingUnit.h"

// Added by R.G.Kromes - 05/02/2025 : reverse the train unit orders, basically the order of front train
// follows the logic - lastly added one is the front train e.g. in scenarion.json
// [9404, 9404] -> front train is 9404
void ReverseTrainOrder(vector<Train>& out) {
	if(out.size() > 1)
	{
		vector<Train> copy_out(out);
		int index=0;
		for(auto t = copy_out.end()-1; t >= copy_out.begin(); --t){
			out[index] = *t;
			index++;
		}
	}
}

const vector<Train> ConvertPBTrains(const PBList<PBTrainUnit>& trains) {
	vector<Train> out;
	for(auto& train: trains) {
		out.push_back(Train(train));
	}
	ReverseTrainOrder(out);
	return out;
}

const vector<Train> ConvertPBTrains(const PBList<PB_HIP_TrainUnit>& trains) {
	vector<Train> out;
	for(auto& train: trains) {
		out.push_back(Train(train));
	}
	ReverseTrainOrder(out);
	return out;
}

const vector<Train> ConvertPBTrains(const PBList<PB_HIP_IncomingTrainUnit>& trains) {
	vector<Train> out;
	for(auto& incomingTrainUnit: trains) {
		out.push_back(Train(incomingTrainUnit.trainunit()));
	}
	ReverseTrainOrder(out);
	return out;
}

ShuntingUnit::ShuntingUnit(const PBTrainGoal& pb_tg)
	: ShuntingUnit(stoi(pb_tg.id()), ConvertPBTrains(pb_tg.members())) {}

ShuntingUnit::ShuntingUnit(const PB_HIP_TrainGoal& pb_tg)
	: ShuntingUnit(stoi(pb_tg.id()), ConvertPBTrains(pb_tg.members())) {}

ShuntingUnit::ShuntingUnit(const PB_HIP_TrainRequest& pb_tr)
	: ShuntingUnit(stoi(pb_tr.displayname()), ConvertPBTrains(pb_tr.trainunits())) {}

void ShuntingUnit::UpdateValues() {
	length = 0;
	needsElectricity = false;
	trainString = "";
	bool first = true;
	trainIDs = {};
	for (auto& t : trains) {
		length += t.GetType()->length;
		needsElectricity |= t.GetType()->needsElectricity;
		trainString += (t.toString() + (first ? "" : "-"));
		trainIDs.push_back(t.GetID()); // Probably here
	}
}

const Train* ShuntingUnit::GetTrainByID(int id) const {
	auto it = find_if(trains.begin(), trains.end(), [id](const Train& t) -> bool { return t.GetID() == id; });
	if(it == trains.end()) return nullptr;
	return &(*it);
}

int ShuntingUnit::GetTrainIndexByID(int id) const {
	auto it = find_if(trains.begin(), trains.end(), [id](const Train& t) -> bool { return t.GetID() == id; });
	if(it == trains.end()) return -1;
	return it - trains.begin();
}


bool ShuntingUnit::HasTrainByID(int id) const {
	return GetTrainByID(id) != nullptr;
}
	
bool ShuntingUnit::HasAnyTrainByIDs(vector<int> ids) const {
	for(auto& id: ids) {
		if(HasTrainByID(id)) return true;
	}
	return false;
}

int ShuntingUnit::GetSetbackTime(const Train* const frontTrain, bool normTime, bool walkTime, int setbackTime) const {
	if(normTime && trains.size() > 0) {
		setbackTime += frontTrain->GetType()->backNormTime;
	}
	if(walkTime) {
		for(auto t: trains) {
			setbackTime += t.GetType()->setbackTime;
		}
	}
	return setbackTime;
}

bool ShuntingUnit::MatchesShuntingUnit(const ShuntingUnit* su) const {
	auto& su1Trains = GetTrains();
	auto& su2Trains = su->GetTrains();
	if(su1Trains.size() != su2Trains.size()) return false;
	bool leftValid = true;
	bool rightValid = true;
	for (int i = 0; i < su1Trains.size(); i++) {
		auto& exp = su1Trains.at(i);
		auto& left = su2Trains.at(i);
		auto& right = su2Trains.at(su2Trains.size() - 1 - i);
		bool haveIDleft = (exp.GetID() != -1 && left.GetID() != -1);
		bool haveIDright = (exp.GetID() != -1 && right.GetID() != -1);
		if ((haveIDleft && exp.GetID() != left.GetID()) ||
			(!haveIDleft && exp.GetType()->displayName != left.GetType()->displayName))
			leftValid = false;
		if ((haveIDright && exp.GetID() != right.GetID()) ||
			(!haveIDright && exp.GetType()->displayName != right.GetType()->displayName))
			rightValid = false;	
		if(!leftValid && !rightValid) return false;
	}
	return true;
}

bool ShuntingUnit::MatchesTrainIDs(const vector<int>& ids, const vector<const TrainUnitType*>& types) const {
	auto& su1Trains = GetTrains();
	if(su1Trains.size() != ids.size()) return false;
	bool leftValid = true;
	bool rightValid = true;
	for (int i = 0; i < su1Trains.size(); i++) {
		auto exp = su1Trains.at(i).GetID();
		auto expType = su1Trains.at(i).GetType();
		auto left = ids.at(i);
		auto leftType = types.at(i);
		auto right = ids.at(ids.size() - 1 - i);
		auto rightType = types.at(ids.size() - 1 - i);
		bool haveIDleft = (exp != -1 && left != -1);
		bool haveIDright = (exp != -1 && right != -1);
		if ((haveIDleft && exp != left) ||
			(!haveIDleft && expType->displayName != leftType->displayName))
			leftValid = false;
		if ((haveIDright && exp != right) ||
			(!haveIDright && expType->displayName != rightType->displayName))
			rightValid = false;	
		if(!leftValid && !rightValid) return false;
	}
	return true;
}