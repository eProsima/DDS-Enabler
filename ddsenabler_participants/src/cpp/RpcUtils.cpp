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
