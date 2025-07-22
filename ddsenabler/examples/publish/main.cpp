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
 * @file main.cpp
 *
 */

#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <Utils.hpp>

#include "ddsenabler/dds_enabler_runner.hpp"
#include "ddsenabler/DDSEnabler.hpp"

#include "CLIParser.hpp"

CLIParser::example_config config;
uint32_t received_types_ = 0;
uint32_t received_topics_ = 0;
uint32_t received_data_ = 0;
std::mutex app_mutex_;
std::condition_variable app_cv_;
bool stop_app_ = false;

const std::string SAMPLES_SUBDIR = "samples";
const std::string TYPES_SUBDIR = "types";
const std::string TOPICS_SUBDIR = "topics";

// Static log callback
void test_log_callback(
        const char* file_name,
        int line_no,
        const char* func_name,
        int category,
        const char* msg)
{
    // NOTE: Default stdout logs can be disabled via configuration ("logging": {"stdout": false}) to avoid duplicated traces

    std::stringstream ss;
    ss << file_name << ":" << line_no << " (" << func_name << "): " << msg << std::endl;
    if (category == eprosima::utils::Log::Kind::Info)
    {
        std::cout << "[INFO] " << ss.str();
    }
    else if (category == eprosima::utils::Log::Kind::Warning)
    {
        std::cerr << "[WARNING] " << ss.str();
    }
    else if (category == eprosima::utils::Log::Kind::Error)
    {
        std::cerr << "[ERROR] " << ss.str();
    }
}

// Static type notification callback
static void test_type_notification_callback(
        const char* type_name,
        const char* serialized_type,
        const unsigned char* serialized_type_internal,
        uint32_t serialized_type_internal_size,
        const char* data_placeholder)
{
    bool notify = false;
    {
        std::lock_guard<std::mutex> lock(app_mutex_);
        notify = ++received_types_ >= config.expected_types;
        std::cout << "Type callback received: " << type_name << ", Total types: " <<
            received_types_ << std::endl << serialized_type << std::endl << std::endl;
        if (!config.persistence_path.empty() &&
                !utils::save_type_to_file((std::filesystem::path(config.persistence_path) / TYPES_SUBDIR).string(), type_name,
                serialized_type_internal, serialized_type_internal_size))
        {
            std::cerr << "Failed to save type: " << type_name << std::endl;
        }
    }
    if (notify)
    {
        app_cv_.notify_all();
    }
}

// Static type query callback
static bool test_type_query_callback(
        const char* type_name,
        std::unique_ptr<const unsigned char[]>& serialized_type_internal,
        uint32_t& serialized_type_internal_size)
{
    if (config.persistence_path.empty())
    {
        std::cerr << "Persistence path is not set, cannot query type: " << type_name << std::endl;
        return false;
    }

    // Load the type from file
    if (!utils::load_type_from_file((std::filesystem::path(config.persistence_path) / TYPES_SUBDIR).string(), type_name,
            serialized_type_internal, serialized_type_internal_size))
    {
        std::cerr << "Failed to load type: " << type_name << std::endl;
        return false;
    }
    return true;
}

// Static topic notification callback
static void test_topic_notification_callback(
        const char* topic_name,
        const char* type_name,
        const char* serialized_qos)
{
    bool notify = false;
    {
        std::lock_guard<std::mutex> lock(app_mutex_);
        notify = ++received_topics_ >= config.expected_topics;
        std::cout << "Topic callback received: " << topic_name << " of type " << type_name << ", Total topics: " <<
            received_topics_ << std::endl << serialized_qos << std::endl << std::endl;
        if (!config.persistence_path.empty() &&
                !utils::save_topic_to_file((std::filesystem::path(config.persistence_path) / TOPICS_SUBDIR).string(),
                topic_name,
                type_name, serialized_qos))
        {
            std::cerr << "Failed to save topic: " << topic_name << std::endl;
        }
    }
    if (notify)
    {
        app_cv_.notify_all();
    }
}

// Static type query callback
static bool test_topic_query_callback(
        const char* topic_name,
        std::string& type_name,
        std::string& serialized_qos)
{
    if (config.persistence_path.empty())
    {
        std::cerr << "Persistence path is not set, cannot query topic: " << topic_name << std::endl;
        return false;
    }

    // Load the topic from file
    if (!utils::load_topic_from_file((std::filesystem::path(config.persistence_path) / TOPICS_SUBDIR).string(), topic_name,
            type_name,
            serialized_qos))
    {
        std::cerr << "Failed to load topic: " << topic_name << std::endl;
        return false;
    }
    return true;
}

// Static data notification callback
static void test_data_notification_callback(
        const char* topic_name,
        const char* json,
        int64_t publish_time)
{
    bool notify = false;
    {
        std::lock_guard<std::mutex> lock(app_mutex_);
        notify = ++received_data_ >= config.expected_data;
        std::cout << "Data callback received: " << topic_name << ", Total data: " <<
            received_data_ << std::endl << json << std::endl << std::endl;
        if (!config.persistence_path.empty() &&
                !utils::save_data_to_file((std::filesystem::path(config.persistence_path) / SAMPLES_SUBDIR).string(),
                topic_name, json,
                publish_time))
        {
            std::cerr << "Failed to save data for topic: " << topic_name << std::endl;
        }
    }
    if (notify)
    {
        app_cv_.notify_all();
    }
}

bool validate_received(
        const CLIParser::example_config& config)
{
    return (received_types_ >= config.expected_types) &&
           (received_topics_ >= config.expected_topics) &&
           (received_data_ >= config.expected_data);
}

bool expected_received(
        const CLIParser::example_config& config)
{
    if (!config.expected_types && !config.expected_topics && !config.expected_data)
    {
        // No expectations set, return false to avoid immediate exit
        return false;
    }
    return validate_received(config);
}

void init_persistence(
        const std::string& persistence_path)
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
        std::vector<std::string> subdirs = {SAMPLES_SUBDIR, TYPES_SUBDIR, TOPICS_SUBDIR};
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

void publish_routine(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& publish_path,
        const std::string& topic_name,
        uint32_t publish_period,
        uint32_t publish_initial_wait,
        std::mutex& app_mutex,
        std::condition_variable& app_cv,
        bool& stop_app)
{
    // Wait a bit before starting to publish so types and topics can be discovered
    std::this_thread::sleep_for(std::chrono::milliseconds(publish_initial_wait));

    // Get collection of files to publish, sorted in increasing order by their name (assumed to be numeric)
    std::vector<std::pair<std::filesystem::path, int32_t>> sample_files;
    get_sorted_files(publish_path, sample_files);

    for (const auto& [path, number] : sample_files)
    {
        std::ifstream file(path, std::ios::binary);
        if (file)
        {
            std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            if (enabler->publish(topic_name, file_content))
            {
                std::cout << "Published content from file: " << path.filename() << " in topic: "
                          << topic_name << std::endl;
            }
            else
            {
                std::cerr << "Failed to publish content from file: " << path.filename() << " in topic: "
                          << topic_name << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to open file: " << path << std::endl;
        }

        // Wait publish period or until stop signal is received
        std::unique_lock<std::mutex> lock(app_mutex);
        if (app_cv.wait_for(lock, std::chrono::milliseconds(publish_period),
                [&stop_app]
                {
                    return stop_app;
                }))
        {
            std::cout << "Publish routine stopped." << std::endl;
            return;
        }
    }
}

void signal_handler(
        int signum)
{
    std::cout << "Signal " << CLIParser::parse_signal(signum) << " received, stopping..." << std::endl;
    {
        std::lock_guard<std::mutex> lock(app_mutex_);
        stop_app_ = true;
    }
    app_cv_.notify_all();
}

int main(
        int argc,
        char** argv)
{
    using namespace eprosima::ddsenabler;

    eprosima::utils::Log::ReportFilenames(true);

    config = CLIParser::parse_cli_options(argc, argv);

    init_persistence(config.persistence_path);

    CallbackSet callbacks{
        test_log_callback,
        {
            test_type_notification_callback,
            test_topic_notification_callback,
            test_data_notification_callback,
            test_type_query_callback,
            test_topic_query_callback
        }
    };

    std::shared_ptr<DDSEnabler> enabler;
    if (!create_dds_enabler(config.config_file_path.c_str(), callbacks, enabler))
    {
        std::cerr << "Failed to create DDS Enabler with the provided configuration." << std::endl;
        return EXIT_FAILURE;
    }

    std::thread publish_thread;
    if (!config.publish_path.empty())
    {
        publish_thread = std::thread(publish_routine,
                        enabler, config.publish_path, config.publish_topic, config.publish_period,
                        config.publish_initial_wait, std::ref(
                            app_mutex_), std::ref(app_cv_), std::ref(stop_app_));
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifndef _WIN32
    signal(SIGQUIT, signal_handler);
    signal(SIGHUP, signal_handler);
#endif // _WIN32

    int ret_code = EXIT_SUCCESS;
    {
        std::unique_lock<std::mutex> lock(app_mutex_);
        if (app_cv_.wait_for(lock, std::chrono::seconds(config.timeout),
                []
                {
                    return stop_app_ || expected_received(config);
                }))
        {
            ret_code = EXIT_SUCCESS;
        }
        else
        {
            std::cerr << "Timeout reached, stopping..." << std::endl;
            ret_code = validate_received(config) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    if (publish_thread.joinable())
    {
        publish_thread.join();
    }
    return ret_code;
}
