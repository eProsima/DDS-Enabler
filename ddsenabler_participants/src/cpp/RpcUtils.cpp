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
RpcInfo get_rpc_info(
        const std::string& topic_name)
{
    RpcProtocol RpcProtocol = detect_rpc_protocol(topic_name);

    return remove_prefix_suffix(topic_name, RpcProtocol);
}

RpcProtocol detect_rpc_protocol(
        const std::string& topic_name)
{
    if (topic_name.rfind(ROS_TOPIC_PREFIX, 0) == 0 ||
            topic_name.rfind(ROS_REQUEST_PREFIX, 0) == 0 ||
            topic_name.rfind(ROS_REPLY_PREFIX, 0) == 0)
    {
        return RpcProtocol::ROS2;
    }
    else if (topic_name.rfind(FASTDDS_TOPIC_PREFIX, 0) == 0 ||
            topic_name.rfind(FASTDDS_REQUEST_PREFIX, 0) == 0 ||
            topic_name.rfind(FASTDDS_REPLY_PREFIX, 0) == 0)
    {
        return RpcProtocol::FASTDDS;
    }
    return RpcProtocol::PROTOCOL_UNKNOWN;
}

RpcInfo remove_prefix_suffix(
        const std::string& topic_name,
        RpcProtocol rpc_protocol)
{
    RpcInfo rpc_info(topic_name);
    rpc_info.rpc_protocol = rpc_protocol;

    std::string request_prefix, request_suffix, reply_prefix, reply_suffix, topic_prefix;
    switch (rpc_protocol)
    {
        case RpcProtocol::ROS2:
            request_prefix = ROS_REQUEST_PREFIX;
            request_suffix = ROS_REQUEST_SUFFIX;
            reply_prefix = ROS_REPLY_PREFIX;
            reply_suffix = ROS_REPLY_SUFFIX;
            topic_prefix = ROS_TOPIC_PREFIX;
            break;
        case RpcProtocol::FASTDDS:
            request_prefix = FASTDDS_REQUEST_PREFIX;
            request_suffix = FASTDDS_REQUEST_SUFFIX;
            reply_prefix = FASTDDS_REPLY_PREFIX;
            reply_suffix = FASTDDS_REPLY_SUFFIX;
            topic_prefix = FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_RPC_UTILS,
                    "Invalid RPC protocol");
            return rpc_info;
    }

    std::string base = topic_name;
    if ((base.rfind(request_prefix, 0) == 0) &&
            (base.size() >= request_suffix.length()) &&
            (base.substr(base.size() - request_suffix.length()) == request_suffix))
    {
        base = base.substr(request_prefix.length());
        base = base.substr(0, base.size() - (request_suffix.length()));
        rpc_info.service_type = ServiceType::REQUEST;
        rpc_info.service_name = base;

        if (base.size() >= (std::strlen(ACTION_GOAL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_GOAL_SUFFIX))) == ACTION_GOAL_SUFFIX)
        {
            rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_GOAL_SUFFIX)));
            rpc_info.rpc_type = RpcType::ACTION;
            rpc_info.action_type = ActionType::GOAL;
            return rpc_info;
        }
        else if (base.size() >= (std::strlen(ACTION_RESULT_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_RESULT_SUFFIX))) == ACTION_RESULT_SUFFIX)
        {
            rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_RESULT_SUFFIX)));
            rpc_info.rpc_type = RpcType::ACTION;
            rpc_info.action_type = ActionType::RESULT;
            return rpc_info;
        }
        else if (base.size() >= (std::strlen(ACTION_CANCEL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_CANCEL_SUFFIX))) == ACTION_CANCEL_SUFFIX)
        {
            rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_CANCEL_SUFFIX)));
            rpc_info.rpc_type = RpcType::ACTION;
            rpc_info.action_type = ActionType::CANCEL;
            return rpc_info;
        }

        rpc_info.rpc_type = RpcType::SERVICE;
        return rpc_info;
    }
    if ((base.rfind(reply_prefix, 0) == 0) &&
            (base.size() >= reply_suffix.length()) &&
            (base.substr(base.size() - reply_suffix.length()) == reply_suffix))
    {
        base = base.substr(reply_prefix.length());
        base = base.substr(0, base.size() - (reply_suffix.length()));
        rpc_info.service_type = ServiceType::REPLY;
        rpc_info.service_name = base;

        if (base.size() >= (std::strlen(ACTION_GOAL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_GOAL_SUFFIX))) == ACTION_GOAL_SUFFIX)
        {
            rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_GOAL_SUFFIX)));
            rpc_info.rpc_type = RpcType::ACTION;
            rpc_info.action_type = ActionType::GOAL;
            return rpc_info;
        }
        else if (base.size() >= (std::strlen(ACTION_RESULT_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_RESULT_SUFFIX))) == ACTION_RESULT_SUFFIX)
        {
            rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_RESULT_SUFFIX)));
            rpc_info.rpc_type = RpcType::ACTION;
            rpc_info.action_type = ActionType::RESULT;
            return rpc_info;
        }
        else if (base.size() >= (std::strlen(ACTION_CANCEL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_CANCEL_SUFFIX))) == ACTION_CANCEL_SUFFIX)
        {
            rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_CANCEL_SUFFIX)));
            rpc_info.rpc_type = RpcType::ACTION;
            rpc_info.action_type = ActionType::CANCEL;
            return rpc_info;
        }

        rpc_info.rpc_type = RpcType::SERVICE;
        return rpc_info;
    }

    // Check for action feedback/status topics
    base = base.substr(topic_prefix.length());
    if (base.size() >= (std::strlen(ACTION_FEEDBACK_SUFFIX) + 1) &&
            base.substr(base.size() - (std::strlen(ACTION_FEEDBACK_SUFFIX) + 1)) ==
            (std::string("/") + ACTION_FEEDBACK_SUFFIX))
    {
        rpc_info.action_name = base.substr(0, base.size() - std::strlen(ACTION_FEEDBACK_SUFFIX));
        rpc_info.rpc_type = RpcType::ACTION;
        rpc_info.action_type = ActionType::FEEDBACK;
        return rpc_info;
    }
    if (base.size() >= (std::strlen(ACTION_STATUS_SUFFIX) + 1) &&
            base.substr(base.size() - (std::strlen(ACTION_STATUS_SUFFIX) + 1)) ==
            (std::string("/") + ACTION_STATUS_SUFFIX))
    {
        rpc_info.action_name = base.substr(0, base.size() - (std::strlen(ACTION_STATUS_SUFFIX)));
        rpc_info.rpc_type = RpcType::ACTION;
        rpc_info.action_type = ActionType::STATUS;
        return rpc_info;
    }

    return rpc_info;
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

std::string create_goal_request_msg(
        const std::string& goal_json,
        UUID& goal_id)
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
        const CancelCode& cancel_code)
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
        auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epoch).count() %
                1'000'000'000;
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
        const StatusCode& status_code,
        const char* json)
{
    nlohmann::json j;
    j["status"] = status_code;
    j["result"] = nlohmann::json::parse(json);
    return j.dump(4);
}

std::string create_status_msg(
        const UUID& goal_id,
        const StatusCode& status_code,
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

std::ostream& operator <<(
        std::ostream& os,
        const UUID& uuid)
{
    for (size_t i = 0; i < uuid.size(); ++i)
    {
        if (i != 0)
        {
            os << "-";
        }
        os << std::to_string(uuid[i]);
    }
    return os;
}

} // namespace RpcUtils
} // namespace participants
} // namespace ddsenabler
} // namespace eprosima
