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
 * @file serialization.cpp
 */

#include <string>

#include <yaml-cpp/yaml.h>

#include <cpp_utils/exception/InconsistencyException.hpp>
#include <cpp_utils/utils.hpp>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/common/CDRMessage_t.hpp>
#include <fastdds/rtps/common/CdrSerialization.hpp>
#include <fastdds/rtps/common/SerializedPayload.hpp>
#include <fastdds/rtps/common/Types.hpp>

#include <ddsenabler_participants/Constants.hpp>
#include <ddsenabler_participants/types/dynamic_types_collection/DynamicTypesCollection.hpp>
#include <ddsenabler_participants/types/dynamic_types_collection/DynamicTypesCollectionPubSubTypes.hpp>

#include <ddsenabler_participants/Serialization.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {
namespace serialization {

using namespace eprosima::ddspipe::core::types;
using namespace eprosima::fastdds::dds::xtypes;

///////////////////////
// QoS serialization //
///////////////////////

std::string serialize_qos(
        const TopicQoS& qos)
{
    YAML::Node qos_yaml;

    // Reliability tag
    YAML::Node reliability_tag = qos_yaml[QOS_SERIALIZATION_RELIABILITY];
    if (qos.is_reliable())
    {
        reliability_tag = true;
    }
    else
    {
        reliability_tag = false;
    }

    // Durability tag
    YAML::Node durability_tag = qos_yaml[QOS_SERIALIZATION_DURABILITY];
    if (qos.is_transient_local())
    {
        durability_tag = true;
    }
    else
    {
        durability_tag = false;
    }

    // Ownership tag
    YAML::Node ownership_tag = qos_yaml[QOS_SERIALIZATION_OWNERSHIP];
    if (qos.has_ownership())
    {
        ownership_tag = true;
    }
    else
    {
        ownership_tag = false;
    }

    // Keyed tag
    YAML::Node keyed_tag = qos_yaml[QOS_SERIALIZATION_KEYED];
    if (qos.keyed)
    {
        keyed_tag = true;
    }
    else
    {
        keyed_tag = false;
    }

    return YAML::Dump(qos_yaml);
}

TopicQoS deserialize_qos(
        const std::string& qos_str)
{
    TopicQoS qos{};

    YAML::Node qos_yaml = YAML::Load(qos_str);
    bool reliable = qos_yaml[QOS_SERIALIZATION_RELIABILITY].as<bool>();
    bool transient_local = qos_yaml[QOS_SERIALIZATION_DURABILITY].as<bool>();
    bool exclusive_ownership = qos_yaml[QOS_SERIALIZATION_OWNERSHIP].as<bool>();
    bool keyed = qos_yaml[QOS_SERIALIZATION_KEYED].as<bool>();

    // Parse reliability
    if (reliable)
    {
        qos.reliability_qos = ReliabilityKind::RELIABLE;
    }
    else
    {
        qos.reliability_qos = ReliabilityKind::BEST_EFFORT;
    }

    // Parse durability
    if (transient_local)
    {
        qos.durability_qos = DurabilityKind::TRANSIENT_LOCAL;
    }
    else
    {
        qos.durability_qos = DurabilityKind::VOLATILE;
    }

    // Parse ownership
    if (exclusive_ownership)
    {
        qos.ownership_qos = OwnershipQosPolicyKind::EXCLUSIVE_OWNERSHIP_QOS;
    }
    else
    {
        qos.ownership_qos = OwnershipQosPolicyKind::SHARED_OWNERSHIP_QOS;
    }

    // Parse keyed
    qos.keyed = keyed;

    return qos;
}

//////////////////////////
// XTypes serialization //
//////////////////////////

template<class DynamicTypeData>
std::string serialize_type_data(
        const DynamicTypeData& type_data);

template<class DynamicTypeData>
DynamicTypeData deserialize_type_data(
        const std::string& typedata_str);

std::string serialize_type_identifier(
        const TypeIdentifier& type_identifier);

TypeIdentifier deserialize_type_identifier(
        const std::string& typeid_str);

std::string serialize_type_object(
        const TypeObject& type_object);

TypeObject deserialize_type_object(
        const std::string& typeobj_str);

bool serialize_dynamic_type(
        const std::string& type_name,
        const TypeIdentifier& type_identifier,
        DynamicTypesCollection& dynamic_types)
{
    TypeIdentifierPair type_identifiers;

    // NOTE: type_identifier is assumed to be complete
    type_identifiers.type_identifier1(type_identifier);

    TypeInformation type_info;
    if (fastdds::dds::RETCODE_OK !=
            fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_information(
                type_identifiers,
                type_info,
                true))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION,
                "Error getting TypeInformation for type " << type_name);
        return false;
    }

    std::string dependency_name;
    unsigned int dependency_index = 0;
    const auto type_dependencies = type_info.complete().dependent_typeids();
    for (auto dependency : type_dependencies)
    {
        TypeIdentifier dependency_type_identifier;
        dependency_type_identifier = dependency.type_id();

        TypeObject dependency_type_object;
        if (fastdds::dds::RETCODE_OK !=
                fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                    dependency_type_identifier,
                    dependency_type_object))
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION,
                    "Error getting TypeObject of dependency " << "for type " << type_name);
            return false;
        }

        dependency_name = type_name + "_" + std::to_string(dependency_index);

        // Store dependency in dynamic_types collection
        serialize_dynamic_type(dependency_type_identifier, dependency_type_object, dependency_name, dynamic_types);

        // Increment suffix counter
        dependency_index++;
    }

    TypeObject type_object;
    if (fastdds::dds::RETCODE_OK !=
            fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                type_identifier,
                type_object))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION, "Error getting TypeObject for type " << type_name);
        return false;
    }

    // Store dynamic type in dynamic_types collection
    return serialize_dynamic_type(type_identifier, type_object, type_name, dynamic_types);
}

bool serialize_dynamic_type(
        const TypeIdentifier& type_identifier,
        const TypeObject& type_object,
        const std::string& type_name,
        DynamicTypesCollection& dynamic_types)
{
    DynamicType dynamic_type;
    dynamic_type.type_name(type_name);

    try
    {
        dynamic_type.type_identifier(utils::base64_encode(serialize_type_identifier(type_identifier)));
        dynamic_type.type_object(utils::base64_encode(serialize_type_object(type_object)));
    }
    catch (const utils::InconsistencyException& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION, "Error serializing DynamicType. Error message:\n " << e.what());
        return false;
    }

    dynamic_types.dynamic_types().push_back(dynamic_type);

    return true;

}

bool deserialize_dynamic_type(
        const DynamicType& dynamic_type,
        std::string& type_name,
        TypeIdentifier& type_identifier,
        TypeObject& type_object)
{
    std::string typeid_str = utils::base64_decode(dynamic_type.type_identifier());
    std::string typeobj_str = utils::base64_decode(dynamic_type.type_object());

    try
    {
        // Deserialize type identifer and object strings
        type_identifier = deserialize_type_identifier(typeid_str);
        type_object = deserialize_type_object(typeobj_str);
    }
    catch (const utils::InconsistencyException& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION,
                "Failed to deserialize " << dynamic_type.type_name() << " DynamicType: " << e.what());
        return false;
    }

    type_name = dynamic_type.type_name();
    return true;
}

template<class DynamicTypeData>
std::string serialize_type_data(
        const DynamicTypeData& type_data)
{
    // Reserve payload and create buffer
    fastcdr::CdrSizeCalculator calculator(fastcdr::CdrVersion::XCDRv2);
    size_t current_alignment {0};
    size_t size = calculator.calculate_serialized_size(type_data, current_alignment) +
            fastdds::rtps::SerializedPayload_t::representation_header_size;

    fastdds::rtps::SerializedPayload_t payload(static_cast<uint32_t>(size));
    fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

    // Create CDR serializer
    fastcdr::Cdr ser(fastbuffer, fastcdr::Cdr::DEFAULT_ENDIAN,
            fastcdr::CdrVersion::XCDRv2);

    payload.encapsulation = ser.endianness() == fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;

    // Serialize
    fastcdr::serialize(ser, type_data);
    payload.length = (uint32_t)ser.get_serialized_data_length();
    size = (ser.get_serialized_data_length() + 3) & ~3;

    // Create CDR message with payload
    std::unique_ptr<fastdds::rtps::CDRMessage_t> cdr_message = std::make_unique<fastdds::rtps::CDRMessage_t>(payload);

    // Add data
    if (!(cdr_message && (cdr_message->pos + payload.length <= cdr_message->max_size)) ||
            (payload.length > 0 && !payload.data))
    {
        if (!cdr_message)
        {
            throw utils::InconsistencyException(
                      "Error adding data -> cdr_message is null.");
        }
        else if (cdr_message->pos + payload.length > cdr_message->max_size)
        {
            throw utils::InconsistencyException(
                      "Error adding data -> not enough space in cdr_message buffer.");
        }
        else if (payload.length > 0 && !payload.data)
        {
            throw utils::InconsistencyException(
                      "Error adding data -> payload length is greater than 0, but payload data is null.");
        }
    }

    memcpy(&cdr_message->buffer[cdr_message->pos], payload.data, payload.length);
    cdr_message->pos += payload.length;
    cdr_message->length += payload.length;

    fastdds::rtps::octet value = 0;
    for (uint32_t count = payload.length; count < size; ++count)
    {
        const uint32_t size_octet = sizeof(value);
        if (!(cdr_message && (cdr_message->pos + size_octet <= cdr_message->max_size)))
        {
            throw utils::InconsistencyException(
                      "Not enough space in cdr_message buffer.");
        }
        for (uint32_t i = 0; i < size_octet; i++)
        {
            cdr_message->buffer[cdr_message->pos + i] = *((fastdds::rtps::octet*)&value + size_octet - 1 - i);
        }
        cdr_message->pos += size_octet;
        cdr_message->length += size_octet;
    }

    // Copy buffer to string
    std::string typedata_str(reinterpret_cast<char const*>(cdr_message->buffer), size);

    return typedata_str;
}

template<class DynamicTypeData>
DynamicTypeData deserialize_type_data(
        const std::string& typedata_str)
{
    // Create CDR message from string
    // NOTE: Use 0 length to avoid allocation
    fastdds::rtps::CDRMessage_t* cdr_message = new fastdds::rtps::CDRMessage_t(0);
    cdr_message->buffer = (unsigned char*)reinterpret_cast<const unsigned char*>(typedata_str.c_str());
    cdr_message->length = typedata_str.length();
#if __BIG_ENDIAN__
    cdr_message->msg_endian = fastdds::rtps::BIGEND;
#else
    cdr_message->msg_endian = fastdds::rtps::LITTLEEND;
#endif // if __BIG_ENDIAN__

    // Reserve payload and create buffer
    const auto parameter_length = cdr_message->length;
    fastdds::rtps::SerializedPayload_t payload(parameter_length);
    fastcdr::FastBuffer fastbuffer((char*)payload.data, parameter_length);

    // Check cdr message is valid
    if (!cdr_message)
    {
        throw utils::InconsistencyException(
                  "Error reading data -> cdr_message is null.");
    }

    // Check enough space in buffer
    if (!(cdr_message->length >= cdr_message->pos + parameter_length))
    {
        throw utils::InconsistencyException(
                  "Error reading data -> not enough space in cdr_message buffer.");
    }

    // Check length is consistent
    if (!(parameter_length > 0))
    {
        throw utils::InconsistencyException(
                  "Error reading data -> payload length is greater than 0.");
    }

    // Check payload is valid
    if (!payload.data)
    {
        throw utils::InconsistencyException(
                  "Error reading data -> payload data is null.");
    }

    // Copy data
    memcpy(payload.data, &cdr_message->buffer[cdr_message->pos], parameter_length);
    cdr_message->pos += parameter_length;

    // Create CDR deserializer
    fastcdr::Cdr deser(fastbuffer, fastcdr::Cdr::DEFAULT_ENDIAN, fastcdr::CdrVersion::XCDRv2);
    payload.encapsulation = deser.endianness() == fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;

    // Deserialize
    DynamicTypeData type_data;
    fastcdr::deserialize(deser, type_data);

    // Delete CDR message
    // NOTE: set wraps attribute to avoid double free (buffer released by string on destruction)
    cdr_message->wraps = true;
    delete cdr_message;

    return type_data;
}

std::string serialize_type_identifier(
        const TypeIdentifier& type_identifier)
{
    std::string typeid_string;
    try
    {
        typeid_string = serialize_type_data(type_identifier);
    }
    catch (const utils::InconsistencyException& e)
    {
        throw utils::InconsistencyException(std::string("Failed to serialize TypeIdentifier: ") + e.what());
    }

    return typeid_string;
}

TypeIdentifier deserialize_type_identifier(
        const std::string& typeid_str)
{
    TypeIdentifier type_id;
    try
    {
        type_id = deserialize_type_data<TypeIdentifier>(typeid_str);
    }
    catch (const utils::InconsistencyException& e)
    {
        throw utils::InconsistencyException(std::string("Failed to deserialize TypeIdentifier: ") + e.what());
    }

    return type_id;
}

std::string serialize_type_object(
        const TypeObject& type_object)
{
    std::string typeobj_string;
    try
    {
        typeobj_string = serialize_type_data(type_object);
    }
    catch (const utils::InconsistencyException& e)
    {
        throw utils::InconsistencyException(std::string("Failed to serialize TypeObject: ") + e.what());
    }

    return typeobj_string;
}

TypeObject deserialize_type_object(
        const std::string& typeobj_str)
{
    TypeObject type_obj;
    try
    {
        type_obj = deserialize_type_data<TypeObject>(typeobj_str);
    }
    catch (const utils::InconsistencyException& e)
    {
        throw utils::InconsistencyException(std::string("Failed to deserialize TypeObject: ") + e.what());
    }

    return type_obj;
}

std::unique_ptr<fastdds::rtps::SerializedPayload_t> serialize_dynamic_types(
        const DynamicTypesCollection& dynamic_types)
{
    // Serialize dynamic types collection using CDR
    fastdds::dds::TypeSupport type_support(new DynamicTypesCollectionPubSubType());
    auto serialized_payload = std::make_unique<fastdds::rtps::SerializedPayload_t>(
        type_support.calculate_serialized_size(&dynamic_types, fastdds::dds::DEFAULT_DATA_REPRESENTATION));
    if (!type_support.serialize(&dynamic_types, *serialized_payload, fastdds::dds::DEFAULT_DATA_REPRESENTATION))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION,
                "Failed to serialize dynamic types collection.");
        return nullptr;
    }

    return serialized_payload;
}

bool deserialize_dynamic_types(
        const unsigned char* dynamic_types_payload,
        uint32_t dynamic_types_payload_size,
        DynamicTypesCollection& dynamic_types)
{
    fastdds::dds::TypeSupport type_support(new DynamicTypesCollectionPubSubType());
    fastdds::rtps::SerializedPayload_t serialized_payload = fastdds::rtps::SerializedPayload_t(
        dynamic_types_payload_size);
    serialized_payload.length = dynamic_types_payload_size;
    std::memcpy(
        serialized_payload.data,
        dynamic_types_payload,
        dynamic_types_payload_size);

    if (!type_support.deserialize(serialized_payload, &dynamic_types))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_SERIALIZATION,
                "Failed to deserialize dynamic types collection.");
        return false;
    }

    return true;
}

} /* namespace serialization */
} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
