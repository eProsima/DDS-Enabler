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
{
}

bool EnablerParticipant::service_discovered_nts_(
        const std::string& service_name,
        const DdsTopic& topic,
        RpcUtils::RpcType rpc_type)
{
    auto [it, inserted] = services_.try_emplace(service_name, std::make_shared<ServiceDiscovered>(service_name));
    return it->second->add_topic(topic, rpc_type);
}

bool EnablerParticipant::action_discovered_nts_(
    const std::string& action_name,
    const DdsTopic& topic,
    RpcUtils::RpcType rpc_type)
{
    auto [it, inserted] = actions_.try_emplace(action_name, ActionDiscovered(action_name));
    std::string service_name;
    RpcUtils::RpcType service_direction = RpcUtils::get_service_name(topic.m_topic_name, service_name);
    if (RpcUtils::RpcType::RPC_NONE != service_direction)
    {
        service_discovered_nts_(service_name, topic, service_direction);
        auto service_it = services_.find(service_name);
        if (services_.end() == service_it)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Service " << service_name << " not found in action " << action_name);
            return false;
        }

        it->second.add_service(service_it->second, rpc_type);
    }
    else
    {
        it->second.add_topic(topic, rpc_type);
    }

    return it->second.check_fully_discovered();
}

std::shared_ptr<IReader> EnablerParticipant::create_reader(
        const ITopic& topic)
{
    if (is_type_object_topic(topic))
    {
        return std::make_shared<BlankReader>();
    }

    std::shared_ptr<IReader> reader;
    {
        std::lock_guard<std::mutex> lck(mtx_);
        auto dds_topic = dynamic_cast<const DdsTopic&>(topic);
        std::string rpc_name;
        RpcUtils::RpcType rpc_type = RpcUtils::get_rpc_name(dds_topic.m_topic_name, rpc_name);
        if (RpcUtils::RpcType::RPC_NONE != rpc_type)
        {
            RpcUtils::RpcType rpc_direction = RpcUtils::get_service_direction(rpc_type);
            if (RpcUtils::RpcType::RPC_NONE != rpc_direction)
                reader = std::make_shared<InternalRpcReader>(id(), dds_topic);
            else
                reader = std::make_shared<InternalReader>(id());

            // Only notify the discovery of topics that do not originate from a topic query callback
            if (dds_topic.topic_discoverer() != this->id())
            {
                if (RpcUtils::ActionType::NONE == RpcUtils::get_action_type(rpc_type))
                {
                    if (service_discovered_nts_(rpc_name, dds_topic, rpc_type))
                    {
                        RpcTopic service = services_.find(rpc_name)->second->get_service();
                        std::static_pointer_cast<Handler>(schema_handler_)->add_service(service);
                    }
                }
                else
                {
                    if (action_discovered_nts_(rpc_name, dds_topic, rpc_type))
                    {
                        auto action = actions_.find(rpc_name)->second.get_action(rpc_name);
                        std::static_pointer_cast<Handler>(schema_handler_)->add_action(action);
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
                std::static_pointer_cast<Handler>(schema_handler_)->add_topic(dds_topic);
            }
        }
        readers_[dds_topic] = reader;
    }
    cv_.notify_all();
    return reader;
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
        if(!query_topic_nts_(topic_name, topic))
        {
            return false;
        }

        ddspipe::core::types::Endpoint _;
        create_topic_writer_nts_(topic, i_reader, _, lck);
        type_name = topic.type_name;

        // (Optionally) wait for writer created in DDS participant to match with external readers, to avoid losing this
        // message when not using transient durability
        std::this_thread::sleep_for(std::chrono::milliseconds(std::static_pointer_cast<EnablerParticipantConfiguration>(
                    configuration_)->initial_publish_wait));
    }

    auto reader = std::dynamic_pointer_cast<InternalReader>(i_reader);

    auto data = std::make_unique<RtpsPayloadData>();

    Payload payload;
    if (!std::static_pointer_cast<Handler>(schema_handler_)->get_serialized_data(type_name, json, payload))
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
    const uint64_t& request_id)
{
    std::unique_lock<std::mutex> lck(mtx_);

    std::string service_name;
    RpcUtils::RpcType rpc_type = RpcUtils::get_service_name(topic_name, service_name);

    auto it = services_.find(service_name);
    if (it == services_.end())
    {
        // There is no case where none of the service topics are discovered and yet the publish should be done
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in service " << service_name << " : service does not exist.");
        return false;
    }

    std::string type_name;
    auto reader = std::dynamic_pointer_cast<InternalRpcReader>(lookup_reader_nts_(topic_name, type_name));

    if (nullptr == reader)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
            "Failed to publish data in service " << service_name << " : service does not exist.");
        return false;
    }

    DdsTopic topic;
    if (!it->second->get_topic(rpc_type, topic))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in service " << service_name << " : topic not found.");
        return false;
    }

    if(type_name != topic.type_name)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic.m_topic_name << " : type name mismatch.");
        return false;
    }

    auto data = std::make_unique<RpcPayloadData>();

    Payload payload;
    if (!std::static_pointer_cast<Handler>(schema_handler_)->get_serialized_data(type_name, json, payload))
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

bool EnablerParticipant::create_service_request_writer_nts_(
        std::shared_ptr<ServiceDiscovered> service,
        std::unique_lock<std::mutex>& lck)
{
    std::string _;
    auto reader = lookup_reader_nts_(service->topic_request.m_topic_name, _);

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
            "Failed to create server as it is already running in ROS2.");
    return false;
}

bool EnablerParticipant::announce_service(
    const std::string& service_name)
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
        services_.erase(it);
    }

    std::shared_ptr<ServiceDiscovered> service = std::make_shared<ServiceDiscovered>(service_name);
    if(!query_service_nts_(service))
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

bool EnablerParticipant::revoke_service_nts_(
    const std::string& service_name)
{
    auto it = services_.find(service_name);
    if (it == services_.end())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to stop service " << service_name << " : service not found.");
        return false;
    }
    if (!it->second->enabler_as_server || !it->second->endpoint_request.has_value())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to stop service " << service_name << " : service not announced as server.");
        return false;
    }

    this->discovery_database_->erase_endpoint(it->second->endpoint_request.value());
    it->second->endpoint_request.reset();
    it->second->remove_topic(RpcUtils::RpcType::RPC_REQUEST);

    std::string request_name = std::string(REQUEST_PREFIX) + service_name + REQUEST_SUFFIX;

    auto reader = lookup_reader_nts_(request_name);
    if (nullptr != reader)
    {
        readers_.erase(reader->topic());
    }

    return true;
}

bool EnablerParticipant::announce_action(
    const std::string& action_name)
{
    std::unique_lock<std::mutex> lck(mtx_);

    {
        auto it = actions_.find(action_name);
        if (it != actions_.end())
        {
            if (it->second.enabler_as_server)
            {
                EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                        "Failed to announce action " << action_name << " : action already announced.");
                return false;
            }
            // Erase the action, to allow re-announcing it
            actions_.erase(it);
        }
    }

    ActionDiscovered action(action_name);
    if(!query_action_nts_(action, lck))
    {
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
                "Failed to stop action " << action_name << " : action not found.");
        return false;
    }
    if (!it->second.enabler_as_server)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to stop action " << action_name << " : action not announced as server.");
        return false;
    }

    auto& action = it->second;

    auto goal = action.goal.lock();
    auto result = action.result.lock();
    auto cancel = action.cancel.lock();
    if (!goal || !result || !cancel)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to stop action " << action_name << " : action services not fully discovered.");
        return false;
    }
    if (this->revoke_service_nts_(goal->service_name) &&
            this->revoke_service_nts_(result->service_name) &&
            this->revoke_service_nts_(cancel->service_name))
    {
        action.enabler_as_server = false;
        action.fully_discovered = false;
        return true;
    }

    return false;
}

bool EnablerParticipant::query_topic_nts_(
    const std::string& topic_name,
    DdsTopic& topic)
{
    if (!topic_query_callback_)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic_name <<
                            " : topic is unknown and topic request callback not set.");
        return false;
    }
    std::string type_name;
    std::string serialized_qos;
    if (!topic_query_callback_(topic_name.c_str(), type_name, serialized_qos))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to publish data in topic " << topic_name << " : topic query callback failed.");
        return false;
    }

    return fill_topic_struct_nts_(topic_name, type_name, serialized_qos, topic);
}

bool EnablerParticipant::fill_topic_struct_nts_(
    const std::string& topic_name,
    const std::string type_name,
    const std::string serialized_qos,
    DdsTopic& topic)

{
    // Deserialize QoS if provided by the user (otherwise use default one)
    TopicQoS qos;
    if (!serialized_qos.empty())
    {
        try
        {
            qos = serialization::deserialize_qos(serialized_qos);
        }
        catch (const std::exception& e)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to deserialize QoS for topic " << topic_name << ": " << e.what());
            return false;
        }
    }

    fastdds::dds::xtypes::TypeIdentifier type_identifier;
    if (!std::static_pointer_cast<Handler>(schema_handler_)->get_type_identifier(type_name, type_identifier))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to create topic " << topic_name << " : type identifier not found.");
        return false;
    }

    topic.m_topic_name = topic_name;
    topic.type_name = type_name;
    topic.topic_qos = qos;
    topic.type_identifiers.type_identifier1(type_identifier);

    return true;
}

bool EnablerParticipant::fullfill_service_type_nts_(
    const std::string _request_type_name,
    const std::string serialized_request_qos_content,
    const std::string _reply_type_name,
    const std::string serialized_reply_qos_content,
    std::shared_ptr<ServiceDiscovered> service)
{

    DdsTopic topic_request;
    std::string topic_request_name = std::string(REQUEST_PREFIX) + service->service_name + REQUEST_SUFFIX;
    if(!fill_topic_struct_nts_(topic_request_name, _request_type_name, serialized_request_qos_content, topic_request))
        return false;
    service->add_topic(topic_request, RpcUtils::RPC_REQUEST);

    DdsTopic topic_reply;
    std::string topic_reply_name = std::string(REPLY_PREFIX) + service->service_name + REPLY_SUFFIX;
    if(!fill_topic_struct_nts_(topic_reply_name, _reply_type_name, serialized_reply_qos_content, topic_reply))
        return false;
    service->add_topic(topic_reply, RpcUtils::RPC_REPLY);

    if (!service->fully_discovered)
        return false;

    service->enabler_as_server = true;
    return true;
}

bool EnablerParticipant::query_service_nts_(
        std::shared_ptr<ServiceDiscovered> service)
{
    if(!service_query_callback_)
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


    if (!service_query_callback_(service->service_name.c_str(), request_type_name, serialized_request_qos_content,
            reply_type_name, serialized_reply_qos_content))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce service " << service->service_name << " : service type request failed.");
        return false;
    }

    return fullfill_service_type_nts_(request_type_name, serialized_request_qos_content,
            reply_type_name, serialized_reply_qos_content, service);
}

bool EnablerParticipant::query_action_nts_(
    ActionDiscovered& action,
    std::unique_lock<std::mutex>& lck)
{
    if(!action_query_callback_)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name <<
                            " : action is unknown and action request callback not set.");
        return false;
    }

    std::string goal_request_action_type;
    std::string goal_reply_action_type;
    std::string cancel_request_action_type;
    std::string cancel_reply_action_type;
    std::string result_request_action_type;
    std::string result_reply_action_type;
    std::string feedback_action_type;
    std::string status_action_type;
    std::string goal_request_action_serialized_qos;
    std::string goal_reply_action_serialized_qos;
    std::string cancel_request_action_serialized_qos;
    std::string cancel_reply_action_serialized_qos;
    std::string result_request_action_serialized_qos;
    std::string result_reply_action_serialized_qos;
    std::string feedback_action_serialized_qos;
    std::string status_action_serialized_qos;

    if (!action_query_callback_(action.action_name.c_str(),
            goal_request_action_type,
            goal_reply_action_type,
            cancel_request_action_type,
            cancel_reply_action_type,
            result_request_action_type,
            result_reply_action_type,
            feedback_action_type,
            status_action_type,
            goal_request_action_serialized_qos,
            goal_reply_action_serialized_qos,
            cancel_request_action_serialized_qos,
            cancel_reply_action_serialized_qos,
            result_request_action_serialized_qos,
            result_reply_action_serialized_qos,
            feedback_action_serialized_qos,
            status_action_serialized_qos))
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

    std::shared_ptr<ServiceDiscovered> goal_service = std::make_shared<ServiceDiscovered>(goal_service_name);
    if(!fullfill_service_type_nts_(
            goal_request_action_type,
            goal_request_action_serialized_qos,
            goal_reply_action_type,
            goal_reply_action_serialized_qos,
            goal_service))
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

    std::shared_ptr<ServiceDiscovered> cancel_service = std::make_shared<ServiceDiscovered>(cancel_service_name);
    if(!fullfill_service_type_nts_(
            cancel_request_action_type,
            cancel_request_action_serialized_qos,
            cancel_reply_action_type,
            cancel_reply_action_serialized_qos,
            cancel_service))
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

    std::shared_ptr<ServiceDiscovered> result_service = std::make_shared<ServiceDiscovered>(result_service_name);
    if(!fullfill_service_type_nts_(
            result_request_action_type,
            result_request_action_serialized_qos,
            result_reply_action_type,
            result_reply_action_serialized_qos,
            result_service))
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

    std::string feedback_topic_name = participants::TOPIC_PREFIX + action.action_name + participants::ACTION_FEEDBACK_SUFFIX;
    DdsTopic feedback_topic;
    if(!fill_topic_struct_nts_(
            feedback_topic_name,
            feedback_action_type,
            feedback_action_serialized_qos,
            feedback_topic))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : feedback topic type not found.");
        return false;
    }
    {
        std::string _;
        auto feedback_reader = lookup_reader_nts_(feedback_topic_name, _);
        if (!feedback_reader)
        {
            ddspipe::core::types::Endpoint _;
            create_topic_writer_nts_(
                    action.feedback,
                    feedback_reader,
                    _,
                    lck);
        }
    }
    action.feedback = feedback_topic;
    action.feedback_discovered = true;

    std::string status_topic_name = participants::TOPIC_PREFIX + action.action_name + participants::ACTION_STATUS_SUFFIX;
    DdsTopic status_topic;
    if(!fill_topic_struct_nts_(
            status_topic_name,
            status_action_type,
            status_action_serialized_qos,
            status_topic))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                "Failed to announce action " << action.action_name << " : status topic type not found.");
        return false;
    }
    {
        std::string _;
        auto status_reader = lookup_reader_nts_(action.status.m_topic_name, _);
        if (!status_reader)
        {
            ddspipe::core::types::Endpoint _;
            create_topic_writer_nts_(
                    action.status,
                    status_reader,
                    _,
                    lck);
        }
    }
    action.status = status_topic;
    action.status_discovered = true;

    action.fully_discovered = true;
    action.enabler_as_server = true;
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

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
