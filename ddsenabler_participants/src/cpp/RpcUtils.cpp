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
 * @file RpcUtils.cpp
 */

#include <ddsenabler_participants/RpcUtils.hpp>

#include <nlohmann/json.hpp>
#include <fstream>
#include <random>


namespace eprosima {
namespace ddsenabler {
namespace participants {
namespace RpcUtils {

/**
 * @brief Extracts the service name from a given topic name.
 *
 * @param [in] topic_name Topic name to extract the service name from
 * @return Extracted service name
 */
RpcType get_rpc_name(const std::string& topic_name, std::string& rpc_name)
{
    std::string original_topic_name = topic_name;
    rpc_name = topic_name;

    bool is_request = false;

    // Detect and remove prefix
    if (rpc_name.rfind(REQUEST_PREFIX, 0) == 0)
    {
        is_request = true;
        rpc_name = rpc_name.substr(3);
    }
    else if (rpc_name.rfind(REPLY_PREFIX, 0) == 0)
    {
        rpc_name = rpc_name.substr(3);
    }
    else
    {
        // Check for action feedback/status topics
        if (rpc_name.size() >= 9 && rpc_name.substr(rpc_name.size() - 9) == (std::string("/") + ACTION_FEEDBACK_SUFFIX))
        {
            rpc_name = rpc_name.substr(0, rpc_name.size() - 8);
            rpc_name = rpc_name.substr(3);
            return ACTION_FEEDBACK;
        }
        else if (rpc_name.size() >= 7 && rpc_name.substr(rpc_name.size() - 7) == (std::string("/") + ACTION_STATUS_SUFFIX))
        {
            rpc_name = rpc_name.substr(0, rpc_name.size() - 6);
            rpc_name = rpc_name.substr(3);
            return ACTION_STATUS;
        }

        return RPC_NONE;
    }

    // Check for action-related services
    if (is_request)
    {
        if (rpc_name.size() >= 7 && rpc_name.substr(rpc_name.size() - 7) == REQUEST_SUFFIX)
        {
            std::string base = rpc_name.substr(0, rpc_name.size() - 7);

            if (base.size() >= 9 && base.substr(base.size() - 9) == ACTION_GOAL_SUFFIX)
            {
                rpc_name = base.substr(0, base.size() - 9);
                return ACTION_GOAL_REQUEST;
            }
            else if (base.size() >= 10 && base.substr(base.size() - 10) == ACTION_RESULT_SUFFIX)
            {
                rpc_name = base.substr(0, base.size() - 10);
                return ACTION_RESULT_REQUEST;
            }
            else if (base.size() >= 11 && base.substr(base.size() - 11) == ACTION_CANCEL_SUFFIX)
            {
                rpc_name = base.substr(0, base.size() - 11);
                return ACTION_CANCEL_REQUEST;
            }

            rpc_name = base;
            return RPC_REQUEST;
        }
    }
    else if (rpc_name.size() >= 5 && rpc_name.substr(rpc_name.size() - 5) == REPLY_SUFFIX)
    {
        std::string base = rpc_name.substr(0, rpc_name.size() - 5);

        if (base.size() >= 9 && base.substr(base.size() - 9) == ACTION_GOAL_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - 9);
            return ACTION_GOAL_REPLY;
        }
        else if (base.size() >= 10 && base.substr(base.size() - 10) == ACTION_RESULT_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - 10);
            return ACTION_RESULT_REPLY;
        }
        else if (base.size() >= 11 && base.substr(base.size() - 11) == ACTION_CANCEL_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - 11);
            return ACTION_CANCEL_REPLY;
        }

        rpc_name = base;
        return RPC_REPLY;
    }

    rpc_name = original_topic_name; // Restore original topic name if no suffix matched
    EPROSIMA_LOG_ERROR(DDSENABLER_RPC_UTILS,
            "Invalid topic name for service: " << original_topic_name << ". Expected suffix 'Request' or 'Reply'.");
    return RPC_NONE;
}

RpcType get_service_name(const std::string& topic_name, std::string& service_name)
{
    service_name = topic_name;

    // Detect and remove prefix
    if (service_name.rfind(REQUEST_PREFIX, 0) == 0)
    {
        service_name = service_name.substr(3);
        if (service_name.size() >= 7 && service_name.substr(service_name.size() - 7) == REQUEST_SUFFIX)
        {
            service_name = service_name.substr(0, service_name.size() - 7);
            return RPC_REQUEST;
        }
    }
    else if (service_name.rfind(REPLY_PREFIX, 0) == 0)
    {
        service_name = service_name.substr(3);
        if (service_name.size() >= 5 && service_name.substr(service_name.size() - 5) == REPLY_SUFFIX)
        {
            service_name = service_name.substr(0, service_name.size() - 5);
            return RPC_REPLY;
        }
    }

    return RPC_NONE;
}

RpcType get_service_direction(RpcType rpc_type)
{
    switch (rpc_type)
    {
        case RPC_REQUEST:
        case RPC_REPLY:
            return rpc_type;
        case ACTION_GOAL_REQUEST:
        case ACTION_CANCEL_REQUEST:
        case ACTION_RESULT_REQUEST:
            return RPC_REQUEST;
        case ACTION_GOAL_REPLY:
        case ACTION_CANCEL_REPLY:
        case ACTION_RESULT_REPLY:
            return RPC_REPLY;
        default:
            break;
    }
    return RPC_NONE;
}

ActionType get_action_type(RpcType rpc_type)
{
    switch (rpc_type)
    {
        case ACTION_GOAL_REQUEST:
        case ACTION_GOAL_REPLY:
            return GOAL;
        case ACTION_RESULT_REQUEST:
        case ACTION_RESULT_REPLY:
            return RESULT;
        case ACTION_CANCEL_REQUEST:
        case ACTION_CANCEL_REPLY:
            return CANCEL;
        case ACTION_FEEDBACK:
            return FEEDBACK;
        case ACTION_STATUS:
            return STATUS;
        default:
            break;
    }
    return NONE;
}


UUID generate_UUID()
{
    UUID uuid;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);

    for (size_t i = 0; i < sizeof(uuid); ++i)
    {
        uuid[i] = static_cast<uint8_t>(dis(gen));
    }
    return uuid;
}

std::string create_goal_request_msg(const std::string& goal_json, UUID& goal_id)
{
    goal_id = generate_UUID();

    nlohmann::json j;
    j["goal_id"]["uuid"] = goal_id;
    j["goal"] = nlohmann::json::parse(goal_json);
    return j.dump(4);
}

std::string create_goal_reply_msg(
        bool accepted)
{
    auto now = std::chrono::system_clock::now();
    auto duration_since_epoch = now.time_since_epoch();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
    auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epoch).count() % 1'000'000'000;

    nlohmann::json j;
    j["accepted"] = accepted;
    j["stamp"]["sec"] = static_cast<int64_t>(sec);
    j["stamp"]["nanosec"] = static_cast<uint32_t>(nanosec);
    return j.dump(4);
}

std::string create_cancel_request_msg(
        const UUID& goal_id,
        const int64_t timestamp)
{
    nlohmann::json j;
    int64_t sec = timestamp / 1'000'000'000;
    uint32_t nanosec = timestamp % 1'000'000'000;
    j["goal_info"]["stamp"]["sec"] = static_cast<int64_t>(sec);
    j["goal_info"]["stamp"]["nanosec"] = static_cast<uint32_t>(nanosec);
    j["goal_info"]["goal_id"]["uuid"] = goal_id;
    return j.dump(4);
}

std::string create_cancel_reply_msg(
        std::vector<std::pair<UUID, std::chrono::system_clock::time_point>> cancelling_goals,
        const CANCEL_CODE& cancel_code)
{
    nlohmann::json j;
    j["return_code"] = cancel_code;
    j["goals_canceling"] = nlohmann::json::array();
    for (const auto& [goal_id, timestamp] : cancelling_goals)
    {
        nlohmann::json goal_json;
        goal_json["goal_id"]["uuid"] = goal_id;
        auto duration_since_epoch = timestamp.time_since_epoch();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
        auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epoch).count() % 1'000'000'000;
        goal_json["stamp"]["sec"] = static_cast<int64_t>(sec);
        goal_json["stamp"]["nanosec"] = static_cast<uint32_t>(nanosec);
        j["goals_canceling"].push_back(goal_json);
    }
    return j.dump(4);
}

std::string create_result_request_msg(
        const UUID& goal_id)
{
    nlohmann::json j;
    j["goal_id"]["uuid"] = goal_id;
    return j.dump(4);
}

std::string create_result_reply_msg(
        const STATUS_CODE& status_code,
        const char* json)
{
    nlohmann::json j;
    j["status"] = status_code;
    j["result"] = nlohmann::json::parse(json);
    return j.dump(4);
}

std::string create_status_msg(
        const UUID& goal_id,
        const STATUS_CODE& status_code,
        std::chrono::system_clock::time_point goal_accepted_stamp)
{
    auto duration_since_epoch = goal_accepted_stamp.time_since_epoch();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
    auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epoch).count() % 1'000'000'000;

    nlohmann::json goal_info;
    goal_info["goal_id"]["uuid"] = goal_id;
    goal_info["stamp"]["sec"] = static_cast<int64_t>(sec);
    goal_info["stamp"]["nanosec"] = static_cast<uint32_t>(nanosec);


    nlohmann::json goal_status;
    goal_status["goal_info"] = goal_info;
    goal_status["status"] = status_code;

    nlohmann::json j;
    j["status_list"] = nlohmann::json::array({goal_status});

    return j.dump(4);
}

std::string create_feedback_msg(
        const char* json,
        const UUID& goal_id)
{
    nlohmann::json j;
    j["goal_id"]["uuid"] = goal_id;
    j["feedback"] = nlohmann::json::parse(json);
    return j.dump(4);
}

} // namespace RpcUtils
} // namespace participants
} // namespace ddsenabler
} // namespace eprosima

std::ostream& operator<<(std::ostream& os, const eprosima::ddsenabler::participants::UUID& uuid)
{
    for (size_t i = 0; i < uuid.size(); ++i)
    {
        if (i != 0)
            os << "-";
        os << std::to_string(uuid[i]);
    }
    return os;
}
