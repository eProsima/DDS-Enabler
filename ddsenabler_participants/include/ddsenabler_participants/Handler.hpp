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
 * @file Handler.hpp
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <ddspipe_core/efficiency/payload/PayloadPool.hpp>
#include <ddspipe_core/types/data/RtpsPayloadData.hpp>
#include <ddspipe_core/types/data/RpcPayloadData.hpp>
#include <ddspipe_core/types/dds/Payload.hpp>
#include <ddspipe_core/types/topic/dds/DdsTopic.hpp>

#include <ddspipe_participants/participant/dynamic_types/ISchemaHandler.hpp>

#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/HandlerConfiguration.hpp>
#include <ddsenabler_participants/Message.hpp>
#include <ddsenabler_participants/Writer.hpp>
#include <ddsenabler_participants/rpc/RpcUtils.hpp>
#include <ddsenabler_participants/rpc/RpcStructs.hpp>
#include <ddsenabler_participants/library/library_dll.h>

namespace std {
template<>
struct hash<eprosima::fastdds::dds::xtypes::TypeIdentifier>
{
    std::size_t operator ()(
            const eprosima::fastdds::dds::xtypes::TypeIdentifier& k) const
    {
        // The collection only has direct hash TypeIdentifiers so the EquivalenceHash can be used.
        return (static_cast<size_t>(k.equivalence_hash()[0]) << 16) |
               (static_cast<size_t>(k.equivalence_hash()[1]) << 8) |
               (static_cast<size_t>(k.equivalence_hash()[2]));
    }

};

template<>
struct hash<eprosima::ddsenabler::participants::UUID>
{
    std::size_t operator ()(
            const eprosima::ddsenabler::participants::UUID& uuid) const noexcept
    {
        std::size_t hash = 0;
        for (uint8_t byte : uuid)
        {
            hash ^= std::hash<uint8_t>{}(byte) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }

};

} // std

namespace eprosima {
namespace ddsenabler {
namespace participants {

class Writer;

/**
 * Class that manages the interaction between \c EnablerParticipant and the user's app.
 * Payloads are efficiently passed from DDS Pipe to the user's app without copying data (only references).
 *
 * @implements ISchemaHandler
 */
class Handler : public ddspipe::participants::ISchemaHandler
{

public:

    /**
     * Handler constructor by required values.
     *
     * Creates Handler instance with given configuration, payload pool.
     *
     * @param config:       Structure encapsulating all configuration options.
     * @param payload_pool: Owner of every payload contained in received messages.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    Handler(
            const HandlerConfiguration& config,
            const std::shared_ptr<ddspipe::core::PayloadPool>& payload_pool);

    /**
     * @brief Destructor
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    ~Handler();

    /**
     * @brief Add a type schema, associated to the given \c dyn_type and \c type_id.
     *
     * @param [in] dyn_type DynamicType containing the type information required to generate the schema.
     * @param [in] type_id TypeIdentifier of the type.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void add_schema(
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const fastdds::dds::xtypes::TypeIdentifier& type_id) override;

    /**
     * @brief Add a topic, associated to the given \c topic.
     *
     * @param [in] topic DDS topic to be added.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void add_topic(
            const ddspipe::core::types::DdsTopic& topic);

    /**
     * @brief Add a service, associated to the given \c service.
     *
     * @param [in] service RPC topic associated to this service.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void add_service(
            const ddspipe::core::types::RpcTopic& service);

    /**
     * @brief Add an action, associated to the given \c action.
     *
     * @param [in] action RPC action associated to this action.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void add_action(
            const RpcAction& action);

    /**
     * @brief Add a data sample, associated to the given \c topic.
     *
     * @param [in] topic DDS topic associated to this sample.
     * @param [in] data payload data to be added.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void add_data(
            const ddspipe::core::types::DdsTopic& topic,
            ddspipe::core::types::RtpsPayloadData& data) override;

    /**
     * @brief Get the TypeIdentifier associated to the given type name.
     *
     * @param [in] type_name Name of the type to be retrieved.
     * @param [out] type_identifier TypeIdentifier of the type.
     * @return \c true if the type was found, \c false otherwise.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    bool get_type_identifier(
            const std::string& type_name,
            fastdds::dds::xtypes::TypeIdentifier& type_identifier);

    /**
     * @brief Get the serialized data (payload) associated to the given type name from a JSON string.
     *
     * @param [in] type_name Name of the type of the data to be serialized.
     * @param [in] json JSON string containing the data to be serialized.
     * @param [out] payload Payload reference where the serialized data will be stored.
     * @return \c true if the data was successfully serialized, \c false otherwise.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    bool get_serialized_data(
            const std::string& type_name,
            const std::string& json,
            ddspipe::core::types::Payload& payload);

    /**
     * @brief Store an action request (goal, cancel or result) with its associated UUID and request ID.
     * This info will later be used to associate the reply id with the UUID of the action.
     *
     * @note: An ActionRequestInfo can only be created if ActionType is GOAL, otherwise it will return false.
     * @note: When creating the ActionRequestInfo the current time will be stored as the goal accepted stamp.
     *
     * @param [in] action_name Name of the action.
     * @param [in] action_id UUID of the action.
     * @param [in] request_id Request ID of the action request.
     * @param [in] action_type Type of the action (GOAL, RESULT, CANCEL).
     * @param [in] RpcProtocol Protocol of the action, only used to create the ActionRequestInfo if action_type is GOAL.
     * @return \c true if the action request was successfully stored, \c false otherwise.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    bool store_action_request(
            const std::string& action_name,
            const UUID& action_id,
            const uint64_t request_id,
            const ActionType action_type,
            const RpcProtocol RpcProtocol = RpcProtocol::ROS2);

    /**
     * @brief Send the reply containing the result of an action or store it for a later reply.
     * If the result request was previously received the reply will be sent, otherwise the result will be stored.
     *
     * @param [in] action_name Name of the action.
     * @param [in] action_id UUID of the action.
     * @param [in] result Result of the action.
     * @return \c true if the result was successfully stored, \c false otherwise.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    bool handle_action_result(
            const std::string& action_name,
            const UUID& action_id,
            const std::string& result);

    /**
     * @brief Erase an action UUID from the internal map that tracks which actions are active.
     *
     * An action is considered active if it has a goal request and has not received its result and a final status (aborted, rejected, succeeded).
     * The action will be forced to be erased if \c force_erase is set to \c true , allowing to erase actions without complying with the previous condition.
     *
     * @note: The status updates are not a strict requirement in the ROS2 actions specification, so there could be servers not sending them.
     *
     * @param [in] action_id UUID of the action to be erased.
     * @param [in] force_erase If \c true, the action will be erased regardless of its status.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void erase_action_UUID(
            const UUID& action_id,
            ActionEraseReason erase_reason);

    /**
     * @brief Check if an action represented by its UUID is active.
     * An action is considered active if it has a goal request and has not received its result and a final status (aborted, rejected, succeeded).
     *
     * @param [in] action_name Name of the action.
     * @param [in] action_id UUID of the action to be checked.
     * @param [out] goal_accepted_stamp Optional pointer to get the timestamp when the goal was accepted.
     * @return \c true if the action is active, \c false otherwise.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    bool is_UUID_active(
            const std::string& action_name,
            const UUID& action_id,
            std::chrono::system_clock::time_point* goal_accepted_stamp = nullptr);

    /**
     * @brief Get the RPC protocol of an action represented by its UUID.
     *
     * @param [in] action_name Name of the action.
     * @param [in] action_id UUID of the action to be checked.
     * @return The RPC protocol of the action, or RpcProtocol::PROTOCOL_UNKNOWN if the action is not found.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    RpcProtocol get_action_rpc_protocol(
            const std::string& action_name,
            const UUID& action_id);

    /**
     * @brief Set the data notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_data_notification_callback(
            participants::DdsDataNotification callback);

    /**
     * @brief Set the topic notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_topic_notification_callback(
            participants::DdsTopicNotification callback);

    /**
     * @brief Set the type notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_type_notification_callback(
            participants::DdsTypeNotification callback);

    /**
     * @brief Set the type query callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_type_query_callback(
            participants::DdsTypeQuery callback);
    /**
     * @brief Set the service notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_service_notification_callback(
            participants::ServiceNotification callback);

    /**
     * @brief Set the service reply notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_service_reply_notification_callback(
            participants::ServiceReplyNotification callback);

    /**
     * @brief Set the service request notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_service_request_notification_callback(
            participants::ServiceRequestNotification callback);

    /**
     * @brief Set the action notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_notification_callback(
            participants::ActionNotification callback);

    /**
     * @brief Set the action result notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_result_notification_callback(
            participants::ActionResultNotification callback);

    /**
     * @brief Set the action feedback notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_feedback_notification_callback(
            participants::ActionFeedbackNotification callback);

    /**
     * @brief Set the action status notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_status_notification_callback(
            participants::ActionStatusNotification callback);

    /**
     * @brief Set the action get result request callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_send_action_get_result_request_callback(
            std::function<bool(const std::string&, const participants::UUID&)> callback);

    /**
     * @brief Set the action goal request notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_goal_request_notification_callback(
            participants::ActionGoalRequestNotification callback);

    /**
     * @brief Set the action cancel request notification callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_action_cancel_request_notification_callback(
            participants::ActionCancelRequestNotification callback);

    /**
     * @brief Set the action send goal reply callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_send_action_send_goal_reply_callback(
            std::function<void(const std::string&, const uint64_t, bool accepted)> callback);

    /**
     * @brief Set the action get result reply callback.
     *
     * @param [in] callback Callback to be set.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void set_send_action_get_result_reply_callback(
            std::function<bool(const std::string&, const UUID&, const std::string&, const uint64_t)> callback);

    /**
     * @brief Get a new request ID (incremented by one) for creating a service or action request.
     *
     * @return A new request ID.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    uint64_t get_new_request_id();

protected:

    /**
     * @brief Pop an action request UUID from the internal map that tracks which actions are active.
     *
     * @param [in] request_id Request ID of the action request.
     * @param [in] action_type Type of the action (GOAL, RESULT, CANCEL).
     * @param [out] action_id UUID of the action associated to the request.
     * @return \c true if the action request was successfully popped, \c false otherwise.
     */
    bool get_action_request_UUID(
            const uint64_t request_id,
            const ActionType action_type,
            UUID& action_id);

    /**
     * @brief Get the result (if it has been previouly stored) of an action associated to the given \c action_id.
     *
     * @param [in] action_id UUID of the action.
     * @param [out] result Result of the action.
     * @return \c true if the result was found, \c false otherwise.
     */
    bool get_action_result(
            const UUID& action_id,
            std::string& result);

    /**
     * @brief Add a schema, associated to the given \c dyn_type and \c type_id.
     *
     * @param [in] dyn_type DynamicType containing the type information required to generate the schema.
     * @param [in] type_id TypeIdentifier of the type.
     * @param [in] write_schema Whether to write the schema or not.
     */
    void add_schema_nts_(
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const fastdds::dds::xtypes::TypeIdentifier& type_id,
            bool write_schema = true);

    /**
     * @brief Add a schema, associated to the given \c type_id and \c type_obj.
     *
     * @param [in] type_id TypeIdentifier of the type.
     * @param [in] type_obj TypeObject of the type.
     * @param [in] write_schema Whether to write the schema or not.
     * @return \c true if the schema was added successfully, \c false otherwise.
     */
    bool add_schema_nts_(
            const fastdds::dds::xtypes::TypeIdentifier& type_id,
            const fastdds::dds::xtypes::TypeObject& type_obj,
            bool write_schema = true);

    /**
     * @brief Write the schema to user's app.
     *
     * @param [in] dyn_type DynamicType containing the type information required to generate the schema.
     * @param [in] type_id TypeIdentifier of the type.
     */
    void write_schema_nts_(
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const fastdds::dds::xtypes::TypeIdentifier& type_id);

    /**
     * @brief Write the topic to user's app.
     *
     * @param [in] topic DDS topic to be added.
     */
    void write_topic_nts_(
            const ddspipe::core::types::DdsTopic& topic);

    /**
     * @brief Write the service to user's app.
     *
     * @param [in] service RPC topic associated to this service.
     */
    void write_service_nts_(
            const ddspipe::core::types::RpcTopic& service);

    /**
     * @brief Write the action to user's app.
     *
     * @param [in] action RPC action associated to this action.
     */
    void write_action_nts_(
            const RpcAction& action);
    /**
     * @brief Write to user's app.
     *
     * @param [in] msg Message to be added
     * @param [in] dyn_type DynamicType containing the type information required.
     */
    void write_sample_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type);

    /**
     * @brief Write the service reply to user's app.
     *
     * @param [in] msg Message containing the service reply.
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] request_id Request ID of the service reply.
     */
    void write_service_reply_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const uint64_t request_id,
            const std::string& service_name);

    /**
     * @brief Write the service request to user's app.
     *
     * @param [in] msg Message containing the service request.
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] request_id Request ID of the service request.
     */
    void write_service_request_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const uint64_t request_id,
            const std::string& service_name);

    /**
     * @brief Write the action result to user's app.
     *
     * @param [in] msg Message containing the action goal request.
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] request_id Request ID of the action goal request.
     */
    void write_action_result_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const UUID& action_id,
            const std::string& action_name);

    /**
     * @brief Write the action feedback to user's app.
     *
     * @param [in] msg Message containing the action feedback.
     * @param [in] dyn_type DynamicType containing the type information required.
     */
    void write_action_feedback_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const std::string& action_name);

    /**
     * @brief Write the action goal reply to user's app.
     *
     * @param [in] msg Message containing the action goal reply.
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] action_id UUID of the action.
     */
    void write_action_goal_reply_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const UUID& action_id,
            const std::string& action_name);

    /**
     * @brief Write the action cancel reply to user's app.
     *
     * @param [in] msg Message containing the action cancel reply.
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] request_id Request ID of the action cancel reply.
     */
    void write_action_cancel_reply_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const uint64_t request_id,
            const std::string& action_name);

    /**
     * @brief Write the action status to user's app.
     *
     * @param [in] msg Message containing the action status.
     * @param [in] dyn_type DynamicType containing the type information required.
     */
    void write_action_status_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const std::string& action_name);

    /**
     * @brief Write the action (goal or cancel) request to user's app.
     *
     * @param [in] msg Message containing the action request.
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] request_id Request ID of the action request.
     */
    void write_action_request_nts_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const uint64_t request_id,
            const std::string& action_name,
            const ActionType action_type);

    /**
     * @brief Register a type using the given serialized type data.
     *
     * @param [in] type_name Name of the type to be registered.
     * @param [in] serialized_type Pointer to the serialized type data.
     * @param [in] serialized_type_size Size of the serialized type data.
     * @param [out] type_identifier TypeIdentifier of the registered type.
     * @param [out] type_object TypeObject of the registered type.
     * @return \c true if the type was registered successfully, \c false otherwise.
     */
    bool register_type_nts_(
            const std::string& type_name,
            const unsigned char* serialized_type,
            uint32_t serialized_type_size,
            fastdds::dds::xtypes::TypeIdentifier& type_identifier,
            fastdds::dds::xtypes::TypeObject& type_object);

    //! Handler configuration
    HandlerConfiguration configuration_;

    //! Payload pool
    std::shared_ptr<ddspipe::core::PayloadPool> payload_pool_;

    //! writer
    std::unique_ptr<Writer> writer_;

    //! Schemas map
    std::map<std::string,
            std::pair<fastdds::dds::xtypes::TypeIdentifier, fastdds::dds::DynamicType::_ref_type>> schemas_;

    //! Unique sequence number assigned to received messages. It is incremented with every sample added
    unsigned int unique_sequence_number_{0};

    //! Mutex synchronizing access to object's data structures
    std::recursive_mutex mtx_;

    //! Callback to request types from the user
    DdsTypeQuery type_query_callback_;

    //! Identifier for the received and sent requests
    uint64_t requests_id_{0};

    //! Map of all ActionRequestInfos to their UUIDs
    std::unordered_map<participants::UUID, ActionRequestInfo> action_request_id_to_uuid_;

    //! Lambda to send action get result reply
    std::function<bool(const std::string&, const UUID&, const std::string&,
            const uint64_t)> send_action_get_result_reply_callback_;
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
