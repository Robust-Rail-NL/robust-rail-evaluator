# Pseudo code of cTORS 

The purpose of this description is to highlight the main features of the cTORS POS plan evaluation process.

Note that cTORS evaluates POS plans, and (I suspect) is not directly compatible with the plan generated by HIP. 

* Small TODO: check whether the HIP Totally Ordered Plan can be used as input (I assume some kind of converter is needed - cTORS can use a predefined protobuf - in my opinion, we can add an extension capable of converting the HIP plan into an input prtobuf used by cTORS).


## Evaluation process

### Introduction
cTORS in principle is able to evaluate the palns in two ways: 
* During the interaction of a user or agent 
* Using a POS plan as input - this later is not explicitly there, but `EngineTest.cpp` contians `engine.EvaluatePlan` 


```bash
PBRun pb_run;
		engine.GetResult(state)->Serialize(engine, &pb_run);
		engine.EndSession(state);
		auto runResult = RunResult::CreateRunResult(&engine.GetLocation(), pb_run);
		CHECK(engine.EvaluatePlan(runResult->GetScenario(), runResult->GetPlan()));
		delete runResult;
```

> Here we might call `POSPlan CreatePOSPlan(location, scenario, pb_action)` - with `pb_action` as input protobuf POS plan and replace `runResult->GetPlan()` with our input POS plan.  

### Pseudo code - 1 

```bash
# Initialize

Initialize LocationEngine <- init with location.json, scenarion.json
    AddGenerators() # used to generate the actions for the given state *
    AddValidators() # used to check the validity of a given action **

StartSession() # Initiates values, fill event queue with events

# Loop 
while(True) # exits when number of actions is not 0 or `ScenarioFailedException` found (the plan is not feseable)
    actions <- getValidActions() # these are the valid actions according to the scenarion and location
    a <- user input # choose among valid actios
    # OR
    a <- agent action input 
    # OR
    a <- action from POS plan

    ApplyActionAndStep(state, a)

result = createRunResultPlan()

# Here we should also evaluate the call when using a plan issued by HIP 

EvaluatePlan(scenario, resultPlan)

# Last operation
EndSession(state) 

```

> **ApplyActionAndStep(state, action)** is one of the most important function
 * `action` can be of `Action` type and `SimpleAction` type

* if type(`action`) is `Action` 
```bash

    ApplyAction(state, action)
        posAction <- createPOSAction()
        AddResult(posAction) # add action to POS results to

    Step(state) -> ExecuteEvents(state) # Go to the next Step in the simulation and update the State
```

* if type(`action`) is `SimpleAction` 
```bash
	#Apply the SimpleAction to the State and go to the next step in the simulation

    action <- generateAction(SimpleAction) # generate action from SimpleAction, using special operators of generators see at `*`
    is_valid <- IsValid(state, action) #checks if the action is valid, using the validators and rules -- when an action is invalid

    ApplyAction(state, action)
        posAction <- createPOSAction()
        AddResult(posAction) # add action to POS results to

    Step(state) -> ExecuteEvents(state) # Go to the next Step in the simulation and update the State

```


> `*` **generators**: `move`, `move_helper`, `arrive`, `exit`, `wait`, `service`, `set_back`, `split`, `combine`



> `**` **rules** followed by the validators: `in_correct_time_rule`, `out_correct_time_rule`,  `out_correct_order_rule`, `single_move_track_rule`, `length_track_rule`, `blocked_track_rule`, `blocked_first_track_rule`, `electric_track_rule`, `legal_on_parking_track_rule`, `legal_on_setback_track_rule`, `electric_move_rule`, `setback_once_rule`, `setback_track_rule`, `correct_facility_rule`, `mandatory_service_task_rule`, `optional_service_task_rule`, `understaffed_rule`, `available_facility_rule`, `capacity_facility_rule`,  `disabled_facility_rule`, `order_preserve_rule`, `park_combine_split_rule`, `setback_combine_split_rule`