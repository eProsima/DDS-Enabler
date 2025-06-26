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
        reader = std::make_shared<InternalReader>(id());
        auto dds_topic = dynamic_cast<const DdsTopic&>(topic);
        readers_[dds_topic] = reader;
        // Only notify the discovery of topics that do not originate from a topic query callback
        if (dds_topic.topic_discoverer() != this->id())
        {
            std::static_pointer_cast<Handler>(schema_handler_)->add_topic(dds_topic);
        }
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
    auto reader = lookup_reader_nts_(topic_name, type_name);

    if (nullptr == reader)
    {
        if (!topic_query_callback_)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to publish data in topic " << topic_name <<
                    " : topic is unknown and topic query callback not set.");
            return false;
        }

        std::string serialized_qos;
        if (!topic_query_callback_(topic_name.c_str(), type_name, serialized_qos))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to publish data in topic " << topic_name << " : topic query callback failed.");
            return false;
        }

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
                    "Failed to publish data in topic " << topic_name << " : type identifier not found.");
            return false;
        }

        DdsTopic topic;
        topic.m_topic_name = topic_name;
        topic.type_name = type_name;
        topic.topic_qos = qos;
        topic.type_identifiers.type_identifier1(type_identifier);
        this->discovery_database_->add_endpoint(rtps::CommonParticipant::simulate_endpoint(topic, this->id()));

        // Wait for reader to be created from discovery thread
        // NOTE: Set a timeout to avoid a deadlock in case the reader is never created for some reason (e.g. the topic
        // is blocked or the underlying DDS Pipe object is disabled/destroyed before the reader is created).
        if (!cv_.wait_for(lck, std::chrono::seconds(5), [&]
                {
                    return nullptr != (reader = lookup_reader_nts_(topic_name));
                }))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_ENABLER_PARTICIPANT,
                    "Failed to create internal reader for topic " << topic_name <<
                    " , please verify that the topic is allowed.");
            return false;
        }

        // (Optionally) wait for writer created in DDS participant to match with external readers, to avoid losing this
        // message when not using transient durability
        std::this_thread::sleep_for(std::chrono::milliseconds(std::static_pointer_cast<EnablerParticipantConfiguration>(
                    configuration_)->initial_publish_wait));
    }

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

std::shared_ptr<ddspipe::participants::InternalReader> EnablerParticipant::lookup_reader_nts_(
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

std::shared_ptr<ddspipe::participants::InternalReader> EnablerParticipant::lookup_reader_nts_(
        const std::string& topic_name) const
{
    std::string _;
    return lookup_reader_nts_(topic_name, _);
}

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
