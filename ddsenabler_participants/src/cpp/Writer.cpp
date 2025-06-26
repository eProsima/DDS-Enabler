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
 * @file Writer.cpp
 */

#include <nlohmann/json.hpp>

#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/utils.hpp>
#include <fastdds/rtps/common/SerializedPayload.hpp>
#include <fastdds/rtps/common/Types.hpp>

#include <ddsenabler_participants/Serialization.hpp>
#include <ddsenabler_participants/types/dynamic_types_collection/DynamicTypesCollection.hpp>

#include <ddsenabler_participants/Writer.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

using namespace eprosima::ddsenabler::participants::serialization;
using namespace eprosima::ddspipe::core::types;

void Writer::write_schema(
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const fastdds::dds::xtypes::TypeIdentifier& type_id)
{
    assert(nullptr != dyn_type);

    const std::string& type_name = dyn_type->get_name().to_string();

    // Schema has not been registered
    EPROSIMA_LOG_INFO(DDSENABLER_WRITER,
            "Writing schema: " << type_name << ".");

    std::stringstream ss_idl;
    auto ret = fastdds::dds::idl_serialize(dyn_type, ss_idl);
    if (ret != fastdds::dds::RETCODE_OK)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Failed to serialize DynamicType to idl for type with name: " << type_name);
        return;
    }

    DynamicTypesCollection types_collection;
    if (!serialize_dynamic_type(type_name, type_id, types_collection))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Failed to serialize dynamic types collection: " << type_name);
        return;
    }

    std::unique_ptr<fastdds::rtps::SerializedPayload_t> types_collection_payload = serialize_dynamic_types(
        types_collection);
    if (nullptr == types_collection_payload)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Failed to serialize dynamic types collection: " << type_name);
        return;
    }

    std::stringstream ss_data_holder;
    ss_data_holder << std::setw(4);
    if (fastdds::dds::RETCODE_OK !=
            fastdds::dds::json_serialize(fastdds::dds::DynamicDataFactory::get_instance()->create_data(dyn_type),
            fastdds::dds::DynamicDataJsonFormat::EPROSIMA, ss_data_holder))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Not able to generate data placeholder for type " << type_name << ".");
        return;
    }

    // Notify type reception
    if (type_notification_callback_)
    {
        type_notification_callback_(
            type_name.c_str(),
            ss_idl.str().c_str(),
            types_collection_payload->data,
            types_collection_payload->length,
            ss_data_holder.str().c_str()
            );
    }
}

void Writer::write_topic(
        const DdsTopic& topic)
{
    EPROSIMA_LOG_INFO(DDSENABLER_WRITER,
            "Writting topic: " << topic.topic_name() << ".");

    // Notify topic reception
    if (topic_notification_callback_)
    {
        std::string serialized_qos = serialize_qos(topic.topic_qos);
        topic_notification_callback_(
            topic.topic_name().c_str(),
            topic.type_name.c_str(),
            serialized_qos.c_str()
            );
    }
}

void Writer::write_data(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type)
{
    assert(nullptr != dyn_type);

    EPROSIMA_LOG_INFO(DDSENABLER_WRITER,
            "Writing message from topic: " << msg.topic.topic_name() << ".");

    // Get the dynamic data to be serialized into JSON
    fastdds::dds::DynamicData::_ref_type dyn_data = get_dynamic_data_(msg, dyn_type);

    if (nullptr == dyn_data)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Not able to get DynamicData from topic " << msg.topic.topic_name() << ".");
        return;
    }

    std::stringstream ss_dyn_data;
    ss_dyn_data << std::setw(4);
    if (fastdds::dds::RETCODE_OK !=
            fastdds::dds::json_serialize(dyn_data, fastdds::dds::DynamicDataJsonFormat::EPROSIMA, ss_dyn_data))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Not able to serialize data of topic " << msg.topic.topic_name() << " into JSON format.");
        return;
    }

    // Fill JSON object with the data
    nlohmann::json json_output;
    {
        // Set id to be the source guid prefix
        std::stringstream ss_source_guid_prefix;
        ss_source_guid_prefix << msg.source_guid.guid_prefix();
        json_output["id"] = ss_source_guid_prefix.str();

        // Set type to be fastdds
        json_output["type"] = "fastdds";

        // Insert type and data (to be filled below) with topic name as key
        json_output[msg.topic.topic_name()] = {
            {"type", msg.topic.type_name},
            {"data", nlohmann::json::object()}
        };

        // Insert data with instance handle as key
        std::stringstream ss_instanceHandle;
        ss_instanceHandle << msg.instanceHandle;
        json_output[msg.topic.topic_name()]["data"][ss_instanceHandle.str()] = nlohmann::json::parse(ss_dyn_data.str());
    }

    // Notify data reception
    if (data_notification_callback_)
    {
        data_notification_callback_(
            msg.topic.topic_name().c_str(),
            json_output.dump(4).c_str(),
            msg.publish_time.to_ns()
            );
    }
}

fastdds::dds::DynamicData::_ref_type Writer::get_dynamic_data_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type) noexcept
{
    // TODO fast this should not be done, but dyn types API is like it is.
    auto& data_no_const = const_cast<eprosima::fastdds::rtps::SerializedPayload_t&>(msg.payload);

    // Create a DynamicData object using the DynamicType
    fastdds::dds::DynamicData::_ref_type dyn_data(
        fastdds::dds::DynamicDataFactory::get_instance()->create_data(dyn_type));

    // Deserialize data into the DynamicData object
    if (!(get_pubsub_type_(dyn_type).deserialize(data_no_const, &dyn_data)))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Failed to deserialize data for topic: " << msg.topic.topic_name());
        return nullptr;
    }

    return dyn_data;
}

fastdds::dds::DynamicPubSubType Writer::get_pubsub_type_(
        const fastdds::dds::DynamicType::_ref_type& dyn_type) noexcept
{
    // Check if we already have this pubsub type
    auto it = dynamic_pubsub_types_.find(dyn_type);
    if (it != dynamic_pubsub_types_.end())
    {
        return it->second;
    }

    // Create a new pubsub type
    fastdds::dds::DynamicPubSubType pubsub_type(dyn_type);
    dynamic_pubsub_types_[dyn_type] = pubsub_type;

    return pubsub_type;
}

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
