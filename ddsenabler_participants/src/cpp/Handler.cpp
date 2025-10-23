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

    writer_->set_is_UUID_active_callback(
        [this](const std::string& action_name, const UUID& uuid)
        {
            return this->is_UUID_active(action_name, uuid, nullptr);
        }
        );

    writer_->set_erase_action_UUID_callback(
        [this](const UUID& uuid, ActionEraseReason reason)
        {
            return this->erase_action_UUID(uuid, reason);
        }
        );
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
    std::lock_guard<std::recursive_mutex> lock(mtx_);

    add_schema_nts_(dyn_type, type_id);
}

void Handler::add_topic(
        const DdsTopic& topic)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);

    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Adding topic: " << topic << ".");

    write_topic_nts_(topic);
}

void Handler::add_service(
        const RpcTopic& service)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);

    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Adding service: " << service.service_name() << ".");

    write_service_nts_(service);
}

void Handler::add_action(
        const RpcAction& action)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);

    EPROSIMA_LOG_INFO(DDSENABLER_HANDLER,
            "Adding action: " << action.action_name << ".");

    write_action_nts_(action);
}

void Handler::add_data(
        const DdsTopic& topic,
        RtpsPayloadData& data)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);

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

    std::shared_ptr<RpcInfo> rpc_info;
    try
    {
        rpc_info = std::make_shared<RpcInfo>(topic.m_topic_name);
    }
    catch (const std::exception& e)
    {
        throw utils::InconsistencyException(STR_ENTRY << e.what());
    }

    switch (rpc_info->rpc_type)
    {
        case RpcType::NONE:
        {
            write_sample_nts_(msg, dyn_type);
            break;
        }

        // SERVICE
        case RpcType::SERVICE:
        {
            if (rpc_info->service_type == ServiceType::REQUEST)
            {
                requests_id_++;
                RpcPayloadData& rpc_data = dynamic_cast<RpcPayloadData&>(data);
                rpc_data.sent_sequence_number = eprosima::fastdds::rtps::SequenceNumber_t(requests_id_);
                write_service_request_nts_(msg, dyn_type, requests_id_, rpc_info->service_name);
            }
            else
            {
                auto request_id =
                        dynamic_cast<ddspipe::core::types::RpcPayloadData&>(data).write_params.get_reference().
                                related_sample_identity().sequence_number().to64long();
                write_service_reply_nts_(msg, dyn_type, request_id, rpc_info->service_name);
            }
            break;
        }

        // ACTIONS CLIENT
        case RpcType::ACTION:
        {
            switch (rpc_info->service_type)
            {
                case ServiceType::REPLY:
                {
                    switch (rpc_info->action_type)
                    {
                        case ActionType::RESULT:
                        {
                            auto action_id =
                                    dynamic_cast<ddspipe::core::types::RpcPayloadData&>(data).write_params.get_reference()
                                            .related_sample_identity().sequence_number().to64long();
                            UUID action_id_uuid;
                            if (get_action_request_UUID(action_id, ActionType::RESULT, action_id_uuid))
                            {
                                write_action_result_nts_(msg, dyn_type, action_id_uuid, rpc_info->action_name);
                            }
                            erase_action_UUID(action_id_uuid, ActionEraseReason::RESULT);
                            break;
                        }

                        case ActionType::GOAL:
                        {
                            auto action_id =
                                    dynamic_cast<ddspipe::core::types::RpcPayloadData&>(data).write_params.get_reference()
                                            .related_sample_identity().sequence_number().to64long();
                            UUID action_id_uuid;
                            if (get_action_request_UUID(action_id, ActionType::GOAL, action_id_uuid))
                            {
                                write_action_goal_reply_nts_(msg, dyn_type, action_id_uuid, rpc_info->action_name);
                            }
                            break;
                        }

                        case ActionType::CANCEL:
                        {
                            auto request_id =
                                    dynamic_cast<ddspipe::core::types::RpcPayloadData&>(data).write_params.get_reference()
                                            .related_sample_identity().sequence_number().to64long();
                            write_action_cancel_reply_nts_(msg, dyn_type, request_id, rpc_info->action_name);
                            break;
                        }

                        default:
                        {
                            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                                    "Unknown action type for topic " << topic.m_topic_name << ".");
                            break;
                        }
                    }
                    break;
                }

                case ServiceType::REQUEST:
                {
                    switch (rpc_info->action_type)
                    {
                        case ActionType::GOAL:
                        case ActionType::CANCEL:
                        {
                            requests_id_++;
                            RpcPayloadData& rpc_data = dynamic_cast<RpcPayloadData&>(data);
                            rpc_data.sent_sequence_number = eprosima::fastdds::rtps::SequenceNumber_t(requests_id_);
                            UUID uuid;
                            if (!writer_->uuid_from_request_json(
                                        msg,
                                        dyn_type,
                                        uuid))
                            {
                                EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                                        "Failed to extract UUID from send_goal_request JSON.");
                                return;
                            }

                            if (store_action_request(
                                        rpc_info->action_name,
                                        uuid,
                                        requests_id_,
                                        rpc_info->action_type))
                            {
                                write_action_request_nts_(msg, dyn_type, requests_id_, rpc_info->action_name,
                                        rpc_info->action_type);
                            }

                            break;
                        }

                        case ActionType::RESULT:
                        {
                            UUID uuid;
                            if (!writer_->uuid_from_request_json(
                                        msg,
                                        dyn_type,
                                        uuid))
                            {
                                EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                                        "Failed to extract UUID from get_result_request JSON.");
                                return;
                            }

                            requests_id_++;
                            RpcPayloadData& rpc_data = dynamic_cast<RpcPayloadData&>(data);
                            rpc_data.sent_sequence_number = eprosima::fastdds::rtps::SequenceNumber_t(requests_id_);
                            if (!store_action_request(
                                        rpc_info->action_name,
                                        uuid,
                                        requests_id_,
                                        ActionType::RESULT))
                            {
                                EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                                        "Failed to store action request for get_result_request.");
                                return;
                            }

                            std::string result;
                            if (get_action_result(uuid, result))
                            {
                                if (send_action_get_result_reply_callback_)
                                {
                                    send_action_get_result_reply_callback_(
                                        rpc_info->action_name,
                                        uuid,
                                        result,
                                        requests_id_);
                                }
                            }
                            break;
                        }

                        default:
                            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                                    "Unknown action type for topic " << topic.m_topic_name << ".");
                            break;
                    }
                    break;
                }

                case ServiceType::NONE:
                {
                    switch (rpc_info->action_type)
                    {
                        case ActionType::FEEDBACK:
                        {
                            write_action_feedback_nts_(msg, dyn_type, rpc_info->action_name);
                            break;
                        }

                        case ActionType::STATUS:
                        {
                            write_action_status_nts_(msg, dyn_type, rpc_info->action_name);
                            break;
                        }

                        default:
                        {
                            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                                    "Unknown RPC type for topic " << topic.m_topic_name << ".");
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

bool Handler::get_type_identifier(
        const std::string& type_name,
        fastdds::dds::xtypes::TypeIdentifier& type_identifier)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);

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
    std::lock_guard<std::recursive_mutex> lock(mtx_);

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

void Handler::write_service_nts_(
        const RpcTopic& service)
{
    writer_->write_service_notification(service);
}

void Handler::write_service_reply_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id,
        const std::string& service_name)
{
    writer_->write_service_reply_notification(msg, dyn_type, request_id, service_name);
}

void Handler::write_service_request_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id,
        const std::string& service_name)
{
    writer_->write_service_request_notification(msg, dyn_type, request_id, service_name);
}

void Handler::write_action_nts_(
        const RpcAction& action)
{
    writer_->write_action_notification(action);
}

void Handler::write_action_result_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const UUID& action_id,
        const std::string& action_name)
{
    writer_->write_action_result_notification(msg, dyn_type, action_id, action_name);
}

void Handler::write_action_feedback_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const std::string& action_name)
{
    writer_->write_action_feedback_notification(msg, dyn_type, action_name);
}

void Handler::write_action_goal_reply_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const UUID& action_id,
        const std::string& action_name)
{
    writer_->write_action_goal_reply_notification(msg, dyn_type, action_id, action_name);
}

void Handler::write_action_cancel_reply_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id,
        const std::string& action_name)
{
    writer_->write_action_cancel_reply_notification(msg, dyn_type, request_id, action_name);
}

void Handler::write_action_status_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const std::string& action_name)
{
    writer_->write_action_status_notification(msg, dyn_type, action_name);
}

void Handler::write_action_request_nts_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id,
        const std::string& action_name,
        const ActionType action_type)
{
    writer_->write_action_request_notification(msg, dyn_type, request_id, action_name, action_type);
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
                "Unexpected dynamic types collection format: " << type_name << " expected to be last item, found " <<
                _type_name <<
                " instead.");
        return false;
    }

    // Assign type identifier and object after all types have been registered
    type_identifier = _type_identifier;
    type_object = _type_object;

    return true;
}

bool Handler::store_action_request(
        const std::string& action_name,
        const UUID& action_id,
        const uint64_t request_id,
        const ActionType action_type,
        const RpcProtocol RpcProtocol)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);

    auto it = action_request_id_to_uuid_.find(action_id);
    if (it != action_request_id_to_uuid_.end())
    {
        if (it->second.action_name != action_name)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Action name mismatch for action, expected "
                    << it->second.action_name << ", got " << action_name);
            return false;
        }
        if (ActionType::GOAL == action_type)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Cannot store action goal request as action id already exists.");
            return false;
        }
        // If it exists, update the request_id for the given action_type
        it->second.set_request(request_id, action_type);
    }
    else
    {
        // If it does not exist, create a new entry only if the action type is goal request
        if (ActionType::GOAL != action_type)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Cannot store action request, action does not exist and request type is not GOAL.");
            return false;
        }
        action_request_id_to_uuid_[action_id] = ActionRequestInfo(action_name, action_type, request_id, RpcProtocol);
    }

    return true;
}

bool Handler::handle_action_result(
        const std::string& action_name,
        const UUID& action_id,
        const std::string& result)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto it = action_request_id_to_uuid_.find(action_id);
    if (it != action_request_id_to_uuid_.end())
    {
        if (it->second.action_name != action_name)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Action name mismatch for action, expected " << it->second.action_name
                                                                 << ", got " << action_name);
            return false;
        }
        if (it->second.result_request_id != 0)
        {
            return send_action_get_result_reply_callback_(
                action_name,
                action_id,
                result,
                it->second.result_request_id);
        }
        if (it->second.set_result(std::move(result)))
        {
            return true;
        }
        else
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Failed to store action result for action, result already set.");
            return false;
        }
    }
    EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
            "Failed to send action result, goal id not found.");
    return false;
}

void Handler::erase_action_UUID(
        const UUID& action_id,
        ActionEraseReason erase_reason)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto it = action_request_id_to_uuid_.find(action_id);
    if (it != action_request_id_to_uuid_.end())
    {
        if (it->second.erase(erase_reason))
        {
            action_request_id_to_uuid_.erase(it);
        }
    }
}

bool Handler::is_UUID_active(
        const std::string& action_name,
        const UUID& action_id,
        std::chrono::system_clock::time_point* goal_accepted_stamp)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto it = action_request_id_to_uuid_.find(action_id);
    if (it != action_request_id_to_uuid_.end() && action_name == it->second.action_name)
    {
        if (goal_accepted_stamp)
        {
            *goal_accepted_stamp = it->second.goal_accepted_stamp;
        }
        return true;
    }

    return false;
}

RpcProtocol Handler::get_action_rpc_protocol(
        const std::string& action_name,
        const UUID& action_id)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto it = action_request_id_to_uuid_.find(action_id);
    if (it != action_request_id_to_uuid_.end() && action_name == it->second.action_name)
    {
        return it->second.rpc_protocol;
    }

    return RpcProtocol::PROTOCOL_UNKNOWN;
}

bool Handler::get_action_request_UUID(
        const uint64_t request_id,
        const ActionType action_type,
        UUID& action_id)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    for (auto it = action_request_id_to_uuid_.begin(); it != action_request_id_to_uuid_.end(); ++it)
    {
        try
        {
            uint64_t action_request_id = it->second.get_request(action_type);
            if (request_id == action_request_id)
            {
                action_id = it->first;
                return true;
            }
        }
        catch (const std::exception& e)
        {
            EPROSIMA_LOG_ERROR(DDSENABLER_HANDLER,
                    "Error getting action request ID: " << e.what());
        }
    }
    return false;
}

bool Handler::get_action_result(
        const UUID& action_id,
        std::string& result)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto it = action_request_id_to_uuid_.find(action_id);
    if (it != action_request_id_to_uuid_.end())
    {
        if (!it->second.result.empty())
        {
            result = it->second.result;
            return true;
        }
    }
    return false;
}

void Handler::set_data_notification_callback(
        participants::DdsDataNotification callback)
{
    writer_->set_data_notification_callback(callback);
}

void Handler::set_topic_notification_callback(
        participants::DdsTopicNotification callback)
{
    writer_->set_topic_notification_callback(callback);
}

void Handler::set_type_notification_callback(
        participants::DdsTypeNotification callback)
{
    writer_->set_type_notification_callback(callback);
}

void Handler::set_type_query_callback(
        participants::DdsTypeQuery callback)
{
    type_query_callback_ = callback;
}

void Handler::set_service_notification_callback(
        participants::ServiceNotification callback)
{
    writer_->set_service_notification_callback(callback);
}

void Handler::set_service_reply_notification_callback(
        participants::ServiceReplyNotification callback)
{
    writer_->set_service_reply_notification_callback(callback);
}

void Handler::set_service_request_notification_callback(
        participants::ServiceRequestNotification callback)
{
    writer_->set_service_request_notification_callback(callback);
}

void Handler::set_action_notification_callback(
        participants::ActionNotification callback)
{
    writer_->set_action_notification_callback(callback);
}

void Handler::set_action_result_notification_callback(
        participants::ActionResultNotification callback)
{
    writer_->set_action_result_notification_callback(callback);
}

void Handler::set_action_feedback_notification_callback(
        participants::ActionFeedbackNotification callback)
{
    writer_->set_action_feedback_notification_callback(callback);
}

void Handler::set_action_status_notification_callback(
        participants::ActionStatusNotification callback)
{
    writer_->set_action_status_notification_callback(callback);
}

void Handler::set_send_action_get_result_request_callback(
        std::function<bool(const std::string&, const participants::UUID&)> callback)
{
    writer_->set_send_action_get_result_request_callback(callback);
}

void Handler::set_action_goal_request_notification_callback(
        participants::ActionGoalRequestNotification callback)
{
    writer_->set_action_goal_request_notification_callback(callback);
}

void Handler::set_action_cancel_request_notification_callback(
        participants::ActionCancelRequestNotification callback)
{
    writer_->set_action_cancel_request_notification_callback(callback);
}

void Handler::set_send_action_send_goal_reply_callback(
        std::function<void(const std::string&, const uint64_t, bool accepted)> callback)
{
    writer_->set_send_action_send_goal_reply_callback(callback);
}

void Handler::set_send_action_get_result_reply_callback(
        std::function<bool(const std::string&, const UUID&, const std::string&, const uint64_t)> callback)
{
    send_action_get_result_reply_callback_ = callback;
}

uint64_t Handler::get_new_request_id()
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    return ++requests_id_;
}

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
