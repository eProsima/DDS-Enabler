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
 * @file RpcStructs.hpp
 */

#pragma once


#include <ddsenabler_participants/rpc/RpcTypes.hpp>
#include <ddsenabler_participants/Constants.hpp>

#include <ddspipe_core/types/dds/Endpoint.hpp>

#include <optional>

namespace eprosima {
namespace ddsenabler {
namespace participants {

enum class ActionEraseReason
{
    FINAL_STATUS,  // Erase triggered by receiving final status (e.g., succeeded, aborted, canceled)
    RESULT,        // Erase triggered by receiving result
    FORCED         // Force erase regardless of status/result
};

struct RpcAction
{
    RpcAction() = default;
    RpcAction(
            const std::string& action_name,
            const ddspipe::core::types::RpcTopic& goal,
            const ddspipe::core::types::RpcTopic& result,
            const ddspipe::core::types::RpcTopic& cancel,
            const ddspipe::core::types::DdsTopic& feedback,
            const ddspipe::core::types::DdsTopic& status);

    std::string action_name;
    ddspipe::core::types::RpcTopic goal;
    ddspipe::core::types::RpcTopic result;
    ddspipe::core::types::RpcTopic cancel;
    ddspipe::core::types::DdsTopic feedback;
    ddspipe::core::types::DdsTopic status;
};

struct RpcInfo
{
    RpcInfo(
            const std::string& dds_topic_name);

    void detect_protocol();

    void extract_rpc_info();

    std::string topic_name;
    std::string service_name;
    std::string action_name;
    Protocol protocol;
    RpcType rpc_type;
    ServiceType service_type;
    ActionType action_type;
};

struct ActionRequestInfo
{
    ActionRequestInfo() = default;

    ActionRequestInfo(
            const std::string& _action_name,
            ActionType action_type,
            uint64_t request_id,
            Protocol protocol);

    void set_request(
            uint64_t request_id,
            ActionType action_type);

    uint64_t get_request(
            ActionType action_type) const;

    bool set_result(
            const std::string&& str);

    bool erase(
            ActionEraseReason erase_reason);

    Protocol get_protocol() const;

    std::string action_name;
    Protocol protocol;
    uint64_t goal_request_id = 0;
    uint64_t result_request_id = 0;
    std::chrono::system_clock::time_point goal_accepted_stamp;
    std::string result;
    bool result_received = false; // Indicates if the result has been received
    bool final_status_received = false; // Indicates if the final status has been received
};

struct ServiceDiscovered
{

    ServiceDiscovered(
            const std::string& service_name,
            Protocol protocol);

    bool add_topic(
            const ddspipe::core::types::DdsTopic& topic,
            ServiceType service_type);

    bool remove_topic(
            ServiceType service_type);

    ddspipe::core::types::RpcTopic get_service();

    bool get_topic(
            ServiceType service_type,
            ddspipe::core::types::DdsTopic& topic);

    Protocol get_protocol() const;

    std::string service_name;
    Protocol protocol{Protocol::PROTOCOL_UNKNOWN};

    ddspipe::core::types::DdsTopic topic_request;
    bool request_discovered{false};

    ddspipe::core::types::DdsTopic topic_reply;
    bool reply_discovered{false};

    std::optional<ddspipe::core::types::RpcTopic> rpc_topic;
    bool fully_discovered{false};

    std::optional<ddspipe::core::types::Endpoint> endpoint_request;
    bool enabler_as_server{false};
    bool external_server{false};
};

struct ActionDiscovered
{
    ActionDiscovered(
            const std::string& action_name,
            Protocol protocol);

    bool check_fully_discovered();

    bool add_service(
            std::shared_ptr<ServiceDiscovered> service,
            ActionType action_type);

    bool add_topic(
            const ddspipe::core::types::DdsTopic& topic,
            ActionType action_type);

    RpcAction get_action();

    std::string action_name;
    Protocol protocol{Protocol::PROTOCOL_UNKNOWN};
    std::weak_ptr<ServiceDiscovered> goal;
    std::weak_ptr<ServiceDiscovered> result;
    std::weak_ptr<ServiceDiscovered> cancel;
    ddspipe::core::types::DdsTopic feedback;
    bool feedback_discovered{false};
    ddspipe::core::types::DdsTopic status;
    bool status_discovered{false};
    bool fully_discovered{false};
    bool enabler_as_server{false};
    bool external_server{false};
};

} // namespace participants
} // namespace ddsenabler
} // namespace eprosima
