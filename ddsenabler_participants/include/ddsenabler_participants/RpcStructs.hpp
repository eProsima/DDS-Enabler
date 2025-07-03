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

#include <optional>

#include <ddspipe_core/types/dds/Endpoint.hpp>
#include <ddsenabler_participants/RpcUtils.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

enum class ActionEraseReason
{
    FINAL_STATUS,  // Erase triggered by receiving final status (e.g., succeeded, aborted, canceled)
    RESULT,        // Erase triggered by receiving result
    FORCED         // Force erase regardless of status/result
};

struct ActionRequestInfo
{
    ActionRequestInfo() = default;

    ActionRequestInfo(
            const std::string& _action_name,
            RpcUtils::ActionType action_type,
            uint64_t request_id)
            : action_name(_action_name)
            , goal_accepted_stamp(std::chrono::system_clock::now())
    {
        set_request(request_id, action_type);
    }

    void set_request(
            uint64_t request_id,
            RpcUtils::ActionType action_type)
    {
        switch (action_type)
        {
            case RpcUtils::ActionType::GOAL:
                goal_request_id = request_id;
                break;
            case RpcUtils::ActionType::RESULT:
                result_request_id = request_id;
                break;
            default:
                break;
        }
        return;
    }

    uint64_t get_request(
            RpcUtils::ActionType action_type) const
    {
        switch (action_type)
        {
            case RpcUtils::ActionType::GOAL:
                return goal_request_id;
            case RpcUtils::ActionType::RESULT:
                return result_request_id;
            default:
                return 0;
        }
    }

    bool set_result(const std::string&& str)
    {
        if (str.empty() || !result.empty())
        {
            return false; // Cannot set string if already set or empty
        }
        result = std::move(str);
        return true;
    }

    bool erase(ActionEraseReason erase_reason)
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

    std::string action_name;
    uint64_t goal_request_id = 0;
    uint64_t result_request_id = 0;
    std::chrono::system_clock::time_point goal_accepted_stamp;
    std::string result;
    bool result_received = false; // Indicates if the result has been received
    bool final_status_received = false; // Indicates if the final status has been received
};

struct ServiceDiscovered
{

    ServiceDiscovered(const std::string& service_name)
        : service_name(service_name)
    {
    }

    std::string service_name;

    ddspipe::core::types::DdsTopic topic_request;
    bool request_discovered{false};

    ddspipe::core::types::DdsTopic topic_reply;
    bool reply_discovered{false};

    std::optional<ddspipe::core::types::RpcTopic> rpc_topic;
    bool fully_discovered{false};

    std::optional<ddspipe::core::types::Endpoint> endpoint_request;
    bool enabler_as_server{false};

    bool add_topic(
            const ddspipe::core::types::DdsTopic& topic,
            RpcUtils::RpcType rpc_type)
    {
        if(rpc_type == RpcUtils::RpcType::RPC_REQUEST)
        {
                if(request_discovered)
                        return false;
                topic_request = topic;
                request_discovered = true;
        }
        else
        {
                if(reply_discovered)
                        return false;
                topic_reply = topic;
                reply_discovered = true;
        }

        if(request_discovered && reply_discovered)
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

    bool remove_topic(RpcUtils::RpcType rpc_type)
    {
        if(rpc_type == RpcUtils::RpcType::RPC_REQUEST)
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

    ddspipe::core::types::RpcTopic get_service()
    {
        if(!fully_discovered || rpc_topic == std::nullopt)
            throw std::runtime_error("Service not fully discovered");
        return rpc_topic.value();
    }

    bool get_topic(
            const RpcUtils::RpcType& rpc_type,
            ddspipe::core::types::DdsTopic& topic)
    {
        if(rpc_type == RpcUtils::RpcType::RPC_REQUEST)
        {
            if(!request_discovered)
                return false;
            topic = topic_request;
            return true;
        }
        if (rpc_type == RpcUtils::RpcType::RPC_REPLY)
        {
            if(!reply_discovered)
                return false;
            topic = topic_reply;
            return true;
        }
        return false;
    }
};

struct ActionDiscovered
{
    ActionDiscovered(const std::string& action_name)
        : action_name(action_name)
    {
    }

    std::string action_name;
    std::weak_ptr<ServiceDiscovered> goal;
    std::weak_ptr<ServiceDiscovered> result;
    std::weak_ptr<ServiceDiscovered> cancel;
    ddspipe::core::types::DdsTopic feedback;
    bool feedback_discovered{false};
    ddspipe::core::types::DdsTopic status;
    bool status_discovered{false};
    bool fully_discovered{false};
    bool enabler_as_server{false};

    bool check_fully_discovered()
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

    bool add_service(
            std::shared_ptr<ServiceDiscovered> service,
            RpcUtils::RpcType rpc_type)
    {
        switch (rpc_type)
        {
            case RpcUtils::RpcType::ACTION_GOAL_REQUEST:
            case RpcUtils::RpcType::ACTION_GOAL_REPLY:
                goal = service;
                break;
            case RpcUtils::RpcType::ACTION_RESULT_REQUEST:
            case RpcUtils::RpcType::ACTION_RESULT_REPLY:
                result = service;
                break;
            case RpcUtils::RpcType::ACTION_CANCEL_REQUEST:
            case RpcUtils::RpcType::ACTION_CANCEL_REPLY:
                cancel = service;
                break;
            default:
                return false;
        }
        return true;
    }


    bool add_topic(
            const ddspipe::core::types::DdsTopic& topic,
            RpcUtils::RpcType rpc_type)
    {
        switch (rpc_type)
        {
            case RpcUtils::RpcType::ACTION_FEEDBACK:
                feedback = topic;
                feedback_discovered = true;
                break;
            case RpcUtils::RpcType::ACTION_STATUS:
                status = topic;
                status_discovered = true;
                break;
            default:
                return false;
        }

        return true;
    }

    RpcUtils::RpcAction get_action(const std::string& action_name)
    {
        auto g = goal.lock();
        auto r = result.lock();
        auto c = cancel.lock();

        if (!fully_discovered || !g || !r || !c)
            throw std::runtime_error("Action not fully discovered or ServiceDiscovered expired");

        return RpcUtils::RpcAction(
            action_name,
            g->get_service(),
            r->get_service(),
            c->get_service(),
            feedback,
            status);
    }
};

} // namespace participants
} // namespace ddsenabler
} // namespace eprosima