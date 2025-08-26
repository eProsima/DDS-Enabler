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
RpcType get_rpc_name(const std::string& topic_name, std::string& rpc_name, RPC_PROTOCOL& rpc_protocol)
{
    rpc_protocol = detect_rpc_protocol(topic_name);

    return remove_prefix_suffix(topic_name, rpc_name, rpc_protocol);
}

RPC_PROTOCOL detect_rpc_protocol(const std::string& topic_name)
{
    if (topic_name.rfind(ROS_TOPIC_PREFIX, 0) == 0 ||
        topic_name.rfind(ROS_REQUEST_PREFIX, 0) == 0 ||
        topic_name.rfind(ROS_REPLY_PREFIX, 0) == 0)
    {
        return RPC_PROTOCOL::ROS2;
    }
    else if (topic_name.rfind(FASTDDS_TOPIC_PREFIX, 0) == 0 ||
             topic_name.rfind(FASTDDS_REQUEST_PREFIX, 0) == 0 ||
             topic_name.rfind(FASTDDS_REPLY_PREFIX, 0) == 0)
    {
        return RPC_PROTOCOL::FASTDDS;
    }
    return RPC_PROTOCOL::UNKNOWN;
}

RpcType remove_prefix_suffix(
        const std::string& topic_name,
        std::string& rpc_name,
        RPC_PROTOCOL rpc_protocol)
{
    std::string request_prefix, request_suffix, reply_prefix, reply_suffix, topic_prefix;
    switch (rpc_protocol)
    {
        case RPC_PROTOCOL::ROS2:
            request_prefix = ROS_REQUEST_PREFIX;
            request_suffix = ROS_REQUEST_SUFFIX;
            reply_prefix = ROS_REPLY_PREFIX;
            reply_suffix = ROS_REPLY_SUFFIX;
            topic_prefix = ROS_TOPIC_PREFIX;
            break;
        case RPC_PROTOCOL::FASTDDS:
            request_prefix = FASTDDS_REQUEST_PREFIX;
            request_suffix = FASTDDS_REQUEST_SUFFIX;
            reply_prefix = FASTDDS_REPLY_PREFIX;
            reply_suffix = FASTDDS_REPLY_SUFFIX;
            topic_prefix = FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_RPC_UTILS,
                    "Invalid RPC protocol");
            return RPC_NONE;
    }

    std::string original_topic_name = topic_name;
    rpc_name = topic_name;
    RpcType rpc_type = RPC_NONE;

    if ((rpc_name.rfind(request_prefix, 0) == 0) &&
        (rpc_name.size() >= request_suffix.length()) &&
        (rpc_name.substr(rpc_name.size() - request_suffix.length()) == request_suffix))
    {
        std::string base = rpc_name.substr(request_prefix.length());
        base = base.substr(0, base.size() - (request_suffix.length()));

        if (base.size() >= (sizeof(ACTION_GOAL_SUFFIX) -1) && base.substr(base.size() - (sizeof(ACTION_GOAL_SUFFIX) -1)) == ACTION_GOAL_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - (sizeof(ACTION_GOAL_SUFFIX) -1));
            return ACTION_GOAL_REQUEST;
        }
        else if (base.size() >= (sizeof(ACTION_RESULT_SUFFIX) - 1) && base.substr(base.size() - (sizeof(ACTION_RESULT_SUFFIX) - 1)) == ACTION_RESULT_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - (sizeof(ACTION_RESULT_SUFFIX) - 1));
            return ACTION_RESULT_REQUEST;
        }
        else if (base.size() >= (sizeof(ACTION_CANCEL_SUFFIX) - 1) && base.substr(base.size() - (sizeof(ACTION_CANCEL_SUFFIX) - 1)) == ACTION_CANCEL_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - (sizeof(ACTION_CANCEL_SUFFIX) - 1));
            return ACTION_CANCEL_REQUEST;
        }

        rpc_name = base;
        return RPC_REQUEST;
    }
    if ((rpc_name.rfind(reply_prefix, 0) == 0) &&
        (rpc_name.size() >= reply_suffix.length()) &&
        (rpc_name.substr(rpc_name.size() - reply_suffix.length()) == reply_suffix))
    {
        std::string base = rpc_name.substr(reply_prefix.length());
        base = base.substr(0, base.size() - (reply_suffix.length()));

        if (base.size() >= (sizeof(ACTION_GOAL_SUFFIX) - 1) && base.substr(base.size() - (sizeof(ACTION_GOAL_SUFFIX) - 1)) == ACTION_GOAL_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - (sizeof(ACTION_GOAL_SUFFIX) - 1));
            return ACTION_GOAL_REPLY;
        }
        else if (base.size() >= (sizeof(ACTION_RESULT_SUFFIX) - 1) && base.substr(base.size() - (sizeof(ACTION_RESULT_SUFFIX) - 1)) == ACTION_RESULT_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - (sizeof(ACTION_RESULT_SUFFIX) - 1));
            return ACTION_RESULT_REPLY;
        }
        else if (base.size() >= (sizeof(ACTION_CANCEL_SUFFIX) - 1) && base.substr(base.size() - (sizeof(ACTION_CANCEL_SUFFIX) - 1)) == ACTION_CANCEL_SUFFIX)
        {
            rpc_name = base.substr(0, base.size() - (sizeof(ACTION_CANCEL_SUFFIX) - 1));
            return ACTION_CANCEL_REPLY;
        }

        rpc_name = base;
        return RPC_REPLY;
    }

    // Check for action feedback/status topics
    rpc_name = rpc_name.substr(topic_prefix.length());
    if (rpc_name.size() >= (sizeof(ACTION_FEEDBACK_SUFFIX)) && rpc_name.substr(rpc_name.size() - (sizeof(ACTION_FEEDBACK_SUFFIX))) == (std::string("/") + ACTION_FEEDBACK_SUFFIX))
    {
        rpc_name = rpc_name.substr(0, rpc_name.size() - sizeof(ACTION_FEEDBACK_SUFFIX));
        return ACTION_FEEDBACK;
    }
    else if (rpc_name.size() >= (sizeof(ACTION_STATUS_SUFFIX)) && rpc_name.substr(rpc_name.size() - (sizeof(ACTION_STATUS_SUFFIX))) == (std::string("/") + ACTION_STATUS_SUFFIX))
    {
        rpc_name = rpc_name.substr(0, rpc_name.size() - (sizeof(ACTION_STATUS_SUFFIX)));
        return ACTION_STATUS;
    }

    return RPC_NONE;
}

// TODO
RpcType get_service_name(const std::string& topic_name, std::string& service_name)
{
    service_name = topic_name;

    // Detect and remove prefix
    if (service_name.rfind(ROS_REQUEST_PREFIX, 0) == 0)
    {
        service_name = service_name.substr(3);
        if (service_name.size() >= 7 && service_name.substr(service_name.size() - 7) == ROS_REQUEST_SUFFIX)
        {
            service_name = service_name.substr(0, service_name.size() - 7);
            return RPC_REQUEST;
        }
    }
    else if (service_name.rfind(ROS_REPLY_PREFIX, 0) == 0)
    {
        service_name = service_name.substr(3);
        if (service_name.size() >= 5 && service_name.substr(service_name.size() - 5) == ROS_REPLY_SUFFIX)
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
