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

// TODO: move these methods somewhere they can be reused

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
