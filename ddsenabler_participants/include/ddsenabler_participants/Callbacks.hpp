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
 * @file Callbacks.hpp
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ddsenabler_participants/rpc/RpcTypes.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {


/*********************/
/*        DDS        */
/*********************/

/**
 * @brief Struct that contains information about a DDS topic.
 *
 * This struct is used to encapsulate the type name and serialized QoS of a DDS topic.
 */
struct TopicInfo
{
    TopicInfo() = default;

    TopicInfo (
            const std::string& _type_name,
            const std::string& _serialized_qos)
        : type_name(_type_name)
        , serialized_qos(_serialized_qos)
    {
    }

    std::string type_name;
    std::string serialized_qos;
};

/**
 * DdsLogFunc - callback executed when consuming log messages
 *
 * @param [in] file_name Name of the file where the log was generated
 * @param [in] line_no Line number in the file where the log was generated
 * @param [in] func_name Name of the function where the log was generated
 * @param [in] category Category of the log message
 * @param [in] msg Log message content
 */
typedef void (* DdsLogFunc)(
        const char* file_name,
        int line_no,
        const char* func_name,
        int category,
        const char* msg);

/**
 * DdsTypeNotification - callback for notifying the reception of DDS types
 *
 * @param [in] type_name Name of the received type
 * @param [in] serialized_type Serialized type in IDL format
 * @param [in] serialized_type_internal Serialized type in internal format
 * @param [in] serialized_type_internal_size Size of the serialized type in internal format
 * @param [in] data_placeholder JSON data placeholder
 */
typedef void (* DdsTypeNotification)(
        const char* type_name,
        const char* serialized_type,
        const unsigned char* serialized_type_internal,
        uint32_t serialized_type_internal_size,
        const char* data_placeholder);

/**
 * DdsTopicNotification - callback for notifying the reception of DDS topics
 *
 * @param [in] topic_name Name of the received topic
 * @param [in] topic_info Information about the topic, including type name and serialized QoS
 */
typedef void (* DdsTopicNotification)(
        const char* topic_name,
        const TopicInfo& topic_info);

/**
 * DdsDataNotification - callback for notifying the reception of DDS data
 *
 * @param [in] topic_name Name of the topic from which the data was received
 * @param [in] json JSON representation of the data
 * @param [in] publish_time Time (nanoseconds since epoch) when the data was published
 */
typedef void (* DdsDataNotification)(
        const char* topic_name,
        const char* json,
        int64_t publish_time);

/**
 * DdsTypeQuery - callback for requesting information (serialized description and size) of a DDS type
 *
 * @param [in] type_name Name of the type to query
 * @param [out] serialized_type_internal Pointer to the serialized type in internal format
 * @param [out] serialized_type_internal_size Size of the serialized type in internal format
 * @return \c true if the type was found and the information was retrieved successfully, \c false otherwise
 */
typedef bool (* DdsTypeQuery)(
        const char* type_name,
        std::unique_ptr<const unsigned char []>& serialized_type_internal,
        uint32_t& serialized_type_internal_size);

/**
 * DdsTopicQuery - callback for requesting information (type and QoS) of a DDS topic
 *
 * @param [in] topic_name Name of the topic to query
 * @param [out] topic_info Information about the topic, including type name and serialized QoS
 * @return \c true if the topic was found and the information was retrieved successfully, \c false otherwise
 */
typedef bool (* DdsTopicQuery)(
        const char* topic_name,
        TopicInfo& topic_info);


/**********************/
/*      SERVICES      */
/**********************/

/**
 * @brief Struct that contains information about a DDS service.
 *
 * This struct is used to encapsulate the request and reply topics of a DDS service.
 */
struct ServiceInfo
{
    ServiceInfo() = default;

    ServiceInfo(
            const TopicInfo& _request,
            const TopicInfo& _reply)
        : request(_request)
        , reply(_reply)
    {
    }

    TopicInfo request;
    TopicInfo reply;
};

/**
 * @brief Callback for notification of service discovery and its request and reply types.
 *
 * This callback is used to notify the discovery of a service and its associated request and reply types.
 *
 * @param [in] service_name The name of the service that was discovered.
 * @param [in] service_info Information about the service, including request and reply types and their serialized QoS.
 */
typedef void (* ServiceNotification)(
        const char* service_name,
        const ServiceInfo& service_info);

/**
 * @brief Callback for reception of service request data.
 *
 * This callback is used to notify the reception of a request for a specific service.
 *
 * @param [in] service_name The name of the service for which the request was received.
 * @param [in] json The JSON data received in the request.
 * @param [in] request_id The unique identifier of the request.
 * @param [in] publish_time The time at which the request was published.
 *
 * @note The request_id is unique for each request and must be later used to identify the reply.
 */
typedef void (* ServiceRequestNotification)(
        const char* service_name,
        const char* json,
        uint64_t request_id,
        int64_t publish_time);

/**
 * @brief Callback for reception of RPC reply data.
 *
 * This callback is used to notify the reception of a reply for a specific service.
 *
 * @param [in] service_name The name of the service for which the reply was received.
 * @param [in] json The JSON data received in the reply.
 * @param [in] request_id The unique identifier of the request for which this is a reply.
 * @param [in] publish_time The time at which the reply was published.
 */
typedef void (* ServiceReplyNotification)(
        const char* service_name,
        const char* json,
        uint64_t request_id,
        int64_t publish_time);

/**
 * @brief Callback requesting the information of a given service's request and reply.
 *
 * This callback is used to request the information for a service's request and reply.
 *
 * @param [in] service_name The name of the service for which the information is requested.
 * @param [out] service_info Information about the service, including request and reply types and their serialized QoS.
 * @return \c true if the service was found and the information was retrieved successfully, \c false otherwise.
 */
typedef bool (* ServiceQuery)(
        const char* service_name,
        ServiceInfo& service_info);


/**********************/
/*      ACTIONS       */
/**********************/

/**
 * @brief Struct that contains information about a DDS action.
 *
 * This struct is used to encapsulate the goal, result and cancel services as well as the feedback and status topics of a DDS action.
 */
struct ActionInfo
{
    ActionInfo() = default;

    ActionInfo(
            const ServiceInfo& _goal,
            const ServiceInfo& _result,
            const ServiceInfo& _cancel,
            const TopicInfo& _feedback,
            const TopicInfo& _status)
        : goal(_goal)
        , result(_result)
        , cancel(_cancel)
        , feedback(_feedback)
        , status(_status)
    {
    }

    ServiceInfo goal;
    ServiceInfo result;
    ServiceInfo cancel;
    TopicInfo feedback;
    TopicInfo status;
};

/**
 * @brief Callback for notification of action discovery and its associated types.
 *
 * This callback is used to notify the discovery of an action and its associated types.
 *
 * @param [in] action_name The name of the action that was discovered.
 * @param [in] action_info Information about the action, including goal, result, cancel, feedback, and status types and their serialized QoS.
 */
typedef void (* ActionNotification)(
        const char* action_name,
        const ActionInfo& action_info);

/**
 * @brief Callback for notification of an action goal request.
 *
 * This callback is used to notify the request of an action goal.
 *
 * @param [in] action_name The name of the action for which the goal is being requested.
 * @param [in] json The JSON data representing the goal request.
 * @param [in] goal_id The unique identifier of the goal associated with the action.
 * @param [in] publish_time The time at which the goal request was published.
 * @return \c true if the goal request has been accepted, \c false otherwise.
 */
typedef bool (* ActionGoalRequestNotification)(
        const char* action_name,
        const char* json,
        const UUID& goal_id,
        int64_t publish_time);

/**
 * @brief Callback for notification of an action cancel request.
 *
 * This callback is used to notify the request to cancel an action goal:
 *
 * If the goal ID is empty and timestamp is zero, cancel all goals
 * If the goal ID is empty and timestamp is not zero, cancel all goals accepted at or before the timestamp
 * If the goal ID is not empty and timestamp is zero, cancel the goal with the given ID regardless of the time it was accepted
 * If the goal ID is not empty and timestamp is not zero, cancel the goal with the given ID and all goals accepted at or before the timestamp
 *
 * @param [in] action_name The name of the action for which the cancel request is being made.
 * @param [in] goal_id The unique identifier of the goal associated with the action.
 * @param [in] timestamp The timestamp used for discriminating which goals to cancel.
 * @param [in] request_id The identifier of the cancel request used for dicriminating which goals to cancel.
 * @param [in] publish_time The time at which the cancel request was published.
 */
typedef void (* ActionCancelRequestNotification)(
        const char* action_name,
        const UUID& goal_id,
        int64_t timestamp,
        uint64_t request_id,
        int64_t publish_time);

/**
 * @brief Callback for notification of action feedback.
 *
 * This callback is used to notify the feedback of an action.
 *
 * @param [in] action_name The name of the action for which the feedback is being notified.
 * @param [in] json The JSON data representing the feedback of the action.
 * @param [in] goal_id The unique identifier of the goal associated with the action.
 * @param [in] publish_time The time at which the feedback was published.
 */
typedef void (* ActionFeedbackNotification)(
        const char* action_name,
        const char* json,
        const UUID& goal_id,
        int64_t publish_time);

/**
 * @brief Callback for notification of an update for an action status.
 *
 * This callback is used to notify the update of the status of an action.
 *
 * @param [in] action_name The name of the action for which the status is being notified.
 * @param [in] goal_id The unique identifier of the goal associated with the action.
 * @param [in] status_code The status code representing the current state of the action.
 * @param [in] status_message A message providing additional information about the status.
 * @param [in] publish_time The time at which the status was published.
 */
typedef void (* ActionStatusNotification)(
        const char* action_name,
        const UUID& goal_id,
        StatusCode status_code,
        const char* status_message,
        int64_t publish_time);

/**
 * @brief Callback for notification of action result.
 *
 * This callback is used to notify the result of an action in case of success.
 *
 * @param [in] action_name The name of the action for which the result is being notified.
 * @param [in] json The JSON data representing the result of the action.
 * @param [in] goal_id The unique identifier of the goal associated with the action.
 * @param [in] publish_time The time at which the result was published.
 */
typedef void (* ActionResultNotification)(
        const char* action_name,
        const char* json,
        const UUID& goal_id,
        int64_t publish_time);

/**
 * @brief Callback for requesting action's information.
 *
 * This callback is used to request information about a specific action.
 *
 * @param [in] action_name The name of the action for which the information is being requested.
 * @param [out] action_info Information about the action, including goal, result, cancel, feedback, and status types and their serialized QoS.
 * @return \c true if the action was found and the information was retrieved successfully, \c false otherwise.
 */
typedef bool (* ActionQuery)(
        const char* action_name,
        ActionInfo& action_info);

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
