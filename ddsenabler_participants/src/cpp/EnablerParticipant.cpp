// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file EnablerParticipant.cpp
 */

#include <ddspipe_core/types/data/RtpsPayloadData.hpp>
#include <ddspipe_core/types/data/RpcPayloadData.hpp>
#include <ddspipe_core/types/dds/Payload.hpp>
#include <ddspipe_core/types/dynamic_types/types.hpp>
#include <ddspipe_participants/participant/rtps/CommonParticipant.hpp>
#include <ddspipe_participants/reader/auxiliar/BlankReader.hpp>

#include <ddsenabler_participants/Handler.hpp>
#include <ddsenabler_participants/Serialization.hpp>

#include <ddsenabler_participants/EnablerParticipant.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

using namespace eprosima::ddspipe::core;
using namespace eprosima::ddspipe::core::types;
using namespace eprosima::ddspipe::participants;

EnablerParticipant::EnablerParticipant(
        std::shared_ptr<EnablerParticipantConfiguration> participant_configuration,
        std::shared_ptr<PayloadPool> payload_pool,
        std::shared_ptr<DiscoveryDatabase> discovery_database,
        std::shared_ptr<ISchemaHandler> schema_handler)
    : ddspipe::participants::SchemaParticipant(participant_configuration, payload_pool, discovery_database,
            schema_handler)
    , handler_(std::static_pointer_cast<Handler>(schema_handler_))
{
}

std::shared_ptr<IReader> EnablerParticipant::create_reader(
        const ITopic& topic)
{
    if (is_type_object_topic(topic))
    {
        return std::make_shared<BlankReader>();
    }

    std::shared_ptr<InternalReader> reader;
    {
        std::lock_guard<std::mutex> lck(mtx_);
        auto dds_topic = dynamic_cast<const DdsTopic&>(topic);
        std::shared_ptr<RpcInfo> rpc_info;
        try
        {
            rpc_info = std::make_shared<RpcInfo>(dds_topic.m_topic_name);
        }
        catch (const std::exception& e)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT, e.what());
            return std::make_shared<BlankReader>();
        }

        if (RpcType::NONE != rpc_info->rpc_type)
        {
            if (ServiceType::NONE != rpc_info->service_type)
            {
                reader = std::make_shared<InternalRpcReader>(id(), dds_topic);
            }
            else
            {
                reader = std::make_shared<InternalReader>(id());
            }

            // Only notify the discovery of topics that do not originate from a topic query callback
            if (dds_topic.topic_discoverer() != this->id())
            {
                if (RpcType::SERVICE == rpc_info->rpc_type)
                {
                    if (service_discovered_nts_(rpc_info, dds_topic))
                    {
                        try
                        {
                            RpcTopic service = services_.find(rpc_info->service_name)->second->get_service();
                            handler_->add_service(service);
                        }
                        catch (const std::exception& e)
                        {
                            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                                    "Failed to add service " << rpc_info->service_name << ": " << e.what());
                            return std::make_shared<BlankReader>();
                        }
                    }
                }
                else if (RpcType::ACTION == rpc_info->rpc_type)
                {
                    if (action_discovered_nts_(rpc_info, dds_topic))
                    {
                        try
                        {
                            auto action = actions_.find(rpc_info->action_name)->second->get_action();
                            handler_->add_action(action);
                        }
                        catch (const std::exception& e)
                        {
                            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                                    "Failed to add action " << rpc_info->action_name << ": " << e.what());
                            return std::make_shared<BlankReader>();
                        }
                    }
                }
            }
        }
        else
        {
            reader = std::make_shared<InternalReader>(id());
            // Only notify the discovery of topics that do not originate from a topic query callback
            if (dds_topic.topic_discoverer() != this->id())
            {
                handler_->add_topic(dds_topic);
            }
        }
        readers_[dds_topic] = reader;
    }
    cv_.notify_all();
    return reader;
}

bool EnablerParticipant::publish(
        const std::string& topic_name,
        const std::string& json)
{
    if (topic_name.empty())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data: topic name is empty.");
        return false;
    }

    std::unique_lock<std::mutex> lck(mtx_);

    std::string type_name;
    auto i_reader = lookup_reader_nts_(topic_name, type_name);

    if (nullptr == i_reader)
    {
        DdsTopic topic;
        if (!query_topic_nts_(topic_name, topic))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to publish data in topic " << topic_name);
            return false;
        }

        if (!create_topic_writer_nts_(topic, i_reader, lck))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to publish data in topic " << topic_name << " : writer creation failed.");
            return false;
        }

        type_name = topic.type_name;

        // (Optionally) wait for writer created in DDS participant to match with external readers, to avoid losing this
        // message when not using transient durability
        std::this_thread::sleep_for(std::chrono::milliseconds(std::static_pointer_cast<EnablerParticipantConfiguration>(
                    configuration_)->initial_publish_wait));
    }

    auto reader = std::dynamic_pointer_cast<InternalReader>(i_reader);

    auto data = std::make_unique<RtpsPayloadData>();

    Payload payload;
    if (!handler_->get_serialized_data(type_name, json, payload))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic_name << " : data serialization failed.");
        return false;
    }

    if (!payload_pool_->get_payload(payload, data->payload))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic_name << " : get_payload failed.");
        return false;
    }

    reader->simulate_data_reception(std::move(data));
    return true;
}

bool EnablerParticipant::publish_rpc(
        const std::string& topic_name,
        const std::string& json,
        const uint64_t request_id)
{
    std::unique_lock<std::mutex> lck(mtx_);

    std::shared_ptr<RpcInfo> rpc_info;
    try
    {
        rpc_info = std::make_shared<RpcInfo>(topic_name);
    }
    catch (const std::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT, e.what());
        return false;
    }

    if (ServiceType::NONE == rpc_info->service_type)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic_name << " : not a service topic.");
        return false;
    }

    auto it = services_.find(rpc_info->service_name);
    if (it == services_.end())
    {
        // There is no case where none of the service topics are discovered and yet the publish should be done
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in service " << rpc_info->service_name << " : service does not exist.");
        return false;
    }

    if (!it->second->external_server && rpc_info->service_type == ServiceType::REQUEST)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in service " << rpc_info->service_name <<
                            " : service is only announced on the enabler side.");
        return false;
    }

    std::string type_name;
    auto reader = std::dynamic_pointer_cast<InternalRpcReader>(lookup_reader_nts_(topic_name, type_name));

    if (nullptr == reader)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in service " << rpc_info->service_name << " : service does not exist.");
        return false;
    }

    DdsTopic topic;
    if (!it->second->get_topic(rpc_info->service_type, topic))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in service " << rpc_info->service_name << " : topic not found.");
        return false;
    }

    if (type_name != topic.type_name)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic.m_topic_name << " : type name mismatch.");
        return false;
    }

    auto data = std::make_unique<RpcPayloadData>();

    Payload payload;
    if (!handler_->get_serialized_data(type_name, json, payload))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic.m_topic_name << " : data serialization failed.");
        return false;
    }

    if (!payload_pool_->get_payload(payload, data->payload))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic.m_topic_name << " : get_payload failed.");
        return false;
    }

    data->origin_sequence_number = eprosima::fastdds::rtps::SequenceNumber_t(request_id);
    data->sent_sequence_number = eprosima::fastdds::rtps::SequenceNumber_t(request_id);
    data->participant_receiver = id();

    fastdds::rtps::SampleIdentity sample_identity;
    sample_identity.sequence_number(fastdds::rtps::SequenceNumber_t(request_id));
    sample_identity.writer_guid(reader->guid());
    data->write_params.get_reference().sample_identity(sample_identity);
    data->write_params.get_reference().related_sample_identity(sample_identity);

    reader->simulate_data_reception(std::move(data));
    return true;
}

bool EnablerParticipant::announce_service(
        const std::string& service_name,
        RpcProtocol RpcProtocol)
{
    std::unique_lock<std::mutex> lck(mtx_);

    auto it = services_.find(service_name);
    if (it != services_.end())
    {
        if (it->second->enabler_as_server)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to announce service " << service_name << " : service already announced.");
            return false;
        }
        it->second->enabler_as_server = true;
        return true;
    }

    std::shared_ptr<ServiceDiscovered> service = std::make_shared<ServiceDiscovered>(service_name, RpcProtocol);
    if (!query_service_nts_(service, RpcProtocol))
    {
        return false;
    }

    if (create_service_request_writer_nts_(service, lck))
    {
        services_.insert_or_assign(service_name, service);
        return true;
    }

    EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
            "Failed to announce service " << service_name << " : service writer creation failed.");
    return false;
}

// TODO after revoking the service the client is still matched
bool EnablerParticipant::revoke_service(
        const std::string& service_name)
{
    std::unique_lock<std::mutex> lck(mtx_);

    return revoke_service_nts_(service_name);
}

bool EnablerParticipant::send_service_request(
        const std::string& service_name,
        const std::string& json,
        uint64_t& request_id,
        RpcProtocol RpcProtocol)
{
    std::string prefix, suffix;
    switch (RpcProtocol)
    {
        case RpcProtocol::ROS2:
            prefix = ROS_REQUEST_PREFIX;
            suffix = ROS_REQUEST_SUFFIX;
            break;
        case RpcProtocol::FASTDDS:
            prefix = FASTDDS_REQUEST_PREFIX;
            suffix = FASTDDS_REQUEST_SUFFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send service request to service " << service_name << ": unknown RPC protocol.");
            return false;
    }

    request_id = handler_->get_new_request_id();
    if (!publish_rpc(
                prefix + service_name + suffix,
                json,
                request_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send service request to service " << service_name);
        return false;
    }

    return true;
}

bool EnablerParticipant::send_service_reply(
        const std::string& service_name,
        const std::string& json,
        const uint64_t request_id)
{
    RpcProtocol RpcProtocol = get_service_rpc_protocol(service_name);
    std::string prefix, suffix;
    switch (RpcProtocol)
    {
        case RpcProtocol::ROS2:
            prefix = ROS_REPLY_PREFIX;
            suffix = ROS_REPLY_SUFFIX;
            break;
        case RpcProtocol::FASTDDS:
            prefix = FASTDDS_REPLY_PREFIX;
            suffix = FASTDDS_REPLY_SUFFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send service reply to unknown service " << service_name);
            return false;
    }

    return publish_rpc(
        prefix + service_name + suffix,
        json,
        request_id);
}

RpcProtocol EnablerParticipant::get_service_rpc_protocol(
        const std::string& service_name)
{
    std::unique_lock<std::mutex> lck(mtx_);
    auto it = services_.find(service_name);
    if (it != services_.end())
    {
        return it->second->get_rpc_protocol();
    }

    return RpcProtocol::PROTOCOL_UNKNOWN;
}

bool EnablerParticipant::announce_action(
        const std::string& action_name,
        RpcProtocol RpcProtocol)
{
    std::unique_lock<std::mutex> lck(mtx_);

    if (RpcProtocol != RpcProtocol::ROS2)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action_name << " : only ROS2 actions are currently supported.");
        return false;
    }

    {
        auto it = actions_.find(action_name);
        if (it != actions_.end())
        {
            if (it->second->enabler_as_server)
            {
                EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                        "Failed to announce action " << action_name << " : action already announced.");
                return false;
            }
            // Erase the action, to allow re-announcing it
            auto goal = it->second->goal.lock();
            auto result = it->second->result.lock();
            auto cancel = it->second->cancel.lock();
            if (goal && result && cancel)
            {
                it->second->enabler_as_server = true;
                goal->enabler_as_server = true;
                result->enabler_as_server = true;
                cancel->enabler_as_server = true;
                return true;
            }
            actions_.erase(it);
        }
    }

    std::shared_ptr<ActionDiscovered> action = std::make_shared<ActionDiscovered>(action_name, RpcProtocol);
    if (!query_action_nts_(*action, RpcProtocol, lck))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action_name << " : action type request failed.");
        return false;
    }

    actions_.insert_or_assign(action_name, action);
    return true;
}

bool EnablerParticipant::revoke_action(
        const std::string& action_name)
{
    std::unique_lock<std::mutex> lck(mtx_);

    auto it = actions_.find(action_name);
    if (it == actions_.end())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to revoke action " << action_name << " : action not found.");
        return false;
    }

    if (!it->second->enabler_as_server)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to revoke action " << action_name << " : action not announced as server.");
        return false;
    }

    if (it->second->external_server)
    {
        it->second->enabler_as_server = false;
        return true;
    }

    auto action = it->second;

    auto goal = action->goal.lock();
    auto result = action->result.lock();
    auto cancel = action->cancel.lock();
    if (!goal || !result || !cancel)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to revoke action " << action_name << " : action services not fully discovered.");
        return false;
    }
    if (this->revoke_service_nts_(goal->service_name) &&
            this->revoke_service_nts_(result->service_name) &&
            this->revoke_service_nts_(cancel->service_name))
    {
        action->enabler_as_server = false;
        action->fully_discovered = false;
        return true;
    }

    EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
            "Failed to revoke action " << action_name << " : error revoking action services.");
    return false;
}

bool EnablerParticipant::send_action_goal(
        const std::string& action_name,
        const std::string& json,
        UUID& action_id,
        RpcProtocol RpcProtocol)
{
    std::string goal_json = RpcUtils::create_goal_request_msg(json, action_id);
    std::string goal_request_topic = action_name + ACTION_GOAL_SUFFIX;
    uint64_t goal_request_id = 0;

    if (!send_service_request(
                goal_request_topic,
                goal_json,
                goal_request_id,
                RpcProtocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action goal request to action " << action_name);
        return false;
    }

    if (!handler_->store_action_request(
                action_name,
                action_id,
                goal_request_id,
                ActionType::GOAL,
                RpcProtocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to store action goal request to action " << action_name);
        handler_->erase_action_UUID(action_id, ActionEraseReason::FORCED);
        return false;
    }

    return true;
}

bool EnablerParticipant::cancel_action_goal(
        const std::string& action_name,
        const UUID& goal_id,
        const int64_t timestamp)
{
    if (goal_id != UUID() &&
            !handler_->is_UUID_active(action_name, goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to cancel action goal for action " << action_name
                                                           << ": goal id not found.");
        return false;
    }

    std::string cancel_json = RpcUtils::create_cancel_request_msg(goal_id, timestamp);

    RpcProtocol protocol = handler_->get_action_rpc_protocol(action_name, goal_id);

    uint64_t cancel_request_id = 0;
    std::string cancel_request_topic = action_name + ACTION_CANCEL_SUFFIX;

    if (send_service_request(
                cancel_request_topic,
                cancel_json,
                cancel_request_id,
                protocol))
    {
        return true;
    }

    EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
            "Failed to send action cancel goal to action " << action_name);
    return false;
}

bool EnablerParticipant::send_action_get_result_request(
        const std::string& action_name,
        const UUID& action_id)
{
    std::string json = RpcUtils::create_result_request_msg(action_id);

    std::string get_result_request_topic = action_name + ACTION_RESULT_SUFFIX;
    uint64_t get_result_request_id = 0;

    RpcProtocol protocol = handler_->get_action_rpc_protocol(action_name, action_id);

    if (!send_service_request(
                get_result_request_topic,
                json,
                get_result_request_id,
                protocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action get result request to action " << action_name);
        return false;
    }

    if (!handler_->store_action_request(
                action_name,
                action_id,
                get_result_request_id,
                ActionType::RESULT))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to store action get result request to action " << action_name
                                                                       << ": cancelling.");
        cancel_action_goal(action_name, action_id, 0);
        return false;
    }

    return true;
}

void EnablerParticipant::send_action_send_goal_reply(
        const std::string& action_name,
        const uint64_t goal_id,
        bool accepted)
{
    std::string reply_json = RpcUtils::create_goal_reply_msg(accepted);

    if (!send_service_reply(
                action_name + ACTION_GOAL_SUFFIX,
                reply_json,
                goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action goal reply to action " << action_name
                                                              << ": goal id not found.");
    }
    return;
}

bool EnablerParticipant::send_action_cancel_goal_reply(
        const char* action_name,
        const std::vector<UUID>& goal_ids,
        const CancelCode& cancel_code,
        const uint64_t request_id)
{
    std::vector<std::pair<UUID, std::chrono::system_clock::time_point>> cancelling_goals;
    for (const auto& goal_id : goal_ids)
    {
        std::chrono::system_clock::time_point timestamp;
        if (!handler_->is_UUID_active(action_name, goal_id, &timestamp))
        {
            continue;
        }

        cancelling_goals.emplace_back(goal_id, timestamp);
    }
    std::string reply_json = RpcUtils::create_cancel_reply_msg(cancelling_goals, cancel_code);

    if (!send_service_reply(
                std::string(action_name) + ACTION_CANCEL_SUFFIX,
                reply_json,
                request_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action cancel reply to action " << action_name
                                                                << ": request id not found.");
        return false;
    }
    return true;
}

bool EnablerParticipant::send_action_result(
        const char* action_name,
        const UUID& goal_id,
        const StatusCode& status_code,
        const char* json)
{
    if (!handler_->is_UUID_active(action_name, goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action result to action " << action_name
                                                          << ": goal id not found.");
        return false;
    }

    std::string reply_json = RpcUtils::create_result_reply_msg(status_code, json);

    return handler_->handle_action_result(action_name, goal_id, reply_json);
}

bool EnablerParticipant::send_action_get_result_reply(
        const std::string& action_name,
        const UUID& goal_id,
        const std::string& reply_json,
        const uint64_t request_id)
{
    std::string result_topic = action_name + ACTION_RESULT_SUFFIX;

    if (send_service_reply(
                result_topic,
                reply_json,
                request_id))
    {
        handler_->erase_action_UUID(goal_id, ActionEraseReason::FORCED);
        return true;
    }

    EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
            "Failed to send action get result to action " << action_name);

    return false;
}

bool EnablerParticipant::send_action_feedback(
        const char* action_name,
        const char* json,
        const UUID& goal_id)
{
    if (!handler_->is_UUID_active(action_name, goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action feedback to action " << action_name
                                                            << ": goal id not found.");
        return false;
    }

    RpcProtocol protocol = handler_->get_action_rpc_protocol(action_name, goal_id);

    std::string prefix;
    switch (protocol)
    {
        case RpcProtocol::ROS2:
            prefix = ROS_TOPIC_PREFIX;
            break;
        case RpcProtocol::FASTDDS:
            prefix = FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send feedback to action " << action_name
                                                         << ": unsupported RPC protocol.");
            return false;
    }

    std::string feedback_json = RpcUtils::create_feedback_msg(json, goal_id);
    std::string feedback_topic = prefix + std::string(action_name) + ACTION_FEEDBACK_SUFFIX;

    return publish(feedback_topic, feedback_json);
}

bool EnablerParticipant::update_action_status(
        const std::string& action_name,
        const UUID& goal_id,
        const StatusCode& status_code)
{
    std::chrono::system_clock::time_point goal_accepted_stamp;
    if (!handler_->is_UUID_active(action_name, goal_id, &goal_accepted_stamp))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to update action status to action " << action_name
                                                            << ": goal id not found.");
        return false;
    }

    RpcProtocol protocol = handler_->get_action_rpc_protocol(action_name, goal_id);

    std::string prefix;
    switch (protocol)
    {
        case RpcProtocol::ROS2:
            prefix = ROS_TOPIC_PREFIX;
            break;
        case RpcProtocol::FASTDDS:
            prefix = FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send status to action " << action_name
                                                       << ": unsupported RPC protocol.");
            return false;
    }

    std::string status_json = RpcUtils::create_status_msg(goal_id, status_code, goal_accepted_stamp);
    std::string status_topic = prefix + action_name + ACTION_STATUS_SUFFIX;
    return publish(status_topic, status_json);
}

bool EnablerParticipant::query_topic_nts_(
        const std::string& topic_name,
        DdsTopic& topic)
{
    if (!topic_query_callback_)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to query data from topic " << topic_name <<
                " : topic is unknown and topic query callback not set.");
        return false;
    }
    TopicInfo topic_info;
    if (!topic_query_callback_(topic_name.c_str(), topic_info))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to query data from topic " << topic_name << " : topic query callback failed.");
        return false;
    }

    return fill_topic_struct_nts_(topic_name, topic_info, topic);
}

bool EnablerParticipant::query_service_nts_(
        std::shared_ptr<ServiceDiscovered> service,
        RpcProtocol RpcProtocol)
{
    if (!service_query_callback_)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce service " << service->service_name <<
                " : service is unknown and service request callback not set.");
        return false;
    }

    std::string request_type_name;
    std::string serialized_request_qos_content;
    std::string reply_type_name;
    std::string serialized_reply_qos_content;


    ServiceInfo service_info;
    if (!service_query_callback_(service->service_name.c_str(), service_info))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce service " << service->service_name << " : service type request failed.");
        return false;
    }

    return fill_service_type_nts_(service_info, service, RpcProtocol);
}

bool EnablerParticipant::query_action_nts_(
        ActionDiscovered& action,
        RpcProtocol RpcProtocol,
        std::unique_lock<std::mutex>& lck)
{
    if (!action_query_callback_)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name <<
                " : action is unknown and action request callback not set.");
        return false;
    }

    ActionInfo action_info;

    if (!action_query_callback_(action.action_name.c_str(),
            action_info))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : action type request failed.");
        return false;
    }

    std::string goal_service_name = action.action_name + ACTION_GOAL_SUFFIX;
    std::string cancel_service_name = action.action_name + ACTION_CANCEL_SUFFIX;
    std::string result_service_name = action.action_name + ACTION_RESULT_SUFFIX;
    std::vector<std::string> topics_names =
    {
        goal_service_name,
        cancel_service_name,
        result_service_name
    };
    for (const auto& topic_name : topics_names)
    {
        auto it = services_.find(topic_name);
        if (it != services_.end())
        {
            // Erase the service, to allow re-announcing it
            services_.erase(it);
        }
    }

    std::shared_ptr<ServiceDiscovered> goal_service = std::make_shared<ServiceDiscovered>(goal_service_name,
                    RpcProtocol);
    if (!fill_service_type_nts_(
                action_info.goal,
                goal_service,
                RpcProtocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : goal service type not found.");
        return false;
    }
    if (!create_service_request_writer_nts_(goal_service, lck))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : goal service writer creation failed.");
        return false;
    }
    services_.insert_or_assign(goal_service_name, goal_service);
    action.goal = goal_service;

    std::shared_ptr<ServiceDiscovered> cancel_service = std::make_shared<ServiceDiscovered>(cancel_service_name,
                    RpcProtocol);
    if (!fill_service_type_nts_(
                action_info.cancel,
                cancel_service,
                RpcProtocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : cancel service type not found.");
        return false;
    }
    if (!create_service_request_writer_nts_(cancel_service, lck))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : cancel service writer creation failed.");
        return false;
    }
    services_.insert_or_assign(cancel_service_name, cancel_service);
    action.cancel = cancel_service;

    std::shared_ptr<ServiceDiscovered> result_service = std::make_shared<ServiceDiscovered>(result_service_name,
                    RpcProtocol);
    if (!fill_service_type_nts_(
                action_info.result,
                result_service,
                RpcProtocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : result service type not found.");
        return false;
    }
    if (!create_service_request_writer_nts_(result_service, lck))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : result service writer creation failed.");
        return false;
    }
    services_.insert_or_assign(result_service_name, result_service);
    action.result = result_service;

    std::string prefix;
    switch (RpcProtocol)
    {
        case RpcProtocol::ROS2:
            prefix = ROS_TOPIC_PREFIX;
            break;
        case RpcProtocol::FASTDDS:
            prefix = FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to announce action " << action.action_name << " : unsupported RPC protocol.");
            return false;
    }

    std::string feedback_topic_name = prefix + action.action_name + ACTION_FEEDBACK_SUFFIX;
    DdsTopic feedback_topic;
    if (!fill_topic_struct_nts_(
                feedback_topic_name,
                action_info.feedback,
                feedback_topic))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : feedback topic type not found.");
        return false;
    }
    {
        auto feedback_reader = lookup_reader_nts_(feedback_topic_name);
        if (!feedback_reader)
        {
            if (!create_topic_writer_nts_(
                        feedback_topic,
                        feedback_reader,
                        lck))
            {
                EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                        "Failed to announce action " << action.action_name <<
                                    " : feedback topic writer creation failed.");
                return false;
            }
        }
    }
    action.feedback = feedback_topic;
    action.feedback_discovered = true;

    std::string status_topic_name = prefix + action.action_name + ACTION_STATUS_SUFFIX;
    DdsTopic status_topic;
    if (!fill_topic_struct_nts_(
                status_topic_name,
                action_info.status,
                status_topic))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : status topic type not found.");
        return false;
    }
    {
        auto status_reader = lookup_reader_nts_(action.status.m_topic_name);
        if (!status_reader)
        {
            if (!create_topic_writer_nts_(
                        status_topic,
                        status_reader,
                        lck))
            {
                EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                        "Failed to announce action " << action.action_name <<
                                    " : status topic writer creation failed.");
                return false;
            }
        }
    }
    action.status = status_topic;
    action.status_discovered = true;

    action.fully_discovered = true;
    action.enabler_as_server = true;
    return true;
}

bool EnablerParticipant::create_topic_writer_nts_(
        const DdsTopic& topic,
        std::shared_ptr<IReader>& reader,
        std::unique_lock<std::mutex>& lck)
{
    ddspipe::core::types::Endpoint request_edp;
    return create_topic_writer_nts_(topic, reader, request_edp, lck);
}

bool EnablerParticipant::create_topic_writer_nts_(
        const DdsTopic& topic,
        std::shared_ptr<IReader>& reader,
        ddspipe::core::types::Endpoint& request_edp,
        std::unique_lock<std::mutex>& lck)
{
    request_edp = rtps::CommonParticipant::simulate_endpoint(topic, this->id());
    this->discovery_database_->add_endpoint(request_edp);

    // Wait for reader to be created from discovery thread
    // NOTE: Set a timeout to avoid a deadlock in case the reader is never created for some reason (e.g. the topic
    // is blocked or the underlying DDS Pipe object is disabled/destroyed before the reader is created).
    if (!cv_.wait_for(lck, std::chrono::seconds(5), [&]
            {
                return nullptr != (reader = lookup_reader_nts_(topic.m_topic_name));
            }))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to create internal reader for topic " << topic.m_topic_name <<
                " , please verify that the topic is allowed.");
        return false;
    }

    return true;
}

bool EnablerParticipant::create_service_request_writer_nts_(
        std::shared_ptr<ServiceDiscovered> service,
        std::unique_lock<std::mutex>& lck)
{
    auto reader = lookup_reader_nts_(service->topic_request.m_topic_name);

    if (nullptr == reader)
    {
        ddspipe::core::types::Endpoint request_edp;
        if (create_topic_writer_nts_(
                    service->topic_request,
                    reader,
                    request_edp,
                    lck))
        {
            service->endpoint_request = request_edp;
            return true;
        }
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to create service request writer for service " << service->service_name << ".");
        return false;
    }

    EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
            "Failed to create server as there is already a server running for service " << service->service_name <<
            ".");
    return false;
}

bool EnablerParticipant::fill_topic_struct_nts_(
        const std::string& topic_name,
        const TopicInfo& topic_info,
        DdsTopic& topic)

{
    // Deserialize QoS if provided by the user (otherwise use default one)
    TopicQoS qos;
    if (!topic_info.serialized_qos.empty())
    {
        try
        {
            qos = serialization::deserialize_qos(topic_info.serialized_qos);
        }
        catch (const std::exception& e)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to deserialize QoS for topic " << topic_name << ": " << e.what());
            return false;
        }
    }

    fastdds::dds::xtypes::TypeIdentifier type_identifier;
    if (!handler_->get_type_identifier(topic_info.type_name, type_identifier))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to create topic " << topic_name << " : type identifier not found.");
        return false;
    }

    topic.m_topic_name = topic_name;
    topic.type_name = topic_info.type_name;
    topic.topic_qos = qos;
    topic.type_identifiers.type_identifier1(type_identifier);

    return true;
}

bool EnablerParticipant::fill_service_type_nts_(
        const ServiceInfo& service_info,
        std::shared_ptr<ServiceDiscovered> service,
        RpcProtocol RpcProtocol)
{
    std::string rq_prefix, rq_suffix, rp_prefix, rp_suffix;
    switch (RpcProtocol)
    {
        case RpcProtocol::ROS2:
            rq_prefix = ROS_REQUEST_PREFIX;
            rq_suffix = ROS_REQUEST_SUFFIX;
            rp_prefix = ROS_REPLY_PREFIX;
            rp_suffix = ROS_REPLY_SUFFIX;
            break;
        case RpcProtocol::FASTDDS:
            rq_prefix = FASTDDS_REQUEST_PREFIX;
            rq_suffix = FASTDDS_REQUEST_SUFFIX;
            rp_prefix = FASTDDS_REPLY_PREFIX;
            rp_suffix = FASTDDS_REPLY_SUFFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to announce service " << service->service_name << ": unknown RPC protocol.");
            return false;
    }

    DdsTopic topic_request;
    std::string topic_request_name = rq_prefix + service->service_name + rq_suffix;
    if (!fill_topic_struct_nts_(topic_request_name, service_info.request, topic_request))
    {
        return false;
    }
    service->add_topic(topic_request, ServiceType::REQUEST);

    DdsTopic topic_reply;
    std::string topic_reply_name = rp_prefix + service->service_name + rp_suffix;
    if (!fill_topic_struct_nts_(topic_reply_name, service_info.reply, topic_reply))
    {
        return false;
    }
    service->add_topic(topic_reply, ServiceType::REPLY);

    if (!service->fully_discovered)
    {
        return false;
    }

    service->enabler_as_server = true;
    return true;
}

std::shared_ptr<IReader> EnablerParticipant::lookup_reader_nts_(
        const std::string& topic_name,
        std::string& type_name) const
{
    for (const auto& reader : readers_)
    {
        if (reader.first.m_topic_name == topic_name)
        {
            type_name = reader.first.type_name;
            return reader.second;
        }
    }
    return nullptr;
}

std::shared_ptr<IReader> EnablerParticipant::lookup_reader_nts_(
        const std::string& topic_name) const
{
    std::string _;
    return lookup_reader_nts_(topic_name, _);
}

bool EnablerParticipant::service_discovered_nts_(
        const std::shared_ptr<RpcInfo> rpc_info,
        const DdsTopic& topic)
{
    auto [it, inserted] = services_.try_emplace(rpc_info->service_name,
                    std::make_shared<ServiceDiscovered>(rpc_info->service_name, rpc_info->rpc_protocol));
    if (it->second->add_topic(topic, rpc_info->service_type))
    {
        it->second->external_server = true;
        return true;
    }
    return false;
}

bool EnablerParticipant::action_discovered_nts_(
        const std::shared_ptr<RpcInfo> rpc_info,
        const DdsTopic& topic)
{
    auto [it, inserted] = actions_.try_emplace(rpc_info->action_name,
                    std::make_shared<ActionDiscovered>(rpc_info->action_name, rpc_info->rpc_protocol));
    if (ServiceType::NONE != rpc_info->service_type)
    {
        if (service_discovered_nts_(rpc_info, topic))
        {
            auto service_it = services_.find(rpc_info->service_name);
            if (services_.end() == service_it)
            {
                EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                        "Service " << rpc_info->service_name << " not found in action " << rpc_info->action_name);
                return false;
            }

            it->second->add_service(service_it->second, rpc_info->action_type);
        }
    }
    else
    {
        it->second->add_topic(topic, rpc_info->action_type);
    }

    if (it->second->check_fully_discovered())
    {
        it->second->external_server = true;
        return true;
    }
    return false;
}

bool EnablerParticipant::revoke_service_nts_(
        const std::string& service_name)
{
    auto it = services_.find(service_name);
    if (it == services_.end())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to revoke service " << service_name << " : service not found.");
        return false;
    }
    if (!it->second->enabler_as_server || !it->second->endpoint_request.has_value())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to revoke service " << service_name << " : service not announced as server.");
        return false;
    }

    if (it->second->external_server)
    {
        it->second->enabler_as_server = false;
        return true;
    }

    std::string request_name = it->second->topic_request.m_topic_name;

    this->discovery_database_->erase_endpoint(it->second->endpoint_request.value());
    it->second->endpoint_request.reset();
    it->second->remove_topic(ServiceType::REQUEST);

    auto reader = lookup_reader_nts_(request_name);
    if (nullptr != reader)
    {
        readers_.erase(reader->topic());
    }

    return true;
}

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
