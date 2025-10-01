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
 * @file EnablerParticipant.hpp
 */

#pragma once

#include <condition_variable>
#include <map>
#include <mutex>

#include <ddspipe_participants/participant/dynamic_types/SchemaParticipant.hpp>

#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/EnablerParticipantConfiguration.hpp>
#include <ddsenabler_participants/library/library_dll.h>
#include <ddsenabler_participants/InternalRpcReader.hpp>
#include <ddsenabler_participants/RpcUtils.hpp>
#include <ddsenabler_participants/RpcStructs.hpp>


namespace eprosima {
namespace ddsenabler {
namespace participants {

class EnablerParticipant : public ddspipe::participants::SchemaParticipant
{
public:

    DDSENABLER_PARTICIPANTS_DllAPI
    EnablerParticipant(
            std::shared_ptr<EnablerParticipantConfiguration> participant_configuration,
            std::shared_ptr<ddspipe::core::PayloadPool> payload_pool,
            std::shared_ptr<ddspipe::core::DiscoveryDatabase> discovery_database,
            std::shared_ptr<ddspipe::participants::ISchemaHandler> schema_handler);

    DDSENABLER_PARTICIPANTS_DllAPI
    bool is_rtps_kind() const noexcept override
    {
        return true; // Temporal workaround until Pipe refactor
    }

    DDSENABLER_PARTICIPANTS_DllAPI
    std::shared_ptr<ddspipe::core::IReader> create_reader(
            const ddspipe::core::ITopic& topic) override;

    DDSENABLER_PARTICIPANTS_DllAPI
    bool publish(
            const std::string& topic_name,
            const std::string& json);

    DDSENABLER_PARTICIPANTS_DllAPI
    bool publish_rpc(
            const std::string& topic_name,
            const std::string& json,
            const uint64_t& request_id);

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_topic_query_callback(
            participants::DdsTopicQuery callback)
    {
        topic_query_callback_ = callback;
    }

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_service_query_callback(
            participants::ServiceQuery callback)
    {
        service_query_callback_ = callback;
    }

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_query_callback(
            participants::ActionQuery callback)
    {
        action_query_callback_ = callback;
    }

    DDSENABLER_PARTICIPANTS_DllAPI
    bool announce_service(
            const std::string& service_name,
            RPC_PROTOCOL rpc_protocol = RPC_PROTOCOL::ROS2);

    DDSENABLER_PARTICIPANTS_DllAPI
    bool revoke_service(
            const std::string& service_name);

    DDSENABLER_PARTICIPANTS_DllAPI
    RPC_PROTOCOL get_service_rpc_protocol(
            const std::string& service_name);

    DDSENABLER_PARTICIPANTS_DllAPI
    bool announce_action(
            const std::string& action_name,
            RPC_PROTOCOL rpc_protocol = RPC_PROTOCOL::ROS2);

    DDSENABLER_PARTICIPANTS_DllAPI
    bool revoke_action(
            const std::string& action_name);

protected:

    bool query_topic_nts_(
            const std::string& topic_name,
            ddspipe::core::types::DdsTopic& topic);

    bool query_service_nts_(
            std::shared_ptr<ServiceDiscovered> service,
            RPC_PROTOCOL rpc_protocol);

    bool query_action_nts_(
            ActionDiscovered& action,
            RPC_PROTOCOL rpc_protocol,
            std::unique_lock<std::mutex>& lck);

    bool create_topic_writer_nts_(
            const ddspipe::core::types::DdsTopic& topic,
            std::shared_ptr<eprosima::ddspipe::core::IReader>& reader,
            ddspipe::core::types::Endpoint& request_edp,
            std::unique_lock<std::mutex>& lck);

    bool create_service_request_writer_nts_(
            std::shared_ptr<ServiceDiscovered> service,
            std::unique_lock<std::mutex>& lck);

    bool fill_topic_struct_nts_(
            const std::string& topic_name,
            const TopicInfo& topic_info,
            ddspipe::core::types::DdsTopic& topic);

    bool fullfill_service_type_nts_(
            const ServiceInfo& service_info,
            std::shared_ptr<ServiceDiscovered> service,
            RPC_PROTOCOL rpc_protocol);

    std::shared_ptr<ddspipe::core::IReader> lookup_reader_nts_(
            const std::string& topic_name,
            std::string& type_name) const;

    std::shared_ptr<ddspipe::core::IReader> lookup_reader_nts_(
            const std::string& topic_name) const;

    bool service_discovered_nts_(
            const RpcInfo& rpc_info,
            const ddspipe::core::types::DdsTopic& topic);

    bool action_discovered_nts_(
            const RpcInfo& rpc_info,
            const ddspipe::core::types::DdsTopic& topic);

    bool revoke_service_nts_(
            const std::string& service_name);

    std::map<ddspipe::core::types::DdsTopic, std::shared_ptr<ddspipe::core::IReader>> readers_;

    std::map<std::string, std::shared_ptr<ServiceDiscovered>> services_;

    std::map<std::string, ActionDiscovered> actions_;

    std::mutex mtx_;

    std::condition_variable cv_;

    DdsTopicQuery topic_query_callback_;

    ServiceQuery service_query_callback_;

    ActionQuery action_query_callback_;
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
