#include <iostream>
#include "Engine.h"

// Usage:
// ./main --mode EVAL/INTER --path_location ~/my_location_folder --path_scenario ~/my_scenarion.json --path_plan ~/my_plan.json --plan_type TORS/HIP
// Args:
//		--mode EVAL - Evaluates a plan according to a scenario and location; INTER: Interactive, the user has to chose a valid action per for each situation (state)
//      --path_location: specifies the path to the location file which most be called as `location.json`
//      --path_scenario: specifies the path to the scenario file e.g., my_scenario.json
//      --path_plan: specifies the path to the plan file e.g., my_plan.json
//      --plan_type: specifies the type of the plan, that when follows TORS format use --plan_type, when plan is issued by HIP use --plan_type HIP

int parse(int argc, char *argv[], std::string &mode, std::string &path_location, std::string &path_scenario, std::string &path_plan, std::string &plan_type);

int main(int argc, char *argv[])
{
	std::string mode;
	std::string path_location;
	std::string path_scenario;
	std::string path_plan;
	std::string plan_type;

	if (parse(argc, argv, mode, path_location, path_scenario, path_plan, plan_type) != 0)
	{
		return 1;
	}

	cout << "-------------------------------------------------------------------------------------------------" << endl;
	cout << "							   LOCATION TEST 										 " << endl;
	cout << "-------------------------------------------------------------------------------------------------" << endl;

	LocationEngine engine(path_location);
	const Location &location = engine.GetLocation();
	const vector<Track *> &tracks = location.GetTracks();

	cout << "\n Location \n";

	for (int i = 0; i < tracks.size(); i++)
	{
		cout << "id : " << tracks[i]->id << "\n";
		if (tracks[i]->type == TrackPartType::Railroad)
		{
			cout << "Type : RailRoad" << "\n";
		}
		cout << "name : " << tracks[i]->name << "\n";
	}

	cout << "-------------------------------------------------------------------------------------------------" << endl;
	cout << "								SCENARIO TEST 										 " << endl;
	cout << "-------------------------------------------------------------------------------------------------" << endl;

	auto &scenario = engine.GetScenario(path_scenario);
	scenario.PrintScenarioInfo();

	if (mode == "EVAL")
	{
		cout << "-------------------------------------------------------------------------------------------------" << endl;
		cout << "							PLAN EVALUATION TEST 		  			   				  " << endl;
		cout << "-------------------------------------------------------------------------------------------------" << endl;

		if (plan_type == "HIP")
		{
			PB_HIP_Plan pb_hip_plan;

			// Parses HIP issued (plan also following HIP format)
			ParseHIP_PlanFromJson(path_plan, pb_hip_plan);
			auto runResult_external = RunResult::CreateRunResult(pb_hip_plan, path_scenario, &location);

			if (engine.EvaluatePlan(runResult_external->GetScenario(), runResult_external->GetPlan()))
			{
				cout << "-------------------------------------------------------------------------------------------------" << endl;
				cout << "					PLAN EVALUATION TEST 		  			   				  " << endl;
				cout << "-------------------------------------------------------------------------------------------------" << endl;

				cout << "The plan is valid" << endl;
			}
			else
			{
				cout << "-------------------------------------------------------------------------------------------------" << endl;
				cout << "					PLAN EVALUATION TEST 		  			   				  " << endl;
				cout << "-------------------------------------------------------------------------------------------------" << endl;

				cout << "The plan is not valid" << endl;
			}

			return 0;
		}
		else if (plan_type == "TORS")
		{
			PBRun pb_run_external;

			// Parses the TORS fromated plan
			GetRunResultProto(path_plan, pb_run_external);

			auto runResult_external = RunResult::CreateRunResult(&location, pb_run_external);

			if (engine.EvaluatePlan(runResult_external->GetScenario(), runResult_external->GetPlan()))
			{

				cout << "-------------------------------------------------------------------------------------------------" << endl;
				cout << "					PLAN EVALUATION TEST 		  			   				  " << endl;
				cout << "-------------------------------------------------------------------------------------------------" << endl;

				cout << "The plan is valid" << endl;
			}
			else
			{

				cout << "-------------------------------------------------------------------------------------------------" << endl;
				cout << "					PLAN EVALUATION TEST 		  			   				  " << endl;
				cout << "-------------------------------------------------------------------------------------------------" << endl;

				cout << "The plan is not valid" << endl;
			}

			return 0;
		}
	}
	else if (mode == "INTER")
	{

		State *state = engine.StartSession(scenario);
		cout << "\nBeginning of session\n";
		engine.Step(state);
		while (true)
		{
			try
			{
				state->PrintStateInfo();
				auto &actions = engine.GetValidActions(state);
				cout << "[T = " + to_string(state->GetTime()) + "]\tChoosing from " << actions.size() << " actions.\n";
				if (actions.size() == 0)
					break;
				const Action *a;
				if (actions.size() == 1)
				{
					a = actions.front();
					cout << "----------------------------------------------------------\n";
					cout << "Action : " << a->toString() << "\n";
					cout << "----------------------------------------------------------\n";
				}
				else
				{
					int i = 0;
					for (auto a : actions)
					{
						cout << to_string(i) + ":\t" + a->toString() + ".\n";
						i++;
					}
					cout << "Which action?\n";
					while (!(cin >> i) || i >= actions.size())
					{
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						cout << "Invalid input.  Try again: Which action?\n";
					}
					a = *next(actions.begin(), i);
				}
				engine.ApplyActionAndStep(state, a);
			}
			catch (ScenarioFailedException e)
			{
				cout << "Scenario failed.\n";
				break;
			}
		}
		PBRun pb_run;
		engine.GetResult(state)->Serialize(engine, &pb_run);
		engine.EndSession(state);
		cout << "End of session\n";
		string out;
		cin >> out;
	}
	else
	{
		cout << "Unknown --mode. It should be EVAL or INTER" << endl;
	}
}

// Parse input arguments for configuration
int parse(int argc, char *argv[], std::string &mode, std::string &path_location, std::string &path_scenario, std::string &path_plan, std::string &plan_type)
{
	std::map<std::string, std::string> args;
	for (int i = 1; i < argc; i += 2)
	{ // Collect only the second itmes of argv[]
		if (i + 1 < argc)
		{
			args[argv[i]] = argv[i + 1];
		}
		else
		{
			std::cerr << "Error: Missing value for " << argv[i] << std::endl;
			return 1;
		}
	}

	if (args.find("--mode") != args.end())
	{
		mode = args["--mode"];
	}
	else
	{
		std::cout << "Missing path_location " << std::endl;
		return 1;
	}
	if (args.find("--path_location") != args.end())
	{
		path_location = args["--path_location"];
	}
	else
	{
		std::cout << "Missing path_location " << std::endl;
		return 1;
	}
	if (args.find("--path_scenario") != args.end())
	{
		path_scenario = args["--path_scenario"];
	}
	else
	{
		std::cout << "Missing path_scenario " << std::endl;
		return 1;
	}
	if (args.find("--path_plan") != args.end())
	{
		path_plan = args["--path_plan"];
	}
	else
	{
		std::cout << "Missing path_plan " << std::endl;
		return 1;
	}
	if (args.find("--plan_type") != args.end())
	{
		plan_type = args["--plan_type"];
	}
	else
	{
		std::cout << "Missing plan_type " << std::endl;
		return 1;
	}

	std::cout << "Configuration:" << std::endl;

	for (const auto &pair : args)
	{
		std::cout << pair.first << ": " << pair.second << std::endl;
	}

	return 0;
}

// auto& actions = engine.GetValidActions(state);
// provides a list of valid actions, the actions are generated by the ActionGenerators
// in main(): LocationEngine() -> ActionManager() -> AddGenerators(): these are called
// when generating the actions for the given state, same the ActionManager() initiates
// validators AddValidators() which are used to check the validity of a given action
// in a given state "IsValid(state, a);"