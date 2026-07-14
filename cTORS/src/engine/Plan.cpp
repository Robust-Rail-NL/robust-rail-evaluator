#include "Plan.h"

int POSAction::newUID{0};

const vector<int> GetTrainIDs(const PBList<string> &pb_train_ids)
{
    vector<string> trainIDs(pb_train_ids.begin(), pb_train_ids.end());
    vector<int> trains(trainIDs.size());
    transform(trainIDs.begin(), trainIDs.end(), trains.begin(), [](const string &s) -> int
              { return stoi(s); });
    return trains;
}

POSAction &POSAction::operator=(const POSAction &pa)
{
    if (this != &pa)
    {
        id = pa.id;
        suggestedStart = pa.suggestedStart;
        suggestedEnd = pa.suggestedEnd;
        minDuration = pa.minDuration;
        delete action;
        action = pa.action->Clone();
    }
    return *this;
}

static string formatExitTrainError(const int &track_exiting_train_ID, const bool &isTrainMatched, const int &actionStartTime, const int &actionEndTime, const int &trainMustDepartTime)
{
    std::ostringstream oss;
    oss << "\n+---------------------------+\n";
    oss << "|     🚨 ERROR OCCURRED     |\n";
    oss << "+---------------------------+\n";
    oss << "| Tracked Train     : " << track_exiting_train_ID << "\n";
    oss << "| Is Train matched  : " << (isTrainMatched ? "Yes" : "No") << "\n";
    oss << "| Action start time : " << actionStartTime << "\n";
    oss << "| Action end time   : " << actionEndTime << "\n";
    oss << "| Trains's departure: " << trainMustDepartTime << "\n";
    oss << "+---------------------------+\n";
    oss << "| Error Suspected:  \n";
    if (trainMustDepartTime >= actionStartTime || trainMustDepartTime <= actionEndTime)
    {
        oss << "| Trains's departure mismatch " << "\n";
        oss << "| with Action start/end time" << "\n";
    }
    else
    {
        oss << "| Probably train IDs cannot be found " << "\n";
    }
    oss << "+---------------------------+\n";
    return oss.str();
}

// Fixed to enable multi move actions
POSAction POSAction::CreatePOSAction(const Location *location, const Scenario *scenario, const PBAction &pb_action)
{

    string jsonResult;
    google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(pb_action, &jsonResult);
    if (!status.ok())
    {
        std::cerr << "Failed to convert protobuf to JSON: " << status.ToString() << std::endl;
    }

    int suggestedStartingTime = pb_action.suggestedstartingtime();
    int suggestedEndingTime = pb_action.suggestedfinishingtime();
    int minDuration = pb_action.minimumduration();

    auto &_pb_trainUnitIDs = pb_action.trainunitids();

    vector<int> trainIDs = GetTrainIDs(pb_action.trainunitids());

    SimpleAction *action;

    if (pb_action.has_movement())
    {
        auto track_id = *(pb_action.movement().path().end() - 1);
        const Track *destination = location->GetTrackByID(to_string(track_id));
        // action = new Move(trainIDs, destination); // TODO change to multi-move

        vector<int> trackIDs_mult(pb_action.movement().path().begin(), pb_action.movement().path().end());

        vector<string> tracks(trackIDs_mult.size());
        transform(trackIDs_mult.begin(), trackIDs_mult.end(), tracks.begin(), [](const int &s) -> string
                  { return to_string(s); });

        action = new MultiMove(trainIDs, tracks);
    }
    else if (pb_action.has_task())
    {
        auto &taskType = pb_action.task().type();
        if (taskType.has_predefined())
        {
            auto task = taskType.predefined();
            switch (task)
            {
            case PBPredefinedTaskType::Arrive:
                action = new Arrive(scenario->GetIncomingByTrainIDs(trainIDs));
                break;
            case PBPredefinedTaskType::Exit:
            {
                // Delta time to allow delays of the departure time

                vector<const TrainUnitType *> types;
                for (int id : trainIDs)
                    types.push_back(scenario->GetTrainByID(id)->GetType());
                int start = pb_action.suggestedstartingtime();
                int end = pb_action.suggestedfinishingtime();

                auto &outgoingTrains = scenario->GetOutgoingTrains();

                map<int, bool> track_exiting_trains(scenario->track_exiting_trains.begin(), scenario->track_exiting_trains.end());

                // If a delay was introduced the actions's suggested finsih (departure time) is increased by this delay
                if (scenario->GetDepartureDelay() != 0)
                {
                    end = end + scenario->GetDepartureDelay();
                    start = start - scenario->GetDepartureDelay();
                }

                auto it = find_if(outgoingTrains.begin(), outgoingTrains.end(),
                                  [start, end, &trainIDs, &types, &track_exiting_trains](const Outgoing *out) -> bool
                                  {
                                      return out->GetTime() >= start &&
                                             out->GetTime() <= end &&
                                             out->GetShuntingUnit()->MatchesTrainIDs(trainIDs, types) &&
                                             track_exiting_trains[out->GetID()] == false;
                                  });
                if (it == outgoingTrains.end())
                {
                    vector<string> errorMessages;
                    const string pathToLocationFile = location->GetLocationFilePath();

                    for (const Outgoing *train : outgoingTrains)
                    {
                        // auto trains = train->GetShuntingUnit()->GetTrains();
                        // trains.at(0)

                        string errorMessage = formatExitTrainError(train->GetID(), train->GetShuntingUnit()->MatchesTrainIDs(trainIDs, types), start, end, train->GetTime());
                        errorMessages.push_back(errorMessage);
                        // cout << "Train: " << train->GetID() << " Tracked Trains: " << track_exiting_trains[train->GetID()] << " Matcing : " << train->GetShuntingUnit()->MatchesTrainIDs(trainIDs, types) << "Action Start Time: " << start << "Action End Time: " << end << " Train Start/End Time: " << train->GetTime()<< endl;
                    }

                    if (!pathToLocationFile.empty())
                    { // That means that the EVAL_AND_STORE mode was called
                        LocationEngine engine(pathToLocationFile);
                        auto state = engine.StartSession(*scenario);
                        state->file.open(scenario->GetEvaluatiorStoragePath());
                        for (string errMessage : errorMessages)
                            state->file << errMessage;
                        state->file << "The plan is not valid";
                        state->file.close();
                    }
                    // throw invalid_argument("Outgoing Train with ids " + Join(outgoingTrains.begin(), outgoingTrains.end(), ", ") + " does not exist.");
                    throw invalid_argument(Join(errorMessages.begin(), errorMessages.end(), " \n\n"));
                }

                action = new Exit(trainIDs, (*it)->GetID(), (*it)->IsInstanding());

                if (scenario->track_exiting_trains.find((*it)->GetID()) != scenario->track_exiting_trains.end())
                {
                    scenario->UpdateTrackExitingTrains((*it)->GetID(), true);
                }

                break;
            }
            case PBPredefinedTaskType::Split:
            {
                // TODO how is the split action defined in protobuf? Current implementation: store the IDs of the first train.
                vector<int> firstTrainIDs = GetTrainIDs(pb_action.task().trainunitids());

                // vector<int> copy_firstTrainIDs(firstTrainIDs.end()-1, firstTrainIDs.begin());

                // int index = 0;
                // for(auto id : copy_firstTrainIDs)
                // {
                //     firstTrainIDs[index] = id;
                //     index++;
                // }

                int splitIndex = 1;
                for (auto &id : firstTrainIDs)
                {
                    auto it = find(trainIDs.begin(), trainIDs.end(), id);
                    if (it == trainIDs.end())
                        throw invalid_argument("Train with id " + to_string(id) + " not found in shunting unit " + Join(trainIDs.begin(), trainIDs.end(), ","));
                    int ix = it - trainIDs.begin();
                    splitIndex = ix + 1 > splitIndex ? ix + 1 : splitIndex;
                }

                action = new Split(trainIDs, splitIndex);

                break;
            }
            case PBPredefinedTaskType::Combine:
            {
                // TODO how is the combine action defined in protobuf? Current implementation: store the IDs of the second train.
                auto secondTrainIDs = GetTrainIDs(pb_action.task().trainunitids());
                action = new Combine(trainIDs, secondTrainIDs);

                break;
            }
            case PBPredefinedTaskType::Walking:
                action = new Setback(trainIDs);
                break;
            case PBPredefinedTaskType::Break:
                action = new Wait(trainIDs);
                break;
            case PBPredefinedTaskType::BeginMove: // TEMP
                action = new BeginMove(trainIDs);
                break;                          // TEMP
            case PBPredefinedTaskType::EndMove: // TEMP
                action = new EndMove(trainIDs);
                break; // TEMP
            case PBPredefinedTaskType::NonService:
                throw invalid_argument("Non-service tasks not implemented.");
            case PBPredefinedTaskType::Move:
                throw invalid_argument("Move task type not used. Use POSMovement instead.");
            default:
                throw invalid_argument("Invalid predefined task type " + to_string(task));
            }
        }
        else
        {
            const Train *train = scenario->GetTrainByID(stoi(pb_action.task().trainunitids().at(0)));
            auto tasks = scenario->GetTasksForTrain(train);
            auto taskTypeString = taskType.other();
            auto it = find_if(tasks.begin(), tasks.end(), [taskTypeString](auto &t) -> bool
                              { return t.taskType == taskTypeString; });
            if (it == tasks.end())
                throw invalid_argument("Could not find task " + taskTypeString + " for train " + train->toString());
            const Facility *facility = location->GetFacilityByID(pb_action.task().facilities().at(0).id());
            action = new Service(trainIDs, *it, *train, facility);
        }
    }
    else
    {
        action = new Wait(trainIDs);
    }
    return POSAction(suggestedStartingTime, suggestedEndingTime, minDuration, action);
}

void POSAction::Serialize(const LocationEngine &engine, const State *state, PBAction *pb_action) const
{
    pb_action->set_suggestedstartingtime(suggestedStart);
    pb_action->set_suggestedfinishingtime(suggestedEnd);
    pb_action->set_minimumduration(minDuration);
    debug_out("Serialize train ids");
    for (auto &t : action->GetTrainIDs())
    {
        *(pb_action->add_trainunitids()) = to_string(t);
    }
    if (instanceof<Move>(action))
    {
        debug_out("Serialize move action");
        auto move = dynamic_cast<const Move *>(action);
        auto pb_move = pb_action->mutable_movement();
        auto path = engine.GetPath(state, *move);
        auto su = state->GetShuntingUnitByTrainIDs(move->GetTrainIDs());
        for (auto t : path.route)
        {
            pb_move->add_path(stoi(t->GetID()));
        }
        auto current = state->GetPosition(su);
        auto previous = state->GetPrevious(su);
        auto destination = engine.GetLocation().GetTrackByID(move->GetDestinationID());
        auto prev_destination = prev(path.route.back());
        pb_move->set_fromside(current->IsASide(previous) ? PBSide::B : PBSide::A);
        pb_move->set_toside(destination->IsASide(prev_destination) ? PBSide::A : PBSide::B);
        pb_move->set_order(0);
    }
    else if (instanceof<Wait>(action))
    {
        auto pb_wait = pb_action->mutable_break_();
    }
    else
    {
        auto pb_task = pb_action->mutable_task();
        auto pb_task_type = pb_task->mutable_type();
        if (instanceof<Service>(action))
        {
            auto service = dynamic_cast<const Service *>(action);
            auto facility = engine.GetLocation().GetFacilityByID(service->GetFacilityID());
            pb_task_type->set_other(service->GetTask().taskType);
            pb_task->set_location(stoi(facility->GetTracks().at(0)->GetID()));
            auto pb_facility = pb_task->add_facilities();
            pb_facility->set_id(facility->GetID());
            pb_task->add_trainunitids(to_string(service->GetTrain().GetID()));
        }
        else if (instanceof<Split>(action))
        {
            pb_task_type->set_predefined(PBPredefinedTaskType::Split);
            auto split = dynamic_cast<const Split *>(action);
            debug_out("Serialize split action " + split->toString() + " #trains: " + to_string(split->GetTrainIDs().size()));
            // TODO how is the split action defined in protobuf? Current implementation: store the IDs of the first train.
            for (int i = 0; i < split->GetSplitIndex(); i++)
            {
                debug_out("Serialize split action: " + to_string(i));
                pb_task->add_trainunitids(to_string(split->GetTrainIDs().at(i)));
            }
        }
        else if (instanceof<Combine>(action))
        {
            pb_task_type->set_predefined(PBPredefinedTaskType::Combine);
            auto combine = dynamic_cast<const Combine *>(action);
            // TODO how is the combine action defined in protobuf? Current implementation: store the IDs of the second train.
            for (auto &t : combine->GetSecondTrainIDs())
                pb_task->add_trainunitids(to_string(t));
        }
        else if (instanceof<Setback>(action))
        {
            pb_task_type->set_predefined(PBPredefinedTaskType::Walking);
        }
        else if (instanceof<Arrive>(action))
        {
            pb_task_type->set_predefined(PBPredefinedTaskType::Arrive);
        }
        else if (instanceof<Exit>(action))
        {
            pb_task_type->set_predefined(PBPredefinedTaskType::Exit);
        }
        else if (instanceof<BeginMove>(action))
        {                                                                  // TEMP
            pb_task_type->set_predefined(PBPredefinedTaskType::BeginMove); // TEMP
        }
        else if (instanceof<EndMove>(action))
        {                                                                // TEMP
            pb_task_type->set_predefined(PBPredefinedTaskType::EndMove); // TEMP
        }
        else
        {
            throw invalid_argument("Action serialization not implemented for action " + action->toString());
        }
    }
}

POSPlan POSPlan::CreatePOSPlan(const Location *location, const Scenario *scenario, const PBPOSPlan &pb_plan)
{

    vector<PBAction> pb_actions(pb_plan.actions().begin(), pb_plan.actions().end());
    vector<POSAction> actions;

    for (PBAction _action : pb_actions)
    {
        string jsonResult;
        google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(_action, &jsonResult);
        if (!status.ok())
        {
            std::cerr << "Failed to convert protobuf to JSON: " << status.ToString() << std::endl;
        }
    }

    transform(pb_actions.begin(), pb_actions.end(), back_inserter(actions),
              [location, scenario](const PBAction &pba) -> const POSAction
              { return POSAction::CreatePOSAction(location, scenario, pba); });

    return POSPlan(actions);
}

void POSPlan::Serialize(LocationEngine &engine, const Scenario &scenario, PBPOSPlan *pb_plan) const
{
    auto state = engine.StartSession(scenario);
    auto it = actions.begin();
    while (it != actions.end())
    {
        try
        {
            std::cout << "Serializing T=" + to_string(state->GetTime()) + ". A=" + it->GetAction()->toString() + " at T=" + to_string(it->GetSuggestedStart()) + "." + "\n";
            engine.Step(state);
            debug_out("Finished Step Update [T=" + to_string(state->GetTime()) + "].");
            if (state->GetTime() >= it->GetSuggestedStart())
            {

                if (instanceof<Wait>(it->GetAction()))
                {
                    // Added by R.G. Kromes:
                    // needed to be able to apply wait action from HIP plans
                    auto pb_action = pb_plan->add_actions();
                    debug_out("Serialize action");
                    it->Serialize(engine, state, pb_action);
                    debug_out("Finish Serialize action");
                    // SKIP
                }
                else if (true || (!instanceof<BeginMove>(it->GetAction()) && !instanceof<EndMove>(it->GetAction())))
                {
                    auto pb_action = pb_plan->add_actions();
                    debug_out("Serialize action");
                    it->Serialize(engine, state, pb_action);
                    debug_out("Finish Serialize action");
                }
                debug_out("Apply action");
                engine.ApplyAction(state, *(it->GetAction()));
                debug_out("End action");
                it++;
            }
        }
        catch (ScenarioFailedException e)
        {
            cout << "Scenario failed.\n";
            pb_plan->set_feasible(false);
            break;
        }
    }
    pb_plan->set_feasible(state->GetShuntingUnits().size() == 0);
    engine.EndSession(state);
}

void POSPlan::SerializeToFile(LocationEngine &engine, const Scenario &scenario, const string &outfile) const
{
    PBPOSPlan pb_plan;
    debug_out("Start Serializing plan.")
        Serialize(engine, scenario, &pb_plan);
    debug_out("End Serializing plan.")
        parse_pb_to_json(outfile, pb_plan);
}

void RunResult::Serialize(LocationEngine &engine, PBRun *pb_run) const
{
    pb_run->set_location(location);
    scenario.Serialize(pb_run->mutable_scenario());
    plan.Serialize(engine, scenario, pb_run->mutable_plan());
    pb_run->set_feasible(feasible);
}

void RunResult::SerializeToFile(LocationEngine &engine, const string &outfile) const
{
    PBRun pb_run;
    Serialize(engine, &pb_run);
    parse_pb_to_json(outfile, pb_run);
}

RunResult *RunResult::CreateRunResult(const Engine &engine, const PBRun &pb_run)
{
    string locationString = pb_run.location();
    auto &location = engine.GetLocation(locationString);
    return CreateRunResult(&location, pb_run);
}

RunResult *RunResult::CreateRunResult(const Location *location, const PBRun &pb_run)
{
    Scenario scenario = Scenario(pb_run.scenario(), *location);

    POSPlan plan = POSPlan::CreatePOSPlan(location, &scenario, pb_run.plan());
    bool feasible = pb_run.feasible();
    return new RunResult(location->GetLocationFilePath(), scenario, plan, feasible);
}

PBAction RunResult::CreateBeginMoveAction(PB_HIP_Action &pb_hip_action)
{
    PBAction PBaction;

    PBaction.set_suggestedstartingtime(pb_hip_action.starttime());
    PBaction.set_suggestedfinishingtime(pb_hip_action.starttime());

    PB_HIP_ShuntingUnit pb_shuntingUnit = pb_hip_action.shuntingunit();

    for (const string &trainUnitId : pb_shuntingUnit.memberids())
    {
        PBaction.add_trainunitids(trainUnitId);
    }

    PBTaskAction *task_action = PBaction.mutable_task();

    PBTaskType *taskType = task_action->mutable_type();

    taskType->set_predefined(PBPredefinedTaskType::BeginMove);

    return PBaction;
}

PBAction RunResult::CreateEndMoveAction(PB_HIP_Action &pb_hip_action)
{
    PBAction PBaction;

    PBaction.set_suggestedstartingtime(pb_hip_action.endtime());
    PBaction.set_suggestedfinishingtime(pb_hip_action.endtime());

    PB_HIP_ShuntingUnit pb_shuntingUnit = pb_hip_action.shuntingunit();

    for (const string &trainUnitId : pb_shuntingUnit.memberids())
    {
        PBaction.add_trainunitids(trainUnitId);
    }

    PBTaskAction *task_action = PBaction.mutable_task();

    PBTaskType *taskType = task_action->mutable_type();

    taskType->set_predefined(PBPredefinedTaskType::EndMove);

    return PBaction;
}

// Compares two neighbooring action by their start time and duration
// if result is positive than pb_action1 should be placed after pb_action2 in the
// actions vector
int compare_helperfunction(PBAction pb_action1, PBAction pb_action2)
{
    int start1 = pb_action1.suggestedstartingtime();
    int start2 = pb_action2.suggestedstartingtime();

    int finish1 = pb_action1.suggestedfinishingtime();
    int finish2 = pb_action2.suggestedfinishingtime();

    // Compute durations
    int duration1 = finish1 - start1;
    int duration2 = finish2 - start2;

    // Primary sort: suggestedStartingTime
    if (start1 != start2)
        return start1 - start2;

    // Secondary sort: duration (if same starting time)
    return duration1 - duration2;
}

// The actions vector is sorted by the start time and the duration of the actions
// in a Increasing order, Arrive actions have to be before the BeginMove
void sort_actions_helperfunction(vector<PBAction> &actions)
{
    int n = actions.size();
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            int compare_res = compare_helperfunction(actions[j], actions[j + 1]);
            if (compare_res > 0)
            {
                swap(actions[j], actions[j + 1]);
            }
            else if (compare_res == 0)
            {
                if (actions[j + 1].task().type().predefined() == PBPredefinedTaskType::Arrive)
                    swap(actions[j], actions[j + 1]);
            }
        }
    }
}

RunResult *RunResult::CreateRunResult(const PB_HIP_Plan &pb_hip_plan, string scenarioFileString, const Location *location, const string &pathToStoreEval, int departureDelay)
{
    PBRun pb_run;

    PBPOSPlan pb_plan;

    vector<PBAction> pb_actions;

    vector<PBAction> pb_split_actions;

    vector<PBAction> pb_combine_actions;

    vector<PBAction> pb_combne_actions;

    vector<PB_HIP_Action> pb_action(pb_hip_plan.actions().begin(), pb_hip_plan.actions().end());

    int index = 0;
    // Iterate over all the HIP action
    for (PB_HIP_Action &hip_action : pb_action)
    {

        PBAction action_;

        action_.set_suggestedstartingtime(hip_action.starttime());
        action_.set_suggestedfinishingtime(hip_action.endtime());

        if (hip_action.endtime() - hip_action.starttime() != 0)
            action_.set_minimumduration(hip_action.endtime() - hip_action.starttime());

        PB_HIP_ShuntingUnit hip_shuntingUnit = hip_action.shuntingunit();

        for (const string &trainUnitId : hip_shuntingUnit.memberids())
        {
            action_.add_trainunitids(trainUnitId);
        }

        // Check if the shunting unit is In/OutStanding train
        // If field is defined it states InStanding when the train unit was alredy on the yard even if the action says it is an arrival
        // or it states OutStanding when the train unit will stay in the shunting yards after the scenario ends even if the action is an exite one
        if (hip_shuntingUnit.standingtype() != "")
        {
            action_.set_standingtype(hip_shuntingUnit.standingtype());
        }

        PB_HIP_TaskType taskType = hip_action.tasktype();

        // test if predefined or other field is defined
        if (taskType.has_predefined())
        {

            switch (taskType.predefined())
            {
            case PB_HIP_PredefinedTaskType::Move:
            {

                // Zero movement in HIP might be generated for HIP specific reasons
                // but a Zero movement is seen as an error by TORS
                if (hip_action.endtime() - hip_action.starttime() == 0)
                {
                    break;
                }

                PBMovementAction *move_action = action_.mutable_movement();

                move_action->add_path(hip_action.location());

                for (PB_HIP_Recource resource : hip_action.resources())
                {
                    move_action->add_path(resource.trackpartid());
                }

                PBAction BeginMoveAction = RunResult::CreateBeginMoveAction(hip_action);

                pb_actions.push_back(BeginMoveAction);

                pb_actions.push_back(action_);

                // If next element is an Exit, the EndAction is not added

                PB_HIP_Action next_action = pb_action[index + 1];

                if (next_action.tasktype().predefined() != PB_HIP_PredefinedTaskType::Exit)
                {
                    PBAction EndMoveAction = RunResult::CreateEndMoveAction(hip_action);

                    pb_actions.push_back(EndMoveAction);
                }
                break;
            }
            case PB_HIP_PredefinedTaskType::Split:
            {
                PBTaskAction *task_action = action_.mutable_task();
                PBTaskType *taskType = task_action->mutable_type();

                taskType->set_predefined(PBPredefinedTaskType::Split);

                auto &trainUnitIds = hip_shuntingUnit.memberids();

                task_action->add_trainunitids(trainUnitIds[0]);

                pb_actions.push_back(action_);

                break;
            }
            case PB_HIP_PredefinedTaskType::Combine:
            {
                PBTaskAction *task_action = action_.mutable_task();
                PBTaskType *taskType = task_action->mutable_type();

                taskType->set_predefined(PBPredefinedTaskType::Combine);

                for (const string &trainUnitId : hip_shuntingUnit.memberids())
                {
                    task_action->add_trainunitids(trainUnitId);
                }

                pb_combne_actions.push_back(action_);

                break;
            }

            case PB_HIP_PredefinedTaskType::Wait:
            {
                action_.mutable_break_();
                pb_actions.push_back(action_);

                break;
            }
            case PB_HIP_PredefinedTaskType::Arrive:
            {

                PBTaskAction *task_action = action_.mutable_task();

                PBTaskType *taskType = task_action->mutable_type();

                taskType->set_predefined(PBPredefinedTaskType::Arrive);

                pb_actions.push_back(action_);

                break;
            }
            case PB_HIP_PredefinedTaskType::Exit:
            {

                PBTaskAction *task_action = action_.mutable_task();

                PBTaskType *taskType = task_action->mutable_type();

                taskType->set_predefined(PBPredefinedTaskType::Exit);

                pb_actions.push_back(action_);

                break;
            }

            default:
                break;
            }
        }
        else if (taskType.has_other())
        {
            PBTaskAction *task_action = action_.mutable_task();

            PBTaskType *taskType_ = task_action->mutable_type();

            taskType_->set_other(taskType.other());

            task_action->set_location(hip_action.location());

            for (const string &trainUnitId : hip_shuntingUnit.memberids())
            {
                task_action->add_trainunitids(trainUnitId);
            }

            for (PB_HIP_Recource resource : hip_action.resources())
            {
                PBFacilityInstance *facilites = task_action->add_facilities();
                facilites->set_id(resource.facilityid());
            }

            pb_actions.push_back(action_);
        }

        string jsonResult;
        google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(action_, &jsonResult);
        if (!status.ok())
        {
            std::cerr << "Failed to convert protobuf to JSON: " << status.ToString() << std::endl;
        }

        index++;
    }

    // post-process combine actions
    std::map<int, std::vector<PBAction>> startTimeToCombineActions;

    for (auto &combine_action : pb_combne_actions)
    {
        startTimeToCombineActions[combine_action.suggestedstartingtime()].push_back(combine_action);
    }

    for (auto &[startTime, combine_actions] : startTimeToCombineActions)
    {

        PBAction PBaction = combine_actions[0];
        // Remove the first train unit id - the one which is the first unit
        // to be combined with the rest
        PBaction.mutable_trainunitids()->Clear();

        PBTaskAction *task_action = PBaction.mutable_task();

        for (size_t i = 1; i < combine_actions.size(); i++)
        {
            // Add all the train units which must be combined
            // a combine action might contains multiple units
            PBAction PBaction_other = combine_actions[i];
            auto &trainunits = PBaction_other.trainunitids();

            for (auto &unit : trainunits)
            {
                task_action->add_trainunitids(unit);
                PBaction.add_trainunitids(unit);
            }
        }
        pb_actions.push_back(PBaction);
    }

    // Sort actions by start time and duration
    sort_actions_helperfunction(pb_actions);

    // Add actions to plan
    for (auto &action : pb_actions)
    {
        string jsonResult;
        google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(action, &jsonResult);
        if (!status.ok())
        {
            std::cerr << "Failed to convert protobuf to JSON: " << status.ToString() << std::endl;
        }
        // cout << "Action : " << jsonResult << endl;

        PBAction *action_ = pb_plan.add_actions();
        action_->CopyFrom(action);
        // *pb_plan.add_actions() = action;
    }

    pb_run.mutable_plan()->CopyFrom(pb_plan);

    // Create Scenario protobuf

    PBScenario pb_scenario;
    parse_json_to_pb(fs::path(scenarioFileString), &pb_scenario);

    // Add scenario to the plan
    pb_run.mutable_scenario()->CopyFrom(pb_scenario);

    // Add default location path to the plan
    pb_run.set_location(".");

    Scenario scenario = Scenario(scenarioFileString, *location);

    // Add delay to the departure time, if not spceified the delay is 0 by default
    scenario.InitDepartureDelay(departureDelay);
    cout << ">>>>>>> TEST departure delay time: " << scenario.GetDepartureDelay() << endl;

    if (!pathToStoreEval.empty())
        scenario.SetEvaluatiorStoragePath(pathToStoreEval);

    POSPlan plan = POSPlan::CreatePOSPlan(location, &scenario, pb_plan);

    bool feasible = pb_run.feasible();

    pb_run.mutable_plan()->CopyFrom(pb_plan);
    string jsonPlan;
    google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(pb_run, &jsonPlan);
    if (!status.ok())
    {
        std::cerr << "Failed to convert protobuf to JSON: " << status.ToString() << std::endl;
    }

    ofstream file;
    file.open("/workspace/robust-rail-solver/ServiceSiteScheduling/database/TUSS-Instance-Generator/scenario_settings/setting_issue/plan_converted.json");
    file << jsonPlan;
    file.close();

    return new RunResult(location->GetLocationFilePath(), scenario, plan, feasible);
}

void GetRunResultProto(string planFileString, PBRun &pb_runResult)
{
    // PBRun pb_runResult2;

    parse_json_to_pb(fs::path(planFileString), &pb_runResult);

    if (!pb_runResult.IsInitialized() || pb_runResult.ByteSizeLong() == 0)
    {
        std::cerr << "RunResult Protobuf is empty or not initialized." << std::endl;
    }
    else
    {
        std::cout << "RunResult Protobuf has been initialized and may contain data." << std::endl;
    }
}

void ParseHIP_PlanFromJson(string planFileString, PB_HIP_Plan &pb_hip_plan)
{

    parse_json_to_pb(fs::path(planFileString), &pb_hip_plan);

    if (!pb_hip_plan.IsInitialized() || pb_hip_plan.ByteSizeLong() == 0)
    {
        std::cerr << "HIP plan Protobuf is empty or not initialized." << std::endl;
    }
    else
    {
        std::cout << "HIP plan Protobuf has been initialized and may contain data." << std::endl;
    }
}
