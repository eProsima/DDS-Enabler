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

#include <ddsenabler_participants/Callbacks.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma once

namespace utils {

using eprosima::ddsenabler::participants::TopicInfo;
using eprosima::ddsenabler::participants::ServiceInfo;
using eprosima::ddsenabler::participants::ActionInfo;

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
        const TopicInfo& topic_info);

bool load_topic_from_file(
        const std::string& directory,
        const char* topic_name,
        TopicInfo& topic_info);

bool save_data_to_file(
        const std::string& directory,
        const std::string& topic_name,
        const std::string& json,
        int64_t publish_time);

void save_service_to_file(
        const std::string& directory,
        const char* service_name,
        const ServiceInfo& service_info);

bool load_service_from_file(
        const std::string& directory,
        const char* service_name,
        ServiceInfo& service_info);

void save_action_to_file(
        const std::string& directory,
        const char* action_name,
        const ActionInfo& action_info);

bool load_action_from_file(
        const std::string& directory,
        const char* action_name,
        ActionInfo& action_info);

void init_persistence(
        const std::string& persistence_path,
        std::vector<std::string> subdirs);

void get_sorted_files(
        const std::string& directory,
        std::vector<std::pair<std::filesystem::path, int32_t>>& files);

} // namespace utils
