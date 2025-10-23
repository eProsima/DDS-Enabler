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
 * @file RpcStructs.cpp
 */

#include <ddsenabler_participants/rpc/RpcStructs.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

RpcAction::RpcAction(
        const std::string& action_name,
        const ddspipe::core::types::RpcTopic& goal,
        const ddspipe::core::types::RpcTopic& result,
        const ddspipe::core::types::RpcTopic& cancel,
        const ddspipe::core::types::DdsTopic& feedback,
        const ddspipe::core::types::DdsTopic& status)
    : action_name(action_name)
    , goal(goal)
    , result(result)
    , cancel(cancel)
    , feedback(feedback)
    , status(status)
{
}

RpcInfo::RpcInfo(
        const std::string& dds_topic_name)
    : topic_name(dds_topic_name)
    , rpc_protocol(RpcProtocol::PROTOCOL_UNKNOWN)
    , rpc_type(RpcType::NONE)
    , service_type(ServiceType::NONE)
    , action_type(ActionType::NONE)
{
    detect_rpc_protocol();
    try
    {
        remove_prefix_suffix();
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Error extracting RPC info from topic name '" + topic_name + "': " + e.what());
    }
}

void RpcInfo::detect_rpc_protocol()
{
    if (topic_name.rfind(ROS_TOPIC_PREFIX, 0) == 0 ||
            topic_name.rfind(ROS_REQUEST_PREFIX, 0) == 0 ||
            topic_name.rfind(ROS_REPLY_PREFIX, 0) == 0)
    {
        rpc_protocol = RpcProtocol::ROS2;
        return;
    }
    // With the current Fast Prefixes being empty strings, this check must be the last one
    else if (topic_name.rfind(FASTDDS_TOPIC_PREFIX, 0) == 0 ||
            topic_name.rfind(FASTDDS_REQUEST_PREFIX, 0) == 0 ||
            topic_name.rfind(FASTDDS_REPLY_PREFIX, 0) == 0)
    {
        rpc_protocol = RpcProtocol::FASTDDS;
        return;
    }
    rpc_protocol = RpcProtocol::PROTOCOL_UNKNOWN;
}

void RpcInfo::remove_prefix_suffix()
{
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
            throw std::runtime_error("Invalid RPC protocol");
    }

    std::string base = topic_name;
    if ((base.rfind(request_prefix, 0) == 0) &&
            (base.size() >= request_suffix.length()) &&
            (base.substr(base.size() - request_suffix.length()) == request_suffix))
    {
        base = base.substr(request_prefix.length());
        base = base.substr(0, base.size() - (request_suffix.length()));
        service_type = ServiceType::REQUEST;
        service_name = base;

        if (base.size() >= (std::strlen(ACTION_GOAL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_GOAL_SUFFIX))) == ACTION_GOAL_SUFFIX)
        {
            action_name = base.substr(0, base.size() - (std::strlen(ACTION_GOAL_SUFFIX)));
            rpc_type = RpcType::ACTION;
            action_type = ActionType::GOAL;
            return;
        }
        else if (base.size() >= (std::strlen(ACTION_RESULT_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_RESULT_SUFFIX))) == ACTION_RESULT_SUFFIX)
        {
            action_name = base.substr(0, base.size() - (std::strlen(ACTION_RESULT_SUFFIX)));
            rpc_type = RpcType::ACTION;
            action_type = ActionType::RESULT;
            return;
        }
        else if (base.size() >= (std::strlen(ACTION_CANCEL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_CANCEL_SUFFIX))) == ACTION_CANCEL_SUFFIX)
        {
            action_name = base.substr(0, base.size() - (std::strlen(ACTION_CANCEL_SUFFIX)));
            rpc_type = RpcType::ACTION;
            action_type = ActionType::CANCEL;
            return;
        }

        rpc_type = RpcType::SERVICE;
        return;
    }
    if ((base.rfind(reply_prefix, 0) == 0) &&
            (base.size() >= reply_suffix.length()) &&
            (base.substr(base.size() - reply_suffix.length()) == reply_suffix))
    {
        base = base.substr(reply_prefix.length());
        base = base.substr(0, base.size() - (reply_suffix.length()));
        service_type = ServiceType::REPLY;
        service_name = base;

        if (base.size() >= (std::strlen(ACTION_GOAL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_GOAL_SUFFIX))) == ACTION_GOAL_SUFFIX)
        {
            action_name = base.substr(0, base.size() - (std::strlen(ACTION_GOAL_SUFFIX)));
            rpc_type = RpcType::ACTION;
            action_type = ActionType::GOAL;
            return;
        }
        else if (base.size() >= (std::strlen(ACTION_RESULT_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_RESULT_SUFFIX))) == ACTION_RESULT_SUFFIX)
        {
            action_name = base.substr(0, base.size() - (std::strlen(ACTION_RESULT_SUFFIX)));
            rpc_type = RpcType::ACTION;
            action_type = ActionType::RESULT;
            return;
        }
        else if (base.size() >= (std::strlen(ACTION_CANCEL_SUFFIX)) &&
                base.substr(base.size() - (std::strlen(ACTION_CANCEL_SUFFIX))) == ACTION_CANCEL_SUFFIX)
        {
            action_name = base.substr(0, base.size() - (std::strlen(ACTION_CANCEL_SUFFIX)));
            rpc_type = RpcType::ACTION;
            action_type = ActionType::CANCEL;
            return;
        }

        rpc_type = RpcType::SERVICE;
        return;
    }

    // Check for action feedback/status topics
    base = base.substr(topic_prefix.length());
    if (base.size() >= (std::strlen(ACTION_FEEDBACK_SUFFIX) + 1) &&
            base.substr(base.size() - (std::strlen(ACTION_FEEDBACK_SUFFIX) + 1)) ==
            (std::string("/") + ACTION_FEEDBACK_SUFFIX))
    {
        action_name = base.substr(0, base.size() - std::strlen(ACTION_FEEDBACK_SUFFIX));
        rpc_type = RpcType::ACTION;
        action_type = ActionType::FEEDBACK;
        return;
    }
    if (base.size() >= (std::strlen(ACTION_STATUS_SUFFIX) + 1) &&
            base.substr(base.size() - (std::strlen(ACTION_STATUS_SUFFIX) + 1)) ==
            (std::string("/") + ACTION_STATUS_SUFFIX))
    {
        action_name = base.substr(0, base.size() - (std::strlen(ACTION_STATUS_SUFFIX)));
        rpc_type = RpcType::ACTION;
        action_type = ActionType::STATUS;
        return;
    }

    return;
}

ActionRequestInfo::ActionRequestInfo(
        const std::string& _action_name,
        ActionType action_type,
        uint64_t request_id,
        RpcProtocol rpc_protocol)
    : action_name(_action_name)
    , goal_accepted_stamp(std::chrono::system_clock::now())
    , rpc_protocol(rpc_protocol)
{
    set_request(request_id, action_type);
}

void ActionRequestInfo::set_request(
        uint64_t request_id,
        ActionType action_type)
{
    switch (action_type)
    {
        case ActionType::GOAL:
            goal_request_id = request_id;
            break;
        case ActionType::RESULT:
            result_request_id = request_id;
            break;
        default:
            break;
    }
    return;
}

uint64_t ActionRequestInfo::get_request(
        ActionType action_type) const
{
    switch (action_type)
    {
        case ActionType::GOAL:
            return goal_request_id;
        case ActionType::RESULT:
            return result_request_id;
        default:
            throw std::runtime_error("Invalid action type for request retrieval");
    }
}

bool ActionRequestInfo::set_result(
        const std::string&& str)
{
    if (str.empty() || !result.empty())
    {
        return false; // Cannot set string if already set or empty
    }
    result = std::move(str);
    return true;
}

bool ActionRequestInfo::erase(
        ActionEraseReason erase_reason)
{
    switch (erase_reason)
    {
        case ActionEraseReason::FINAL_STATUS:
            final_status_received = true;
            break;
        case ActionEraseReason::RESULT:
            result_received = true;
            break;
        case ActionEraseReason::FORCED:
            final_status_received = true;
            result_received = true;
            break;
    }
    return final_status_received && result_received;
}

RpcProtocol ActionRequestInfo::get_rpc_protocol() const
{
    return rpc_protocol;
}

ServiceDiscovered::ServiceDiscovered(
        const std::string& service_name,
        RpcProtocol rpc_protocol)
    : service_name(service_name)
    , rpc_protocol(rpc_protocol)
{
}

bool ServiceDiscovered::add_topic(
        const ddspipe::core::types::DdsTopic& topic,
        ServiceType service_type)
{
    if (service_type == ServiceType::REQUEST)
    {
        if (request_discovered)
        {
            return false;
        }
        topic_request = topic;
        request_discovered = true;
    }
    else
    {
        if (reply_discovered)
        {
            return false;
        }
        topic_reply = topic;
        reply_discovered = true;
    }

    if (request_discovered && reply_discovered)
    {
        if (service_name.empty())
        {
            return false;
        }
        fully_discovered = true;
        rpc_topic = std::make_optional<ddspipe::core::types::RpcTopic>(
            service_name,
            topic_request,
            topic_reply);
        return true;
    }

    return false;
}

bool ServiceDiscovered::remove_topic(
        ServiceType service_type)
{
    if (service_type == ServiceType::REQUEST)
    {
        request_discovered = false;
        topic_request = ddspipe::core::types::DdsTopic();
    }
    else
    {
        reply_discovered = false;
        topic_reply = ddspipe::core::types::DdsTopic();
    }

    fully_discovered = false;
    rpc_topic = std::nullopt;

    return true;
}

ddspipe::core::types::RpcTopic ServiceDiscovered::get_service()
{
    if (!fully_discovered || rpc_topic == std::nullopt)
    {
        throw std::runtime_error("Service " + service_name + " not fully discovered");
    }
    return rpc_topic.value();
}

bool ServiceDiscovered::get_topic(
        ServiceType service_type,
        ddspipe::core::types::DdsTopic& topic)
{
    if (service_type == ServiceType::REQUEST)
    {
        if (!request_discovered)
        {
            return false;
        }
        topic = topic_request;
        return true;
    }
    if (service_type == ServiceType::REPLY)
    {
        if (!reply_discovered)
        {
            return false;
        }
        topic = topic_reply;
        return true;
    }
    return false;
}

RpcProtocol ServiceDiscovered::get_rpc_protocol() const
{
    return rpc_protocol;
}

ActionDiscovered::ActionDiscovered(
        const std::string& action_name,
        RpcProtocol rpc_protocol)
    : action_name(action_name)
    , rpc_protocol(rpc_protocol)
{
}

bool ActionDiscovered::check_fully_discovered()
{
    auto g = goal.lock();
    auto r = result.lock();
    auto c = cancel.lock();

    if (g && r && c &&
            g->fully_discovered && r->fully_discovered && c->fully_discovered &&
            feedback_discovered && status_discovered)
    {
        fully_discovered = true;
        return true;
    }
    fully_discovered = false;
    return false;
}

bool ActionDiscovered::add_service(
        std::shared_ptr<ServiceDiscovered> service,
        ActionType action_type)
{
    switch (action_type)
    {
        case ActionType::GOAL:
            goal = service;
            break;
        case ActionType::RESULT:
            result = service;
            break;
        case ActionType::CANCEL:
            cancel = service;
            break;
        default:
            return false;
    }
    return true;
}

bool ActionDiscovered::add_topic(
        const ddspipe::core::types::DdsTopic& topic,
        ActionType action_type)
{
    switch (action_type)
    {
        case ActionType::FEEDBACK:
            feedback = topic;
            feedback_discovered = true;
            break;
        case ActionType::STATUS:
            status = topic;
            status_discovered = true;
            break;
        default:
            return false;
    }

    return true;
}

RpcAction ActionDiscovered::get_action()
{
    auto g = goal.lock();
    auto r = result.lock();
    auto c = cancel.lock();

    if (!fully_discovered || !g || !r || !c)
    {
        throw std::runtime_error("Action not fully discovered or ServiceDiscovered expired");
    }

    try
    {
        return RpcAction(
            action_name,
            g->get_service(),
            r->get_service(),
            c->get_service(),
            feedback,
            status);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to create action " + action_name + ": " + e.what());
    }
}

} // namespace participants
} // namespace ddsenabler
} // namespace eprosima
