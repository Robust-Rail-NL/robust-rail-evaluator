# Description TORS

This document provides some details about TORS and the input required to run the evaluation and other functions.

### Location

The shunting yard is modeled as consisting of several tracks. Every track is connected to a number of tracks on the A-side and on the B-side. Each track has a type: `Bumper`, `Railroad`, or `Switch`. A bumper is the end of a track and is only connected on the A-side or B-side to one other track. A railroad is a track that is connected to one track at both the A- and B-side. Railroads have a length, and trains may be parked on railroads (if allowed). There are several types of switches. The switch is a track splitting into two tracks. An `English Switch` connects both A-side tracks to both B-side tracks, or both B-side tracks to A-side tracks. A `Half English Switch` connects A-side-1 to both B-sides, but A-side-2 is connected only to B-side-2. An `Intersection` connects four tracks, but A-side-1 is only connected with B-side-2, and A-side-2 is only connected to B-side-1. 

The tracks also have a number of attributes:
* **length**: this number describes the length of the track in meters (only Railroad tracks) and determines the amount of trains that can be parked on a track at one moment in time. 

* **saw_movement_allowed**: only if this attribute is true, can `setback` (change the shunting unit's direction) operations be performed on this track. 

* **parking_allowed**: only if this attribute is true, is parking allowed on this track

 Trains are directed. Therefore, a train's position cannot only be described by its current track, but also by its previous track. For this same reason, when you run, for example, the location.`get_shortest_path` function, you need to provide four tracks as parameters: (`from_previous`, `from_track`, `to_previous`, `to_track`). 

When a train arrives, the incoming object has a `parking_track` attribute and a side_track attribute. The latter describes the "`bumper`" from which the shunting unit will enter the yard. Once it has arrived, this side track becomes the previous track of the shunting unit. For the outgoing shunting unit, the side_track describes the "`bumper`" over which the shunting unit will depart from the yard.

### Shunting units 

In TORS, the most important entities are shunting units. A shunting unit is a combination of one or more trains that is shunted as one unit. In this terminology (unlike what you might expect) a train is considered to be an atomic unit (even though a train exists of several carriages), i.e., in TORS a train cannot be split. The number of trains and shunting units is therefore not necessarily the same. Note also that this means that trains are atomic and persistent, whereas shunting units are transient! 

Every train has a unique id. The shunting units also have id's, which are unique on a given moment in time, but over time the same id can be used for different shunting units. For example, a scenario may have an incoming shunting unit with id 33333, consisting of two trains with ids 2401 and 2402, and an outgoing shunting unit also with id 33333, but with one train with id 2401. 

In the first phase of the competition, only shunting units consisting of one train are considered, so this can still be ignored. But for later phases the distinction between a shunting unit and a train will become important. 

Also, in the second phase, the matching problem will be added to the challenge. This means that outgoing goals will not be specified by train ids, but by train types, e.g. a shunting unit with id 11111 should leave from track 1 at time 800 consisting of two trains of type SLT4 (instead of two trains with id 2401 and 2402).

Every train has a type. In phase 1, the train type influences the problem in two ways:

* The train type determines the length of the setback action. Therefore, the location.get_shortest_path function also requires the train type as a parameter. 

* The train type determines the length of a shunting unit. This is important, because fewer longer trains can fit on one track than short trains.

In summary: 

* Trains are atomic and persistent. Shunting units are transient and can be split and joined together. 

* Train ids are unique. Shunting unit ids are only unique on that moment in time, but may be reused later in time. 

* Incoming and outgoing goals are defined on shunting units. 

* Maintenance tasks are defined for trains.

### TORS simulation loop 

TORS is a state-event-action based simulator and operates as follows: 

1) An initial state is generated from the scenario description. 

2) **While** the scenario is not finished (i.e. there are remaining events or the scenario end time is not reached yet): 

    1) Execute all events that are triggered on this timestamp. 

    2) **If** no action is required (i.e. all shunting units have an active action), go back to 2.1 
    
    3) **End if**

    4) Read the next action from the plan.

        1) Execute the action. (if invalid, stop the scenario)
        
3) **End While** 


### Actions

Every time an action is required, one is assigned from the plan. The TORS simulator has two different classes for actions: `Action` and `SimpleAction`. In general, `Action` is used for the internal representation of actions. `SimpleAction` is used to communicate with external parties, such as the plan.

Note that even though an action is valid it does not always end up in a valid state (an invalid state is a state in which an action is required, but no action is available). For example a shunting is moved to another track, but while it moves it blocks all tracks that another shunting unit needs (and this other unit is not allowed to park). 

The following actions are available: 

* `Arrive(inc)`: Let the shunting unit described by the Incoming instance inc arrive. 

* `Exit(su, out)`: Exit the shunting unit `su`, according to the outgoing goal described by out. Note that you need to specify which shunting unit you want to exit. This is because of the matching problem, where out does not describe a uniquely identifiable shunting unit. 

* `BeginMove(su)`: Begin a move operation for shunting unit `su`. 

* `EndMove(su)`: End a move operation for the shunting unit `su`. This means park the shunting unit. 

* `Move(su, to_track)`: Move shunting unit `su` to track to_track. 

* `Setback(su)`: Setback the shunting unit (that is, change the unit's direction). 

* `Service(su, task, train, facility)`: Execute the specified service task on the train in the shunting unit su at the given facility. 

* `Split(su, split_index)`: Split the shunting unit `su` at the specified index. TU3 - TU2 - TU1> with split index 2 is split into TU3> and TU2 - TU1>. 

* `Combine(su1, su2)`: Combine the two adjacent shunting units `su1` and `su2` into one unit. 

* `Wait(su)`: Let the shunting unit `su` wait until the next event.

### Move actions and duration 

When the simulator is asked to generate actions, it will generate move actions by checking for full origin-destination paths (without setbacks). When such paths are available, they are changed into step by step move actions. Such moves are always from one railroad track to a (indirect) neighboring rail track. Here one could consider the railroad tracks to be the nodes of a graph, and all the switches to be the edges. The is done to make the action space smaller. 

The duration of move actions is determined as follows: the duration of a move action is `movement_constant` + `railroad_coefficient` * `number_of_railroad_tracks` + `switch_coefficient` * `number_of_switches`.


### Business rules
To check an action's validity, a number of business rules are checked. This table shows which business rules are checked.

| Name           | Category         | Description  |
|:-------------:|:--------------:|:---------------:|
| end_correct_order_on_track_rule            | arrival_departure              | Rule that verifies that shunting units which stay in the shunting yard after the scheduling period will be located in the right order on their track.             |
| in_correct_time_rule |	arrival_departure |	Rule that verifies that shunting units that are arriving, arrive at the correct time. Note: shunting units will never arrive too early, so this rule only checks if a shunting unit arrives too late.  |
| out_correct_order_rule |	arrival_departure | Rule that verifies that leaving shunting units have their train units in the correct order when they leave the shunting yard.  |
|out_correct_time_rule |	arrival_departure |	Rule that verifies that leaving shunting units leave at the correct time. |
|out_correct_track_rule	| arrival_departure |	Rule that verifies that leaving shunting units leave over the correct tracks. |
|blocked_first_track_rule | track_occupation | Rule that verifies that shunting units, upon starting a movement, are not blocked on exit by other shunting units on their current track.| 
|blocked_track_rule | track_occupation | Rule that verifies that moving shunting units are not blocked by other shunting units. Rule that verifies that shunting units on a single |
| length_track_rule | track_occupation | Rule that verifies that shunting units on a single track do not take up more space than available on that track.|
|single_move_track_rule | track_occupation | Rule that verifies that at most one shunting unit can use a piece of track at a given time. |
| electric_track_rule | parking | Rule that verifies that shunting units which need electricity park only on electrified tracks. |
| legal_on_parking_track_rule | parking | Rule that verifies that parked shunting units are on a track where parking is allowed. |
| legal_on_setback_track_rule | parking | Rule that verifies if a shunting unit is parked on a track where setback is allowed. |
| electric_move_rule | shunting | Rule that verifies that shunting units which need electricity park only on electrified tracks. |
|setback_once_rule | shunting |Rule that verifies that a setback action is not performed on a shunting unit which is already in a neutral state. A shunting unit is in a neutral state if a setback or service action is performed. |
|setback_track_rule| shunting | Rule that verifies that performing a setback action on a shunting unit is allowed on the track where the shunting unit is at. |
|available_facility_rule| facility | Rule that verifies that tasks assigned to a facility are only executed when that facility is available. |
| capacity_facility_rule | facility | Rule that verifies that no more tasks are executed at a facility than the facility can handle. |
| disabled_facility_rule | facility | Rule that verifies that no tasks are assigned to facilities which are disabled by a disturbance. |
| correct_facility_rule | service_tasks | Rule that verifies that service tasks are executed at the correct facility. |
|mandatory_service_task_rule | service_tasks | Rule that verifies that all required service tasks are performed before a shunting unit leaves the shunting yard.|
|optional_service_task_rule | service_tasks | Rule that verifies that all optional service tasks are performed before a shunting unit leaves the shunting yard. |
| understaffed_rule | service_tasks | Rule that verifies that all tasks have enough employees assigned, with the right skills, such that the task will have all of its required skills available. |
| order_preserve_rule | combine_and_split | Rule that verifies that combining or splitting shunting units does not change the order of train
units on a track. |
| park_combine_split_rule | combine_and_split | Rule that verifies that combine and split actions on shunting units are only performed on tracks where parking is allowed. |
| setback_combine_split_rule | combine_and_split | Rule that verifies that combine and split actions on shunting units are only performed on tracks where setback is allowed.| 


### config.json 

In the data folder there should be a config.json, a location.json.

The `config.json` contains the following settings: 

* `business_rules`: configuration for the business rules 

* `business_rules`/`categories`: shortcuts to disable whole categories of rules (currently ignored) 

* `business_rules`/`rules`: per rule a dictionary with parameters: 

    * `on`: boolean to turn the rule on or off 

    * `soft`: (currently ignored) boolean to turn the constraint into a soft constraint 

    * `category`: (currently ignored) 

    * `parameters`: (currently ignored) extra parameters for the business rule 

    * `priority`: (currently ignored) 

    * `status`: (currently ignored) 

* `actions`: settings for every action type: 

    * `parameters`: a set of extra parameters (e.g. for setback how to calculate the move duration) 

    * `on`: boolean to turn this action on or off (for phase 1 split and combine are off) 

    * `status`: (currently ignored) 

* `employees`: (currently ignored) whether to include employees in the problem 

* `verbose`: (currently ignored) 

* `br_break_on_fail`: (currently ignored) 

* `routing`: (currently ignored) 

* `distance_matrix`: (currently ignored) 

### location.json
The same folder also contains the location.json config file which describes the location. All the tracks and facilities are described in a clear way. It also contains the following settings:

* `MovementConstant`
* `MovementTrackCoefficient`
* `MovementSwitchCoefficient`

These three are used to calculate the duration of move actions.