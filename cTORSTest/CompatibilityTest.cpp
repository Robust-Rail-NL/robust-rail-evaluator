#include "doctest/doctest.h"
#include "Engine.h"


#include <cstdlib>
#include <google/protobuf/util/json_util.h>

namespace cTORSTest
{
	// To specify the scenario / location / plan - PATH
	// In the terminal the followinf should be exported by respecting the argument names
	// 
	// export LOCATION_PATH="/path/to/location_folder" - where the location.json file can be found e.g. "~/data/Demo/TUSS-Instance-Generator/kleine_brinkhorst_v2"
	// export SCENARIO_PATH="/path/to/scenario_folder/scenario.json"
	// export PLAN_PATH="/path/to/plan_folder/plan.json"
	// export RESULT_PATH="/path/to/plan_folder/result.txt"

	string location_path;
	string scenario_path;
	string plan_path;
	string result_path;

	TEST_CASE("Scenario and Location Compatibility test")
	{


		
		cout << "-------------------------------------------------------------------------------------------------" << endl;
		cout << "											SCENARIO AND LOCATION TEST " << endl;
		cout << "-------------------------------------------------------------------------------------------------" << endl;
				

		// Get location path and ensure it is specified 
		const char* LOCATION_PATH = getenv("LOCATION_PATH");
		REQUIRE(LOCATION_PATH != nullptr);

		location_path = string(LOCATION_PATH);
		cout << "Location path: " << location_path << endl;

		// Get scenario path and ensure it is specified 
		const char* SCENARIO_PATH = getenv("SCENARIO_PATH");
		REQUIRE(SCENARIO_PATH != nullptr);

		scenario_path = string(SCENARIO_PATH);
		cout << "Scenario path: " << scenario_path << endl;


		// Get scenario path and ensure it is specified 
		const char* PLAN_PATH = getenv("PLAN_PATH");
		REQUIRE(PLAN_PATH != nullptr);

		plan_path = string(PLAN_PATH);
		cout << "Plan path: " << plan_path << endl;
		

		// Get evaluation result path and ensure it is specified
		const char* RESULT_PATH = getenv("RESULT_PATH");
		REQUIRE(RESULT_PATH != nullptr);

		result_path = string(RESULT_PATH);
		cout << "Evaluation result path: " << result_path << endl;
		

		LocationEngine engine(location_path);


		cout << "------------------------------------------------------" << endl;
		cout << "               Location file loading is done" << endl;
		cout << "------------------------------------------------------" << endl;

		bool no_error = true;

		// auto &scenarioToTest = engine.GetScenario(scenario_path);
		const Scenario &scenarioToTest = engine.GetScenario(scenario_path);
		auto &locationToTest = engine.GetLocation();

		try
		{
			scenarioToTest.CheckScenarioCorrectness(locationToTest);
		}
		catch (const std::invalid_argument &e)
		{
			no_error = false;
			std::cerr << "Issue detected with the Scenario: " << e.what() << std::endl;

		}
		CHECK(no_error);
	}

	TEST_CASE("Plan Compatibility test")
	{
		cout << "-------------------------------------------------------------------------------------------------" << endl;
		cout << "											PLAN TEST " << endl;
		cout << "-------------------------------------------------------------------------------------------------" << endl;
		/* HIP plan protobuf is different thatn the one used by cTORS. HIP plans converted into json format that has to be somehow parsed to cTORS protobuf
		this code is intended to do this conversion*/

		LocationEngine engine(location_path);
		auto &scenario = engine.GetScenario(scenario_path);
		const Location &location = engine.GetLocation();

		PB_HIP_Plan pb_hip_plan;


		ParseHIP_PlanFromJson(plan_path, pb_hip_plan);

		auto runResult_external = RunResult::CreateRunResult(pb_hip_plan, scenario_path, &location);
		CHECK(engine.EvaluatePlan(runResult_external->GetScenario(), runResult_external->GetPlan(), result_path));

	}
}