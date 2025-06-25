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
 * @file Handler.cpp
 */

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/xtypes/utils.hpp>

#include <cpp_utils/exception/InconsistencyException.hpp>

#include <ddsenabler_participants/Serialization.hpp>
#include <ddsenabler_participants/types/dynamic_types_collection/DynamicTypesCollection.hpp>

#include <ddsenabler_participants/Handler.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

using namespace eprosima::ddspipe::core::types;

Handler::Handler(
        const HandlerConfiguration& config,
        const std::shared_ptr<ddspipe::core::PayloadPool>& payload_pool)
    : configuration_(config)
    , payload_pool_(payload_pool)
{
    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Creating handler instance.");

    writer_ = std::make_unique<Writer>();
}

Handler::~Handler()
{
    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Destroying handler.");
}

void Handler::add_schema(
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const fastdds::dds::xtypes::TypeIdentifier& type_id)
{
    std::lock_guard<std::mutex> lock(mtx_);

    add_schema_nts_(dyn_type, type_id);
}

void Handler::add_topic(
        const DdsTopic& topic)
{
    std::lock_guard<std::mutex> lock(mtx_);

    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Adding topic: " << topic << ".");

    write_topic_nts_(topic);
}

void Handler::add_data(
        const DdsTopic& topic,
        RtpsPayloadData& data)
{
    std::lock_guard<std::mutex> lock(mtx_);

    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Adding data in topic: " << topic << ".");

    fastdds::dds::DynamicType::_ref_type dyn_type;
    auto it = schemas_.find(topic.type_name);
    if (it == schemas_.end())
    {
        EPROSIMA_LOG_WARNING(DDSENABLER_HANDLER,
                "Schema for type " << topic.type_name << " not available.");
        return;
    }
    dyn_type = it->second.second;

    Message msg;
    msg.sequence_number = unique_sequence_number_++;
    msg.publish_time = data.source_timestamp;
    if (data.payload.length > 0)
    {
        msg.topic = topic;
        msg.instanceHandle = data.instanceHandle;
        msg.source_guid = data.source_guid;

        if (data.payload_owner != nullptr)
        {
            payload_pool_->get_payload(
                data.payload,
                msg.payload);

            msg.payload_owner = payload_pool_.get();
        }
        else
        {
            throw utils::InconsistencyException(STR_ENTRY << "Payload owner not found in data received.");
        }
    }
    else
    {
        throw utils::InconsistencyException(STR_ENTRY << "Received sample with no payload.");
    }

    write_sample_nts_(msg, dyn_type);
}

bool Handler::get_type_identifier(
        const std::string& type_name,
        fastdds::dds::xtypes::TypeIdentifier& type_identifier)
{
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = schemas_.find(type_name);
    if (it != schemas_.end())
    {
        type_identifier = it->second.first;
        return true;
    }

    // Try to retrieve it from local registry
    fastdds::dds::xtypes::TypeIdentifierPair type_ids;
    if (fastdds::dds::RETCODE_OK ==
            fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_identifiers(
                type_name, type_ids))
    {
        // Get complete type object
        type_identifier =
                (fastdds::dds::xtypes::EK_COMPLETE ==
                type_ids.type_identifier1()._d()) ? type_ids.type_identifier1() : type_ids.type_identifier2();
        fastdds::dds::xtypes::TypeObject type_object;
        if (fastdds::dds::RETCODE_OK ==
                fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                    type_identifier, type_object))
        {
            // If already in the registry, just add it to schemas map. Also report to the user the schema and all
            // associated data required for persistence in case she does not have it yet.
            if (add_schema_nts_(type_identifier, type_object, true))
            {
                return true;
            }
            // If failed to add schema from the type object found in the registry, attempt requesting it to the user
        }
    }

    if (!type_query_callback_)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Type query callback not set.");
        return false;
    }

    std::unique_ptr<const unsigned char []> serialized_type;
    uint32_t serialized_type_size;
    if (!type_query_callback_(type_name.c_str(), serialized_type, serialized_type_size))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Type query callback failed to retrieve " << type_name << " type.");
        return false;
    }

    // Register the type obtained through the type query callback
    fastdds::dds::xtypes::TypeObject type_object;
    if (!register_type_nts_(type_name, serialized_type.get(), serialized_type_size, type_identifier, type_object))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to register type " << type_name << ".");
        return false;
    }

    // Add to schemas map, but do not report it to the user as it is the exact same information we obtained through the
    // type query callback.
    add_schema_nts_(type_identifier, type_object, false);

    return true;
}

bool Handler::get_serialized_data(
        const std::string& type_name,
        const std::string& json,
        Payload& payload)
{
    std::lock_guard<std::mutex> lock(mtx_);

    fastdds::dds::DynamicType::_ref_type dyn_type;
    auto it = schemas_.find(type_name);
    if (it == schemas_.end())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to deserialize data for type " << type_name << " : schema not available.");
        return false;
    }
    dyn_type = it->second.second;

    fastdds::dds::DynamicData::_ref_type dyn_data;
    if ((fastdds::dds::RETCODE_OK !=
            fastdds::dds::json_deserialize(json, dyn_type, fastdds::dds::DynamicDataJsonFormat::EPROSIMA,
            dyn_data)) || !dyn_data)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to deserialize data for type " << type_name << " : json deserialization failed.");
        return false;
    }

    // Use XCDR1 for backwards compatibility (e.g. ROS 2 distributions prior to Kilted)
    fastdds::dds::DynamicPubSubType pubsub_type (dyn_type);
    uint32_t payload_size = pubsub_type.calculate_serialized_size(&dyn_data,
                    fastdds::dds::DataRepresentationId::XCDR_DATA_REPRESENTATION);

    if (!payload_pool_->get_payload(payload_size, payload))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to deserialize data for type " << type_name << " : get_payload failed.");
        return false;
    }

    if (!pubsub_type.serialize(&dyn_data, payload, fastdds::dds::DataRepresentationId::XCDR_DATA_REPRESENTATION))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to deserialize data for type " << type_name << " : payload serialization failed.");
        return false;
    }

    return true;
}

void Handler::add_schema_nts_(
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const fastdds::dds::xtypes::TypeIdentifier& type_id,
        bool write_schema)
{
    assert(nullptr != dyn_type);

    const std::string& type_name = dyn_type->get_name().to_string();

    // Check if it exists already
    auto it = schemas_.find(type_name);
    if (it != schemas_.end())
    {
        return;
    }
    schemas_[type_name] = {type_id, dyn_type};

    // Add to schemas map
    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Adding schema with name " << type_name << ".");

    if (write_schema)
    {
        write_schema_nts_(dyn_type, type_id);
    }
}

bool Handler::add_schema_nts_(
        const fastdds::dds::xtypes::TypeIdentifier& type_id,
        const fastdds::dds::xtypes::TypeObject& type_obj,
        bool write_schema)
{
    // Create a DynamicType from TypeObject
    fastdds::dds::DynamicType::_ref_type dyn_type =
            fastdds::dds::DynamicTypeBuilderFactory::get_instance()->create_type_w_type_object(type_obj)
                    ->build();
    if (!dyn_type)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to create Dynamic Type from TypeObject.");
        return false;
    }

    add_schema_nts_(dyn_type, type_id, write_schema);
    return true;
}

void Handler::write_schema_nts_(
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const fastdds::dds::xtypes::TypeIdentifier& type_id)
{
    writer_->write_schema(dyn_type, type_id);
}

void Handler::write_topic_nts_(
        const DdsTopic& topic)
{
    writer_->write_topic(topic);
}

void Handler::write_sample_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type)
{
    writer_->write_data(msg, dyn_type);
}

bool Handler::register_type_nts_(
        const std::string& type_name,
        const unsigned char* serialized_type,
        uint32_t serialized_type_size,
        fastdds::dds::xtypes::TypeIdentifier& type_identifier,
        fastdds::dds::xtypes::TypeObject& type_object)
{
    DynamicTypesCollection dynamic_types;
    if (!serialization::deserialize_dynamic_types(serialized_type, serialized_type_size, dynamic_types))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Failed to deserialize dynamic types collection.");
        return false;
    }

    std::string _type_name;
    fastdds::dds::xtypes::TypeIdentifier _type_identifier;
    fastdds::dds::xtypes::TypeObject _type_object;

    // Deserialize and register all dependencies and main type (last one in collection)
    for (DynamicType& dynamic_type : dynamic_types.dynamic_types())
    {
        if (!serialization::deserialize_dynamic_type(dynamic_type, _type_name, _type_identifier, _type_object))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Failed to deserialize " << dynamic_type.type_name() << " DynamicType.");
            return false;
        }

        // Create a TypeIdentifierPair to use in register_type_identifier
        fastdds::dds::xtypes::TypeIdentifierPair type_identifiers;
        type_identifiers.type_identifier1(_type_identifier);

        // Register in factory
        if (fastdds::dds::RETCODE_OK !=
                fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().register_type_object(
                    _type_object, type_identifiers))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Failed to register " << dynamic_type.type_name() << " DynamicType.");
            return false;
        }
    }

    if (_type_name != type_name)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                "Unexpected dynamic types collection format: " << type_name << " expected to be last item, found " << _type_name <<
                " instead.");
        return false;
    }

    // Assign type identifier and object after all types have been registered
    type_identifier = _type_identifier;
    type_object = _type_object;

    return true;
}

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
