/** \file Proto.h
 * Imports the generated protobuf headers and defines the protobuf types
 */
#ifndef PROTO_H
#define PROTO_H
//!\cond SYS_HEADER
#include <string>
#include <vector>
#include <list>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include "Location.pb.h"
#include "Scenario.pb.h"
#include "TrainUnitTypes.pb.h"
#include "PartialOrderSchedule.pb.h"
#include "Plan.pb.h"
#include "Run.pb.h"

#include "HIP_Location.pb.h"
#include "HIP_Plan.pb.h"
#include "HIP_Scenario.pb.h"
//!\endcond
using namespace std;

typedef proto_tors::Scenario PBScenario;                        /**< The protobuf Scenario class */
typedef proto_tors::Train PBTrainGoal;                          /**< The protobuf TrainGoal class */
typedef proto_tors::ShuntingUnit PBShuntingUnit;                /**< The protobuf ShuntingUnit class */

typedef proto_tors::TrainUnit PBTrainUnit;                      /**< The protobuf Train class */
typedef proto_tors::TrainUnitType PBTrainUnitType;              /**< The protobuf TrainUnitType class */
typedef proto_tors::TaskType PBTaskType;                        /**< The protobuf Task type description class */
typedef proto_tors::PredefinedTaskType PBPredefinedTaskType;    /**< The protobuf predefined Task type class */
typedef proto_tors::TaskSpec PBTask;                            /**< The protobuf Task class */

typedef proto_tors::MemberOfStaff PBMemberOfStaff;              /**< The protobuf Employee class */
typedef proto_tors::TimeInterval PBTimeInterval;                /**< The protobuf TimeShift class */

typedef proto_tors::Location PBLocation;                        /**< The protobuf Location class */
typedef proto_tors::Facility PBFacility;                        /**< The protobuf Facility class */
typedef proto_tors::TrackPart PBTrack;                          /**< The protobuf Track class */
typedef proto_tors::TrackPartType PBTrackPartType;              /**< The protobuf TrackPartType class */
typedef proto_tors::Side PBSide;                                /**< The protobuf class to describe A or B sides */

typedef proto_tors::PartialOrderSchedule PBPOSPlan;             /**< The protobuf POSPlan class */
typedef proto_tors::Match PBMatch;                              /**< The protobuf POSMatch class */
typedef proto_tors::Action PBAction;                            /**< The protobuf POSAction class */
typedef proto_tors::MovementAction PBMovementAction;            /**< The protobuf movement action class */
typedef proto_tors::TaskAction PBTaskAction;                    /**< The protobuf service action class */
typedef proto_tors::FacilityInstance PBFacilityInstance;
typedef proto_tors::BreakAction PBBreakAction;                  /**< The protobuf wait action class */
typedef proto_tors::POSPrecedenceConstraint PBPOSPrecedenceConstraint; /**< The protobuf POSPrecedenceConstraint class */
typedef proto_tors::Run PBRun;                                  /**< The protobuf RunResult class */


// HIP based protobuf types
typedef HIP_protos::Scenario PB_HIP_Scenario;
typedef HIP_protos::IncomingTrain PB_HIP_TrainGoal;
typedef HIP_protos::TrainRequest PB_HIP_TrainRequest;
typedef HIP_protos::IncomingTrainUnit PB_HIP_IncomingTrainUnit;
typedef HIP_protos::ShuntingUnit PB_HIP_ShuntingUnit;

typedef HIP_protos::TrainUnit PB_HIP_TrainUnit;
typedef HIP_protos::TrainUnitType PB_HIP_TrainUnitType;
typedef HIP_protos::TaskType PB_HIP_TaskType;
typedef HIP_protos::PredefinedTaskType PB_HIP_PredefinedTaskType;
typedef HIP_protos::TaskSpec PB_HIP_Task;
typedef HIP_protos::NonServiceTraffic PB_HIP_NonServiceTraffic;
typedef HIP_protos::DisabledTrackPart PB_HIP_DisabledTrackPart;
typedef HIP_protos::MemberOfStaff PB_HIP_MemberOfStaff;

typedef HIP_protos::Location PB_HIP_Location;
typedef HIP_protos::Facility PB_HIP_Facility;
typedef HIP_protos::TrackPart PB_HIP_Track;

typedef HIP_protos::Plan PB_HIP_Plan;
typedef HIP_protos::Action PB_HIP_Action;
typedef HIP_protos::Resource PB_HIP_Recource;



//!\cond NO_DOC
typedef google::protobuf::uint64 UInt; 
template<class Item> using PBList =  google::protobuf::RepeatedPtrField<Item>;

inline vector<string> PBToStringVector(const PBList<string>& items) {
    return vector<string>(items.begin(), items.end());
}

inline list<string> PBToStringList(const PBList<string>& items) {
    return list<string>(items.begin(), items.end());
}
//!\endcond

#endif