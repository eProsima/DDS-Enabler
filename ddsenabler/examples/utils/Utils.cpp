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

/**
 * @file Utils.cpp
 */

#include "Utils.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <iostream>

namespace utils {

std::string safe_file_name(
        const std::string& name)
{
    std::string safe_name = name;
    for (auto& c : safe_name)
    {
        if (c == ':' || c == '/' || c == '\\')
        {
            c = '_';
        }
    }
    return safe_name;
}

bool save_type_to_file(
        const std::string& directory,
        const char* type_name,
        const unsigned char*& serialized_type_internal,
        uint32_t serialized_type_internal_size)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_type_name = safe_file_name(type_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / (safe_type_name + ".bin");

    // Check if already exists, do nothing if it does
    if (std::filesystem::exists(file_path))
    {
        std::cout << "File already exists: " << file_path << std::endl;
        return true;
    }

    // Open file in binary mode
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs)
    {
        std::cerr << "Error opening file for writing: " << file_path << std::endl;
        return false;
    }

    // Write the binary data
    ofs.write(reinterpret_cast<const char*>(serialized_type_internal), serialized_type_internal_size);

    if (!ofs.good())
    {
        std::cerr << "Error writing to file: " << file_path << std::endl;
        return false;
    }

    ofs.close();
    return true;
}

bool load_type_from_file(
        const std::string& directory,
        const char* type_name,
        std::unique_ptr<const unsigned char[]>& serialized_type_internal,
        uint32_t& serialized_type_internal_size)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_type_name = safe_file_name(type_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / (safe_type_name + ".bin");

    // Check if exists
    if (!std::filesystem::exists(file_path))
    {
        std::cerr << "File does not exist: " << file_path << std::endl;
        return false;
    }

    // Open file in binary mode, position at end to get size
    std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
    if (!ifs)
    {
        std::cerr << "Error opening file for reading: " << file_path << std::endl;
        return false;
    }

    // Get file size
    std::streamsize size = ifs.tellg();
    if (size <= 0)
    {
        std::cerr << "File is empty or invalid: " << file_path << std::endl;
        return false;
    }
    serialized_type_internal_size = static_cast<uint32_t>(size);

    // Allocate memory with unique_ptr
    std::unique_ptr<unsigned char[]> temp_buffer(new unsigned char[serialized_type_internal_size]);

    // Read the data
    ifs.seekg(0, std::ios::beg);
    if (!ifs.read(reinterpret_cast<char*>(temp_buffer.get()), serialized_type_internal_size))
    {
        std::cerr << "Error reading file: " << file_path << std::endl;
        serialized_type_internal_size = 0;
        return false;
    }

    // Transfer ownership and cast to const
    serialized_type_internal = std::unique_ptr<const unsigned char[]>(std::move(temp_buffer));

    return true;
}

bool save_topic_to_file(
        const std::string& directory,
        const char* topic_name,
        const char* type_name,
        const char* serialized_qos)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_topic_name = safe_file_name(topic_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / (safe_topic_name + ".bin");

    // Check if already exists, do nothing if it does
    if (std::filesystem::exists(file_path))
    {
        std::cout << "File already exists: " << file_path << std::endl;
        return true;
    }

    // Open file in binary mode
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs)
    {
        std::cerr << "Error opening file for writing: " << file_path << std::endl;
        return false;
    }

    // Write type name
    uint32_t len_type_name = static_cast<uint32_t>(std::strlen(type_name));
    ofs.write(reinterpret_cast<const char*>(&len_type_name), sizeof(len_type_name));
    ofs.write(type_name, len_type_name);

    // Write serialized QoS
    uint32_t len_serialized_qos = static_cast<uint32_t>(std::strlen(serialized_qos));
    ofs.write(reinterpret_cast<const char*>(&len_serialized_qos), sizeof(len_serialized_qos));
    ofs.write(serialized_qos, len_serialized_qos);

    if (!ofs.good())
    {
        std::cerr << "Error writing to file: " << file_path << std::endl;
        return false;
    }

    ofs.close();
    return true;
}

bool load_topic_from_file(
        const std::string& directory,
        const char* topic_name,
        std::string& type_name,
        std::string& serialized_qos)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_topic_name = safe_file_name(topic_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / (safe_topic_name + ".bin");

    // Check if exists
    if (!std::filesystem::exists(file_path))
    {
        std::cerr << "File does not exist: " << file_path << std::endl;
        return false;
    }

    // Open file in binary mode
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs)
    {
        std::cerr << "Error opening file for reading: " << file_path << std::endl;
        return false;
    }

    uint32_t len_type_name = 0, len_serialized_qos = 0;

    // Read type name length
    ifs.read(reinterpret_cast<char*>(&len_type_name), sizeof(len_type_name));
    if (!ifs.good() || len_type_name == 0)
    {
        std::cerr << "Error reading type name length." << std::endl;
        return false;
    }

    // Read type name
    std::vector<char> type_name_buffer(len_type_name);
    ifs.read(type_name_buffer.data(), len_type_name);
    if (!ifs.good())
    {
        std::cerr << "Error reading type name." << std::endl;
        return false;
    }

    // Read serialized QoS length
    ifs.read(reinterpret_cast<char*>(&len_serialized_qos), sizeof(len_serialized_qos));
    if (!ifs.good() || len_serialized_qos == 0)
    {
        std::cerr << "Error reading serialized qos length." << std::endl;
        return false;
    }

    // Read serialized QoS
    std::vector<char> serialized_qos_buffer(len_serialized_qos);
    ifs.read(serialized_qos_buffer.data(), len_serialized_qos);
    if (!ifs.good())
    {
        std::cerr << "Error reading serialized qos." << std::endl;
        return false;
    }

    type_name.assign(type_name_buffer.begin(), type_name_buffer.end());
    serialized_qos.assign(serialized_qos_buffer.begin(), serialized_qos_buffer.end());

    return true;
}

bool save_data_to_file(
        const std::string& directory,
        const std::string& topic_name,
        const std::string& json,
        int64_t publish_time)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_topic_name = safe_file_name(topic_name);

    // Construct topic folder path
    auto topic_path = std::filesystem::path(directory) / safe_topic_name;

    // Create directory if it does not exist
    if (!std::filesystem::exists(topic_path) && !std::filesystem::create_directories(topic_path))
    {
        std::cerr << "Error creating directory: " << topic_path << std::endl;
        return false;
    }

    // Construct full file path
    auto file_path = topic_path / std::to_string(publish_time);

    // Check if exists, fail if it does
    if (std::filesystem::exists(file_path))
    {
        std::cerr << "File already exists: " << file_path << std::endl;
        return false;
    }

    // Open file in text mode
    std::ofstream ofs(file_path);
    if (!ofs)
    {
        std::cerr << "Error opening file for writing: " << file_path << std::endl;
        return false;
    }

    // Check if JSON is valid
    nlohmann::json full_json;
    try
    {
        full_json = nlohmann::json::parse(json);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }

    // Extract the DDS data
    auto it = full_json.find(topic_name);
    if (it == full_json.end())
    {
        std::cerr << "Error: JSON does not contain data for topic: " << topic_name << std::endl;
        return false;
    }
    nlohmann::json& json_data = (*it)["data"];

    if (json_data.empty())
    {
        std::cerr << "Error: No data found for topic: " << topic_name << std::endl;
        return false;
    }

    if (json_data.size() > 1)
    {
        std::cerr << "Warning: Multiple data entries found for topic: " << topic_name
                  << ". Only the first one will be written to file." << std::endl;
    }

    // Write DDS data only
    ofs << std::setw(4) << json_data.begin().value();
    if (!ofs.good())
    {
        std::cerr << "Error writing to file: " << file_path << std::endl;
        return false;
    }

    ofs.close();
    return true;
}

void save_service_to_file(
        const std::string& directory,
        const char* service_name,
        const char* request_type_name,
        const char* reply_type_name,
        const char* request_serialized_qos,
        const char* reply_serialized_qos)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return;
    }

    // Remove problematic characters
    std::string safe_service_name = safe_file_name(service_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / safe_service_name;

    // Check if already exists, do nothing if it does
    if (std::filesystem::exists(file_path))
    {
        std::cout << "File already exists: " << file_path << std::endl;
        return;
    }

    nlohmann::json j;
    j["service_name"] = service_name;
    j["request_type_name"] = request_type_name;
    j["reply_type_name"] = reply_type_name;
    j["request_serialized_qos"] = request_serialized_qos;
    j["reply_serialized_qos"] = reply_serialized_qos;

    std::ofstream ofs(file_path);
    if (ofs.is_open()) {
    ofs << j.dump(4);
    }
}

bool load_service_from_file(
        const std::string& directory,
        const char* service_name,
        std::string& request_type_name,
        std::string& reply_type_name,
        std::string& request_serialized_qos,
        std::string& reply_serialized_qos)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_service_name = safe_file_name(service_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / safe_service_name;

    // Check if already exists, do nothing if it does not
    if (!std::filesystem::exists(file_path))
    {
        std::cout << "File does not exist: " << file_path << std::endl;
        return false;
    }

    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        return false;
    }

    nlohmann::json j;
    ifs >> j;

    std::string file_service_name = j["service_name"].get<std::string>();
    if (file_service_name != std::string(service_name)) {
        return false;  // Service name does not match
    }

    request_type_name = j["request_type_name"].get<std::string>();
    reply_type_name = j["reply_type_name"].get<std::string>();
    request_serialized_qos = j["request_serialized_qos"].get<std::string>();
    reply_serialized_qos = j["reply_serialized_qos"].get<std::string>();
    ifs.close();

    return true;
}

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
    const char* status_action_serialized_qos)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return;
    }

    // Remove problematic characters
    std::string safe_action_name = safe_file_name(action_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / safe_action_name;

    // Check if already exists, do nothing if it does
    if (std::filesystem::exists(file_path))
    {
        std::cout << "File already exists: " << file_path << std::endl;
        return;
    }

    nlohmann::json j;
    j["action_name"] = action_name;
    j["goal_request_action_type"] = goal_request_action_type;
    j["goal_reply_action_type"] = goal_reply_action_type;
    j["cancel_request_action_type"] = cancel_request_action_type;
    j["cancel_reply_action_type"] = cancel_reply_action_type;
    j["result_request_action_type"] = result_request_action_type;
    j["result_reply_action_type"] = result_reply_action_type;
    j["feedback_action_type"] = feedback_action_type;
    j["status_action_type"] = status_action_type;
    j["goal_request_action_serialized_qos"] = goal_request_action_serialized_qos;
    j["goal_reply_action_serialized_qos"] = goal_reply_action_serialized_qos;
    j["cancel_request_action_serialized_qos"] = cancel_request_action_serialized_qos;
    j["cancel_reply_action_serialized_qos"] = cancel_reply_action_serialized_qos;
    j["result_request_action_serialized_qos"] = result_request_action_serialized_qos;
    j["result_reply_action_serialized_qos"] = result_reply_action_serialized_qos;
    j["feedback_action_serialized_qos"] = feedback_action_serialized_qos;
    j["status_action_serialized_qos"] = status_action_serialized_qos;

    std::ofstream ofs(file_path);
    if (ofs.is_open()) {
        ofs << j.dump(4);
        ofs.close();
    }
}

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
    std::string& status_action_serialized_qos)
{
    // Check if directory exists
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Remove problematic characters
    std::string safe_action_name = safe_file_name(action_name);

    // Construct full file path
    auto file_path = std::filesystem::path(directory) / safe_action_name;

    // Check if already exists, do nothing if it does not
    if (!std::filesystem::exists(file_path))
    {
        std::cout << "File does not exist: " << file_path << std::endl;
        return false;
    }

    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        return false;
    }

    nlohmann::json j;
    ifs >> j;

    std::string file_action_name = j["action_name"].get<std::string>();
    if (file_action_name != std::string(action_name)) {
        return false;  // Action name does not match
    }
    goal_request_action_type = j["goal_request_action_type"].get<std::string>();
    goal_reply_action_type = j["goal_reply_action_type"].get<std::string>();
    cancel_request_action_type = j["cancel_request_action_type"].get<std::string>();
    cancel_reply_action_type = j["cancel_reply_action_type"].get<std::string>();
    result_request_action_type = j["result_request_action_type"].get<std::string>();
    result_reply_action_type = j["result_reply_action_type"].get<std::string>();
    feedback_action_type = j["feedback_action_type"].get<std::string>();
    status_action_type = j["status_action_type"].get<std::string>();
    goal_request_action_serialized_qos = j["goal_request_action_serialized_qos"].get<std::string>();
    goal_reply_action_serialized_qos = j["goal_reply_action_serialized_qos"].get<std::string>();
    cancel_request_action_serialized_qos = j["cancel_request_action_serialized_qos"].get<std::string>();
    cancel_reply_action_serialized_qos = j["cancel_reply_action_serialized_qos"].get<std::string>();
    result_request_action_serialized_qos = j["result_request_action_serialized_qos"].get<std::string>();
    result_reply_action_serialized_qos = j["result_reply_action_serialized_qos"].get<std::string>();
    feedback_action_serialized_qos = j["feedback_action_serialized_qos"].get<std::string>();
    status_action_serialized_qos = j["status_action_serialized_qos"].get<std::string>();
    ifs.close();

    return true;
}

void init_persistence(
        const std::string& persistence_path,
        std::vector<std::string> subdirs)
{
    auto ensure_directory_exists = [](const std::filesystem::path& path)
            {
                if (!std::filesystem::exists(path) && !std::filesystem::create_directories(path))
                {
                    std::cerr << "Failed to create directory: " << path << std::endl;
                }
            };

    if (!persistence_path.empty())
    {
        ensure_directory_exists(persistence_path);
        for (const auto& sub : subdirs)
        {
            ensure_directory_exists(std::filesystem::path(persistence_path) / sub);
        }
    }
}

void get_sorted_files(
        const std::string& directory,
        std::vector<std::pair<std::filesystem::path, int32_t>>& files)
{
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            try
            {
                // assumes name is just a number
                files.emplace_back(entry.path(), static_cast<int32_t>(std::stoll(filename)));
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Skipping non-numeric file: " << filename << std::endl;
            }
        }
    }

    // Sort files by numeric value
    std::sort(files.begin(), files.end(),
            [](const auto& a, const auto& b)
            {
                return a.second < b.second;
            });
}


} // namespace utils