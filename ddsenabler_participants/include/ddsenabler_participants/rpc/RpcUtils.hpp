// Copyright 2025 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file RpcUtils.hpp
 */

#pragma once

#include <string>

#include <ddsenabler_participants/Constants.hpp>
#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/rpc/RpcTypes.hpp>
#include <ddsenabler_participants/rpc/RpcStructs.hpp>

#include <ddsenabler_participants/library/library_dll.h>

namespace eprosima {
namespace ddsenabler {
namespace participants {
namespace RpcUtils {

/**
 * @brief Generates a UUID.
 *
 * @return A new UUID.
 */
DDSENABLER_PARTICIPANTS_DllAPI
UUID generate_UUID();

// ROS 2 ACTION MSGS
/**
 * @brief Creates a JSON string for sending a goal request.
 *
 * @param goal_json The JSON string representing the goal (without the UUID part).
 * @return The JSON string for sending the goal request.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_goal_request_msg(
        const std::string& goal_json,
        UUID& goal_id);

/**
 * @brief Creates a JSON string for sending a goal reply.
 *
 * @param accepted Indicates whether the goal was accepted or not.
 * @return A JSON string representing the goal reply.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_goal_reply_msg(
        bool accepted);

/**
 * @brief Creates a cancel message for an action.
 *
 * @param goal_id The UUID of the goal to cancel.
 * @param timestamp The timestamp for canceling logic.
 * @return A JSON string representing the cancel message.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_cancel_request_msg(
        const UUID& goal_id,
        const int64_t timestamp);

/**
 * @brief Creates a cancel reply message for an action.
 *
 * @param cancelling_goals A vector of pairs containing the UUIDs of the goals to cancel and their timestamps.
 * @param cancel_code The cancel code indicating the result of the cancellation.
 * @return A JSON string representing the cancel reply message.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_cancel_reply_msg(
        std::vector<std::pair<UUID, std::chrono::system_clock::time_point>> cancelling_goals,
        const CancelCode& cancel_code);

/**
 * @brief Creates a result request message for an action.
 *
 * @param goal_id The UUID of the goal for which the result is being requested.
 * @return A JSON string representing the result request message.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_result_request_msg(
        const UUID& goal_id);

/**
 * @brief Creates a result reply message for an action.
 *
 * @param status_code The status code of the action result.
 * @param json The JSON string representing the result.
 * @return A JSON string representing the result reply message.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_result_reply_msg(
        const StatusCode& status_code,
        const char* json);

/**
 * @brief Creates a feedback message for an action.
 *
 * @param json The JSON string representing the feedback.
 * @param goal_id The UUID of the goal for which the feedback is being sent.
 * @return A JSON string representing the feedback message.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_feedback_msg(
        const char* json,
        const UUID& goal_id);

/**
 * @brief Creates a status message for an action.
 *
 * @param goal_id The UUID of the goal.
 * @param status_code The status code of the action.
 * @param goal_accepted_stamp The timestamp when the goal was accepted.
 * @return A JSON string representing the status message.
 */
DDSENABLER_PARTICIPANTS_DllAPI
std::string create_status_msg(
        const UUID& goal_id,
        const StatusCode& status_code,
        std::chrono::system_clock::time_point goal_accepted_stamp);

} // namespace RpcUtils
} // namespace participants
} // namespace ddsenabler
} // namespace eprosima

DDSENABLER_PARTICIPANTS_DllAPI
std::ostream& operator <<(
        std::ostream& os,
        const eprosima::ddsenabler::participants::UUID& uuid);
