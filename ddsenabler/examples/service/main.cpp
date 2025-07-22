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
bool service_discovered_ = false;
uint32_t received_replies_ = 0;
std::vector<std::pair<uint64_t, std::string>> received_requests_;
std::mutex app_mutex_;
std::condition_variable app_cv_;
bool stop_app_ = false;

const std::string REQUESTS_SUBDIR = "requests";
const std::string TYPES_SUBDIR = "types";
const std::string SERVICES_SUBDIR = "services";

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
    if (category == eprosima::utils::Log::Kind::Warning)
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
    std::lock_guard<std::mutex> lock(app_mutex_);
    std::cout << "Type callback received: " << type_name << std::endl;
    if (!config.persistence_path.empty() &&
            !utils::save_type_to_file((std::filesystem::path(config.persistence_path) / TYPES_SUBDIR).string(), type_name,
            serialized_type_internal, serialized_type_internal_size))
    {
        std::cerr << "Failed to save type: " << type_name << std::endl;
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

// Static data notification callback
static void test_data_notification_callback(
        const char* topic_name,
        const char* json,
        int64_t publish_time)
{
}

// Static topic notification callback
static void test_topic_notification_callback(
        const char* topic_name,
        const char* type_name,
        const char* serialized_qos)
{
}

// Static type query callback
static bool test_topic_query_callback(
        const char* topic_name,
        std::string& type_name,
        std::string& serialized_qos)
{
    return false;
}

// Static service notification callback
static void test_service_notification_callback(
        const char* service_name,
        const char* request_type_name,
        const char* reply_type_name,
        const char* request_serialized_qos,
        const char* reply_serialized_qos)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    if (config.service_name == std::string(service_name))
    {
        std::cout << "Service callback received: " << service_name << std::endl;

        std::string service_file = (std::filesystem::path(
                config.persistence_path) / SERVICES_SUBDIR
                ).string();
        if (!config.persistence_path.empty())
        {
            utils::save_service_to_file(
                service_file,
                service_name,
                request_type_name,
                reply_type_name,
                request_serialized_qos,
                reply_serialized_qos);

            service_discovered_ = true;
            app_cv_.notify_all();
        }
    }
    else
    {
        std::cout << "Ignoring service callback for: " << service_name << std::endl;
    }
}

// Static service query callback
static bool test_service_query_callback(
        const char* service_name,
        std::string& request_type_name,
        std::string& request_serialized_qos,
        std::string& reply_type_name,
        std::string& reply_serialized_qos)
{
    app_cv_.notify_all();

    std::cout << "Service type request callback received: " << service_name << std::endl;

    std::string service_file = (std::filesystem::path(
            config.persistence_path) /
            SERVICES_SUBDIR
            ).string();
    if (utils::load_service_from_file(
                service_file,
                service_name,
                request_type_name,
                reply_type_name,
                request_serialized_qos,
                reply_serialized_qos))
    {
        return true;
    }
    std::cout << "ERROR Tester: fail to load service from file: " << service_file << std::endl;
    return false;
}

// Static service reply notification callback
static void test_service_reply_notification_callback(
        const char* service_name,
        const char* json,
        uint64_t request_id,
        int64_t publish_time)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    received_replies_++;
    std::cout << "Reply callback received with id: " << request_id << " for service: " << service_name << std::endl;
    std::cout << "Reply: " << json << std::endl;

    app_cv_.notify_all();
}

// Static service request notification callback
static void test_service_request_notification_callback(
        const char* service_name,
        const char* json,
        uint64_t request_id,
        int64_t publish_time)
{
    std::lock_guard<std::mutex> lock(app_mutex_);

    std::cout << "Request callback received with id: " << request_id << " for service: " << service_name << std::endl;
    received_requests_.emplace_back(request_id, json);
    app_cv_.notify_all();
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
        std::vector<std::string> subdirs = {TYPES_SUBDIR, SERVICES_SUBDIR};
        if (config.announce_server)
        {
            subdirs.push_back(REQUESTS_SUBDIR);
        }
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

bool wait_for_service_discovery(
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv)
{
    std::unique_lock<std::mutex> lock(app_mutex);
    if (!app_cv.wait_for(lock, std::chrono::seconds(timeout),
            []()
            {
                return service_discovered_;
            }))
    {
        std::cerr << "Timeout waiting for service discovery." << std::endl;
        return false;
    }
    return true;
}

bool wait_for_service_request(
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv)
{
    std::unique_lock<std::mutex> lock(app_mutex);
    if (!app_cv.wait_for(lock, std::chrono::seconds(timeout),
            []()
            {
                return !received_requests_.empty();
            }))
    {
        std::cerr << "Timeout waiting for service request." << std::endl;
        return false;
    }
    return true;
}

bool wait_for_service_reply(
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv,
        uint32_t sent_requests)
{
    std::unique_lock<std::mutex> lock(app_mutex);
    if (!app_cv.wait_for(lock, std::chrono::seconds(timeout),
            [&sent_requests]()
            {
                return received_replies_ >= sent_requests;
            }))
    {
        std::cerr << "Timeout waiting for service reply." << std::endl;
        return false;
    }
    return true;
}


bool client_routine(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& service_name,
        const std::string& request_path,
        uint32_t timeout,
        uint32_t request_initial_wait,
        std::mutex& app_mutex,
        std::condition_variable& app_cv,
        bool& stop_app)
{
    // Wait for service to be discovered
    if (!wait_for_service_discovery(timeout, app_mutex, app_cv))
    {
        std::cerr << "Failed to discover service: " << service_name << std::endl;
        return false;
    }

    // Wait a bit before starting to publish so types and topics can be discovered
    std::this_thread::sleep_for(std::chrono::seconds(request_initial_wait));

    // Get collection of files to publish, sorted in increasing order by their name (assumed to be numeric)
    std::vector<std::pair<std::filesystem::path, int32_t>> sample_files;
    get_sorted_files(request_path, sample_files);
    uint32_t sent_requests = 0;
    for (const auto& [path, number] : sample_files)
    {
        std::ifstream file(path, std::ios::binary);
        if (file)
        {
            std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            uint64_t request_id = 0;
            if (enabler->send_service_request(service_name, file_content, request_id))
            {
                std::cout << "Published content from file: " << path.filename() << " in service: "
                          << service_name << " with request ID: " << request_id << std::endl;
                sent_requests++;
            }
            else
            {
                std::cerr << "Failed to publish content from file: " << path.filename() << " in service: "
                          << service_name << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to open file: " << path << std::endl;
            return false;
        }

        // Wait publish period or until stop signal is received
        if (!wait_for_service_reply(timeout, app_mutex, app_cv, sent_requests))
        {
            std::cerr << "Failed to receive service reply." << std::endl;
            return false;
        }
    }
    return true;
}

bool server_specific_logic(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& service_name,
        const std::string& request,
        uint64_t request_id)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate processing time
    std::string json = "{\"sum\": 3}"; // Example response, replace with actual logic
    if (!enabler->send_service_reply(service_name, json, request_id))
    {
        std::cerr << "Failed to send service reply for request ID: " << request_id << std::endl;
        return false;
    }
    return true;
}

bool server_routine(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& service_name,
        uint32_t expected_requests,
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv,
        bool& stop_app)
{
    // Announce service
    if (!enabler->announce_service(service_name))
    {
        std::cerr << "Failed to announce service: " << service_name << std::endl;
        return false;
    }

    std::cout << "Service announced: " << service_name << std::endl;

    while (true)
    {
        if (!wait_for_service_request(timeout, app_mutex, app_cv))
        {
            std::cerr << "Timeout waiting for service request." << std::endl;
            return false;
        }

        // Pop the last request from the received requests vector
        uint64_t request_id = received_requests_.back().first;
        std::string request = received_requests_.back().second;
        received_requests_.pop_back();
        std::cout << "Received request for service: " << service_name << " with request ID: " << request_id << std::endl;

        // Example response
        server_specific_logic(
            enabler,
            service_name,
            request,
            request_id);

        // Check if we have received the expected number of requests
        {
            std::lock_guard<std::mutex> lock(app_mutex);
            if (++received_replies_ >= expected_requests)
            {
                return true;
            }
        }
    }

    // Revoke service
    if (!enabler->revoke_service(service_name))
    {
        std::cerr << "Failed to revoke service: " << service_name << std::endl;
        return false;
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

    // Parse CLI options
    config = CLIParser::parse_cli_options(argc, argv);

    // Initialize persistence if required
    init_persistence(config.persistence_path);

    // Set up callbacks
    CallbackSet callbacks{
        test_log_callback,
        {
            test_type_notification_callback,
            test_topic_notification_callback,
            test_data_notification_callback,
            test_type_query_callback,
            test_topic_query_callback
        },
        {
            test_service_notification_callback,
            test_service_request_notification_callback,
            test_service_reply_notification_callback,
            test_service_query_callback
        }
    };

    std::shared_ptr<DDSEnabler> enabler;
    if (!create_dds_enabler(config.config_file_path.c_str(), callbacks, enabler))
    {
        std::cerr << "Failed to create DDSEnabler instance." << std::endl;
        return EXIT_FAILURE;
    }

    bool ret = false;
    // Service logic based on announce_server flag
    if (config.announce_server)
    {
        ret = server_routine(enabler, config.service_name, config.expected_requests, config.timeout, app_mutex_, app_cv_, stop_app_);
    }
    else
    {
        auto request_path = config.persistence_path.empty() ? std::string() : (std::filesystem::path(config.persistence_path) / REQUESTS_SUBDIR).string();
        ret = client_routine(enabler, config.service_name, request_path, config.timeout, config.request_initial_wait, app_mutex_, app_cv_, stop_app_);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
