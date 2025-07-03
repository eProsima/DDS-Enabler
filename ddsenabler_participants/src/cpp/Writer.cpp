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

#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/utils.hpp>
#include <fastdds/rtps/common/SerializedPayload.hpp>
#include <fastdds/rtps/common/Types.hpp>

#include <ddsenabler_participants/Serialization.hpp>
#include <ddsenabler_participants/RpcUtils.hpp>
#include <ddsenabler_participants/types/dynamic_types_collection/DynamicTypesCollection.hpp>
#include <ddsenabler_participants/Handler.hpp>

#include <ddsenabler_participants/Writer.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

using namespace eprosima::ddsenabler::participants::serialization;
using namespace eprosima::ddspipe::core::types;


UUID json_to_uuid(const nlohmann::json& json)
{
    UUID uuid;
    if (!json.contains("uuid") || !json["uuid"].is_array() || json["uuid"].size() != sizeof(UUID))

    {
        throw std::invalid_argument("Invalid UUID format in JSON");
    }

    for (size_t i = 0; i < sizeof(UUID); ++i)
    {
        uuid[i] = json["uuid"][i].get<uint8_t>();
    }

    return uuid;
}

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

    // Filter placeholder based on action/service type suffix
    std::string data_placeholder = ss_data_holder.str();
    try {
        using json = nlohmann::json;
        auto j = json::parse(data_placeholder);

        if (type_name.find("SendGoal_Request_") != std::string::npos) {
            // only the 'goal' sub-object
            data_placeholder = j.at("goal").dump();
        }
        else if (type_name.find("GetResult_Response_") != std::string::npos) {
            // only the 'result' sub-object
            data_placeholder = j.at("result").dump();
        }
        else if (type_name.find("FeedbackMessage_") != std::string::npos) {
            // only the 'feedback' sub-object
            data_placeholder = j.at("feedback").dump();
        }
        // all other types (SendGoal_Response, CancelGoal_*, GetResult_Request,
        // GoalStatusArray_ get an empty placeholder
        else if (type_name.find("SendGoal_Response_") != std::string::npos ||
                 type_name.find("CancelGoal_Request_") != std::string::npos ||
                 type_name.find("GetResult_Request_") != std::string::npos ||
                 type_name.find("GoalStatusArray_") != std::string::npos)
        {
            data_placeholder = "{}";
        }
    }
    catch (const std::exception& e) {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
            "JSON filter failed for " << type_name << ": " << e.what());
    }

    // Notify type reception
    if (type_notification_callback_)
    {
        type_notification_callback_(
            type_name.c_str(),
            ss_idl.str().c_str(),
            types_collection_payload->data,
            types_collection_payload->length,
            data_placeholder.c_str()
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
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    //STORE DATA
    if (data_notification_callback_)
    {
        data_notification_callback_(
            msg.topic.topic_name().c_str(),
            json_data.dump(4).c_str(),
            msg.publish_time.to_ns()
            );
    }
}

void Writer::write_service_notification(
    const ddspipe::core::types::RpcTopic& service)
{
    EPROSIMA_LOG_INFO(DDSENABLER_WRITER,
            "Writting service: " << service.service_name() << ".");

    if (service_notification_callback_)
    {
        std::string request_serialized_qos = serialize_qos(service.request_topic().topic_qos);
        std::string reply_serialized_qos = serialize_qos(service.reply_topic().topic_qos);
        service_notification_callback_(
            service.service_name().c_str(),
            service.request_topic().type_name.c_str(),
            service.reply_topic().type_name.c_str(),
            request_serialized_qos.c_str(),
            reply_serialized_qos.c_str()
            );
    }
}

void Writer::write_service_reply_notification(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    // Get the service name
    std::string service_name;
    if(RpcUtils::RpcType::RPC_REPLY != RpcUtils::get_rpc_name(msg.topic.topic_name(), service_name))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Wrong topic name for service reply: " << msg.topic.topic_name() << ".");
        return;
    }

    //STORE DATA
    if (service_reply_notification_callback_)
    {
        service_reply_notification_callback_(
            service_name.c_str(),
            json_data.dump(4).c_str(),
            request_id,
            msg.publish_time.to_ns()
            );
    }
}

void Writer::write_service_request_notification(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    // Get the service name
    std::string service_name;
    if(RpcUtils::RpcType::RPC_REQUEST != RpcUtils::get_rpc_name(msg.topic.topic_name(), service_name))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Wrong topic name for service request: " << msg.topic.topic_name() << ".");
        return;
    }

    //STORE DATA
    if (service_request_notification_callback_)
    {
        service_request_notification_callback_(
            service_name.c_str(),
            json_data.dump(4).c_str(),
            request_id,
            msg.publish_time.to_ns()
            );
    }
}

void Writer::write_action_notification(
        const RpcUtils::RpcAction& action)
{
    EPROSIMA_LOG_INFO(DDSENABLER_WRITER,
            "Writting action: " << action.action_name << ".");

    if (action_notification_callback_)
    {
        std::string goal_request_serialized_qos = serialize_qos(action.goal.request_topic().topic_qos);
        std::string goal_reply_serialized_qos = serialize_qos(action.goal.reply_topic().topic_qos);
        std::string cancel_request_serialized_qos = serialize_qos(action.cancel.request_topic().topic_qos);
        std::string cancel_reply_serialized_qos = serialize_qos(action.cancel.reply_topic().topic_qos);
        std::string result_request_serialized_qos = serialize_qos(action.result.request_topic().topic_qos);
        std::string result_reply_serialized_qos = serialize_qos(action.result.reply_topic().topic_qos);
        std::string feedback_serialized_qos = serialize_qos(action.feedback.topic_qos);
        std::string status_serialized_qos = serialize_qos(action.status.topic_qos);

        action_notification_callback_(
            action.action_name.c_str(),
            action.goal.request_topic().type_name.c_str(),
            action.goal.reply_topic().type_name.c_str(),
            action.cancel.request_topic().type_name.c_str(),
            action.cancel.reply_topic().type_name.c_str(),
            action.result.request_topic().type_name.c_str(),
            action.result.reply_topic().type_name.c_str(),
            action.feedback.type_name.c_str(),
            action.status.type_name.c_str(),
            goal_request_serialized_qos.c_str(),
            goal_reply_serialized_qos.c_str(),
            cancel_request_serialized_qos.c_str(),
            cancel_reply_serialized_qos.c_str(),
            result_request_serialized_qos.c_str(),
            result_reply_serialized_qos.c_str(),
            feedback_serialized_qos.c_str(),
            status_serialized_qos.c_str()
            );
    }
}

void Writer::write_action_result_notification(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const UUID& action_id)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    // Get the action name
    std::string action_name;
    if(RpcUtils::RpcType::ACTION_RESULT_REPLY != RpcUtils::get_rpc_name(msg.topic.topic_name(), action_name))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Wrong topic name for action result: " << msg.topic.topic_name() << ".");
        return;
    }

    //STORE DATA
    if (action_result_notification_callback_)
    {
        action_result_notification_callback_(
            action_name.c_str(),
            json_data.dump(4).c_str(),
            action_id,
            msg.publish_time.to_ns()
            );
    }
}

void Writer::write_action_feedback_notification(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    // Get the action name
    std::string action_name;
    RpcUtils::get_rpc_name(msg.topic.topic_name(), action_name);

    std::stringstream instanceHandle;
    instanceHandle << msg.instanceHandle;
    if (action_feedback_notification_callback_)
    {
        action_feedback_notification_callback_(
            action_name.c_str(),
            json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["feedback"].dump(4).c_str(),
            json_to_uuid(json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goal_id"]),
            msg.publish_time.to_ns()
            );
    }
}

void Writer::write_action_goal_reply_notification(
    const Message& msg,
    const fastdds::dds::DynamicType::_ref_type& dyn_type,
    const UUID& action_id)
{
    std::string action_name;
    RpcUtils::get_rpc_name(msg.topic.topic_name(), action_name);

    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    std::string status_message = "Action goal accepted";
    ddsenabler::participants::STATUS_CODE status_code = ddsenabler::participants::STATUS_CODE::STATUS_ACCEPTED;
    std::stringstream instanceHandle;
    instanceHandle << msg.instanceHandle;
    try
    {
        if (!json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["accepted"])
        {
            status_message = "Action goal rejected";
            status_code = ddsenabler::participants::STATUS_CODE::STATUS_REJECTED;
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Error parsing action goal reply notification: " << e.what());
        status_message = "Action goal reply notification malformed";
        status_code = ddsenabler::participants::STATUS_CODE::STATUS_UNKNOWN;
    }
    if (action_status_notification_callback_)
    {
        action_status_notification_callback_(
            action_name.c_str(),
            action_id,
            status_code,
            status_message.c_str(),
            msg.publish_time.to_ns()
            );
    }

    if (ddsenabler::participants::STATUS_CODE::STATUS_ACCEPTED == status_code &&
            send_action_get_result_request_callback_ &&
            !send_action_get_result_request_callback_(
                action_name.c_str(),
                action_id))
    {
        status_message = "Action goal aborted";
        status_code = ddsenabler::participants::STATUS_CODE::STATUS_ABORTED;

        if (action_status_notification_callback_)
        {
            action_status_notification_callback_(
                action_name.c_str(),
                action_id,
                status_code,
                status_message.c_str(),
                msg.publish_time.to_ns()
                );
        }
    }
}

void Writer::write_action_cancel_reply_notification(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        const uint64_t request_id)
{
    ddsenabler::participants::STATUS_CODE status_code = ddsenabler::participants::STATUS_CODE::STATUS_CANCELED;
    std::string action_name;
    RpcUtils::get_rpc_name(msg.topic.topic_name(), action_name);

    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    std::stringstream instanceHandle;
    instanceHandle << msg.instanceHandle;

    std::string status_message;
    int return_code;
    try
    {
        return_code = json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["return_code"];
    }
    catch (const nlohmann::json::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Error parsing action cancel reply notification: " << e.what());
        status_code = ddsenabler::participants::STATUS_CODE::STATUS_UNKNOWN;
        return;
    }
    switch (return_code)
    {
        case 0:
            status_message = "Action canceled successfully";
            status_code = ddsenabler::participants::STATUS_CODE::STATUS_CANCELED;
            break;
        case 1:
            status_message = "Action cancel request rejected";
            status_code = ddsenabler::participants::STATUS_CODE::STATUS_REJECTED;
            break;
        case 2:
            status_message = "Action cancel request unknown goal ID";
            status_code = ddsenabler::participants::STATUS_CODE::STATUS_CANCEL_REQUEST_FAILED;
            break;
        case 3:
            status_message = "Action cancel request rejected as goal was already terminated";
            status_code = ddsenabler::participants::STATUS_CODE::STATUS_CANCEL_REQUEST_FAILED;
            break;
        default:
            status_message = "Action cancel request unknown code";
            status_code = ddsenabler::participants::STATUS_CODE::STATUS_UNKNOWN;
            break;
    }

    try
    {
        const auto& goals = json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goals_canceling"];
        for (const auto& goal : goals)
        {
            UUID msg_action_id = json_to_uuid(goal["goal_id"]);
            if(is_UUID_active_callback_ && !is_UUID_active_callback_(action_name, msg_action_id))
                continue;

            if (action_status_notification_callback_)
            {
                action_status_notification_callback_(
                    action_name.c_str(),
                    msg_action_id,
                    status_code,
                    status_message.c_str(),
                    msg.publish_time.to_ns()
                    );
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Error parsing action cancel reply notification goals: " << e.what());
    }
}

void Writer::write_action_status_notification(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    // Get the action name
    std::string action_name;
    RpcUtils::get_rpc_name(msg.topic.topic_name(), action_name);

    std::stringstream instanceHandle;
    instanceHandle << msg.instanceHandle;

    try
    {
        const auto& list = json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["status_list"];
        for (const auto& status : list)
        {
            UUID uuid = json_to_uuid(status["goal_info"]["goal_id"]);
            if(is_UUID_active_callback_ && !is_UUID_active_callback_(action_name, uuid))
                continue;

            ddsenabler::participants::STATUS_CODE status_code(status["status"]);
            std::string status_message;
            switch (status_code)
            {
                case ddsenabler::participants::STATUS_CODE::STATUS_UNKNOWN:
                    status_message = "The status has not been properly set";
                    break;

                case ddsenabler::participants::STATUS_CODE::STATUS_ACCEPTED:
                    status_message = "The goal has been accepted and is awaiting execution";
                    break;

                case ddsenabler::participants::STATUS_CODE::STATUS_EXECUTING:
                    status_message = "The goal is currently being executed by the action server";
                    break;

                case ddsenabler::participants::STATUS_CODE::STATUS_CANCELING:
                    status_message = "The client has requested that the goal be canceled and the action server has accepted the cancel request";
                    break;

                case ddsenabler::participants::STATUS_CODE::STATUS_SUCCEEDED:
                    if (erase_action_UUID_callback_)
                        erase_action_UUID_callback_(uuid, ActionEraseReason::FINAL_STATUS);
                    status_message = "The goal was achieved successfully by the action server";
                    break;

                case ddsenabler::participants::STATUS_CODE::STATUS_CANCELED:
                    if (erase_action_UUID_callback_)
                        erase_action_UUID_callback_(uuid, ActionEraseReason::FINAL_STATUS);
                    status_message = "The goal was canceled after an external request from an action client";
                    break;

                case ddsenabler::participants::STATUS_CODE::STATUS_ABORTED:
                    if (erase_action_UUID_callback_)
                        erase_action_UUID_callback_(uuid, ActionEraseReason::FINAL_STATUS);
                    status_message = "The goal was terminated by the action server without an external request";
                    break;
                case ddsenabler::participants::STATUS_CODE::STATUS_REJECTED:
                    if (erase_action_UUID_callback_)
                        erase_action_UUID_callback_(uuid, ActionEraseReason::FINAL_STATUS);
                    status_message = "The goal was rejected by the action server, it will not be executed";
                    break;
                default:
                    status_message = "Unknown status code";
                    break;
            }

            if (action_status_notification_callback_)
            {
                action_status_notification_callback_(
                    action_name.c_str(),
                    uuid,
                    status_code,
                    status_message.c_str(),
                    msg.publish_time.to_ns()
                    );
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Error parsing action status notification: " << e.what());
        return;
    }
}

void Writer::write_action_request_notification(
    const Message& msg,
    const fastdds::dds::DynamicType::_ref_type& dyn_type,
    const uint64_t request_id)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return;
    }

    // Get the action name
    std::string action_name;
    RpcUtils::RpcType rpc_type = RpcUtils::get_rpc_name(msg.topic.topic_name(), action_name);

    std::stringstream instanceHandle;
    instanceHandle << msg.instanceHandle;
    try
    {
        if (RpcUtils::RpcType::ACTION_GOAL_REQUEST == rpc_type)
        {
            bool accepted;
            if (action_goal_request_notification_callback_)
                accepted = action_goal_request_notification_callback_(
                        action_name.c_str(),
                        json_data.dump(4).c_str(),
                        json_to_uuid(json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goal_id"]),
                        msg.publish_time.to_ns()
                        );

            send_action_send_goal_reply_callback_(
                action_name.c_str(),
                request_id,
                accepted);

            return;
        }
        if (RpcUtils::RpcType::ACTION_CANCEL_REQUEST == rpc_type)
        {
            if (action_cancel_request_notification_callback_)
            {
                auto sec = json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goal_info"]["sec"];
                auto nanosec = json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goal_info"]["nanosec"];
                int64_t timestamp = static_cast<int64_t>(sec.get<int64_t>()) * 1000000000 +
                    static_cast<int64_t>(nanosec.get<uint32_t>());
                action_cancel_request_notification_callback_(
                    action_name.c_str(),
                    json_to_uuid(json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goal_id"]),
                    timestamp,
                    request_id,
                    msg.publish_time.to_ns()
                    );
            }
            return;
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Error parsing action request notification: " << e.what());
        return;
    }
}

bool Writer::uuid_from_request_json(
    const Message& msg,
    const fastdds::dds::DynamicType::_ref_type& dyn_type,
    UUID& uuid)
{
    nlohmann::json json_data;
    if (!prepare_json_data_(msg, dyn_type, json_data))
    {
        return false;
    }

    std::stringstream instanceHandle;
    instanceHandle << msg.instanceHandle;
    try
    {
        uuid = json_to_uuid(json_data[msg.topic.topic_name()]["data"][instanceHandle.str()]["goal_id"]);
    }
    catch (const nlohmann::json::exception& e)
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Error parsing UUID from request JSON: " << e.what());
        return false;
    }
    return true;
}

bool Writer::prepare_json_data_(
        const Message& msg,
        const fastdds::dds::DynamicType::_ref_type& dyn_type,
        nlohmann::json& json_output)
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
        return false;
    }

    std::stringstream ss_dyn_data;
    ss_dyn_data << std::setw(4);
    if (fastdds::dds::RETCODE_OK !=
            fastdds::dds::json_serialize(dyn_data, fastdds::dds::DynamicDataJsonFormat::EPROSIMA, ss_dyn_data))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_WRITER,
                "Not able to serialize data of topic " << msg.topic.topic_name() << " into JSON format.");
        return false;
    }

    // Fill JSON object with the data
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

    return true;
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
