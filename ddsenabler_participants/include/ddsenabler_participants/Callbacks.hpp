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

namespace eprosima {
namespace ddsenabler {
namespace participants {

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
 * @param [in] type_name Name of the type associated with the topic
 * @param [in] serialized_qos Serialized Quality of Service (QoS) of the topic
 */
typedef void (* DdsTopicNotification)(
        const char* topic_name,
        const char* type_name,
        const char* serialized_qos);

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
 * @param [out] type_name Name of the type associated with the topic
 * @param [out] serialized_qos Serialized Quality of Service (QoS) of the topic
 * @return \c true if the topic was found and the information was retrieved successfully, \c false otherwise
 */
typedef bool (* DdsTopicQuery)(
        const char* topic_name,
        std::string& type_name,
        std::string& serialized_qos);

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
