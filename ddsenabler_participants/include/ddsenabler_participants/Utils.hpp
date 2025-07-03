// Copyright 2025 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#pragma once

namespace eprosima {
namespace ddsenabler {
namespace participants {
namespace utils {

std::string safe_file_name(
        const std::string& name);

bool save_type_to_file(
        const std::string& directory,
        const char* type_name,
        const unsigned char*& serialized_type_internal,
        uint32_t serialized_type_internal_size);

bool load_type_from_file(
        const std::string& directory,
        const char* type_name,
        std::unique_ptr<const unsigned char[]>& serialized_type_internal,
        uint32_t& serialized_type_internal_size);

bool save_topic_to_file(
        const std::string& directory,
        const char* topic_name,
        const char* type_name,
        const char* serialized_qos);

bool load_topic_from_file(
        const std::string& directory,
        const char* topic_name,
        std::string& type_name,
        std::string& serialized_qos);

bool save_data_to_file(
        const std::string& directory,
        const std::string& topic_name,
        const std::string& json,
        int64_t publish_time);

void save_service_to_file(
        const std::string& directory,
        const char* service_name,
        const char* request_type_name,
        const char* reply_type_name,
        const char* request_serialized_qos,
        const char* reply_serialized_qos);

bool load_service_from_file(
        const std::string& directory,
        const char* service_name,
        std::string& request_type_name,
        std::string& reply_type_name,
        std::string& request_serialized_qos,
        std::string& reply_serialized_qos);

void save_action_to_file(
        const std::string& directory,
        const char* action_name,
        const char* goal_request_action_type,
        const char* goal_reply_action_type,
        const char* cancel_request_action_type,
        const char* cancel_reply_action_type,
        const char* result_request_action_type,
        const char* result_reply_action_type,
        const char* feedback_action_type,
        const char* status_action_type,
        const char* goal_request_action_serialized_qos,
        const char* goal_reply_action_serialized_qos,
        const char* cancel_request_action_serialized_qos,
        const char* cancel_reply_action_serialized_qos,
        const char* result_request_action_serialized_qos,
        const char* result_reply_action_serialized_qos,
        const char* feedback_action_serialized_qos,
        const char* status_action_serialized_qos);

bool load_action_from_file(
        const std::string& directory,
        const char* action_name,
        std::string& goal_request_action_type,
        std::string& goal_reply_action_type,
        std::string& cancel_request_action_type,
        std::string& cancel_reply_action_type,
        std::string& result_request_action_type,
        std::string& result_reply_action_type,
        std::string& feedback_action_type,
        std::string& status_action_type,
        std::string& goal_request_action_serialized_qos,
        std::string& goal_reply_action_serialized_qos,
        std::string& cancel_request_action_serialized_qos,
        std::string& cancel_reply_action_serialized_qos,
        std::string& result_request_action_serialized_qos,
        std::string& result_reply_action_serialized_qos,
        std::string& feedback_action_serialized_qos,
        std::string& status_action_serialized_qos);

} // namespace utils
} // namespace participants
} // namespace ddsenabler
} // namespace eprosima