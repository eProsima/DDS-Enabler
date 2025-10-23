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

#include "Utils.hpp"

#include "ddsenabler/dds_enabler_runner.hpp"
#include "ddsenabler/DDSEnabler.hpp"

#include "CLIParser.hpp"

CLIParser::example_config config;
bool action_discovered_ = false;
uint32_t received_results_ = 0;
std::vector<std::pair<eprosima::ddsenabler::participants::UUID, std::string>> received_requests_;
std::mutex app_mutex_;
std::condition_variable app_cv_;
bool stop_app_ = false;

const std::string REQUESTS_SUBDIR = "goals";
const std::string TYPES_SUBDIR = "types";
const std::string ACTION_SUBDIR = "actions";

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
            !utils::save_type_to_file((std::filesystem::path(config.persistence_path) / TYPES_SUBDIR).string(),
            type_name,
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
        const eprosima::ddsenabler::participants::TopicInfo& topic_info)
{
}

// Static type query callback
static bool test_topic_query_callback(
        const char* topic_name,
        eprosima::ddsenabler::participants::TopicInfo& topic_info)
{
    return false;
}

// Static action notification callback
static void test_action_notification_callback(
        const char* action_name,
        const eprosima::ddsenabler::participants::ActionInfo& action_info)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    if (config.action_name == std::string(action_name))
    {
        std::cout << "Action callback received: " << action_name << std::endl;

        std::string action_file = (std::filesystem::path(
                    config.persistence_path) /
                ACTION_SUBDIR
                ).string();
        if (!config.persistence_path.empty())
        {
            utils::save_action_to_file(
                action_file,
                action_name,
                action_info);

            action_discovered_ = true;
            app_cv_.notify_all();
        }
    }
    else
    {
        std::cout << "Ignoring action callback for: " << action_name << std::endl;
    }
}

// Static action query callback
static bool test_action_query_callback(
        const char* action_name,
        eprosima::ddsenabler::participants::ActionInfo& action_info)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    if (config.action_name == std::string(action_name))
    {
        std::cout << "Action query callback received: " << action_name << std::endl;

        if (!config.persistence_path.empty())
        {
            std::string action_file = (std::filesystem::path(
                        config.persistence_path) /
                    ACTION_SUBDIR
                    ).string();
            if (!utils::load_action_from_file(
                        action_file,
                        action_name,
                        action_info))
            {
                std::cerr << "Failed to load action: " << action_name << std::endl;
                return false;
            }
            return true;
        }
    }
    else
    {
        std::cout << "Ignoring action query callback for: " << action_name << std::endl;
    }
    return false;
}

// Static action goal request notification callback
static bool test_action_goal_request_notification_callback(
        const char* action_name,
        const char* json,
        const eprosima::ddsenabler::participants::UUID& goal_id,
        int64_t publish_time)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    if (config.action_name == std::string(action_name))
    {
        std::cout << "Action goal request callback received: " << action_name << std::endl;
        received_requests_.emplace_back(goal_id, json);
        app_cv_.notify_all();
        return true;
    }
    return false;
}

// Static action result notification callback
static void test_action_result_notification_callback(
        const char* action_name,
        const char* json,
        const eprosima::ddsenabler::participants::UUID& goal_id,
        int64_t publish_time)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    std::cout << "Action result callback received: " << action_name << std::endl;
    received_results_++;
    app_cv_.notify_all();
}

// Static action feedback notification callback
static void test_action_feedback_notification_callback(
        const char* action_name,
        const char* json,
        const eprosima::ddsenabler::participants::UUID& goal_id,
        int64_t publish_time)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    std::cout << "Action feedback callback received for action: " << action_name << std::endl;
}

// Static action status notification callback
static void test_action_status_notification_callback(
        const char* action_name,
        const eprosima::ddsenabler::participants::UUID& goal_id,
        eprosima::ddsenabler::participants::StatusCode statusCode,
        const char* statusMessage,
        int64_t publish_time)
{
    std::lock_guard<std::mutex> lock(app_mutex_);
    std::cout << "Action status callback received: " << statusMessage << std::endl;
}

// Static action cancel request notification callback
static void test_action_cancel_request_notification_callback(
        const char* action_name,
        const eprosima::ddsenabler::participants::UUID& goal_id,
        int64_t timestamp,
        uint64_t request_id,
        int64_t publish_time)
{
    // NOT IMPLEMENTED
}

bool wait_for_action_discovery(
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv)
{
    std::unique_lock<std::mutex> lock(app_mutex);
    if (stop_app_ || !app_cv.wait_for(lock, std::chrono::seconds(timeout),
            []()
            {
                return stop_app_ || action_discovered_;
            }))
    {
        std::cerr << "Timeout waiting for service discovery." << std::endl;
        return false;
    }
    return true;
}

bool wait_for_action_request(
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv,
        eprosima::ddsenabler::participants::UUID& request_id,
        std::string& goal_json)
{
    std::unique_lock<std::mutex> lock(app_mutex);
    if (stop_app_ || !app_cv.wait_for(lock, std::chrono::seconds(timeout),
            []()
            {
                return stop_app_ || !received_requests_.empty();
            }))
    {
        std::cerr << "Timeout waiting for service request." << std::endl;
        return false;
    }

    request_id = received_requests_.back().first;
    goal_json = received_requests_.back().second;
    received_requests_.pop_back();
    return true;
}

bool wait_for_action_result(
        uint32_t timeout,
        std::mutex& app_mutex,
        std::condition_variable& app_cv,
        uint32_t sent_requests)
{
    std::unique_lock<std::mutex> lock(app_mutex);
    if (stop_app_ || !app_cv.wait_for(lock, std::chrono::seconds(timeout),
            [&sent_requests]()
            {
                return stop_app_ || received_results_ >= sent_requests;
            }))
    {
        std::cerr << "Timeout waiting for action result." << std::endl;
        return false;
    }
    return true;
}

bool client_routine(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& action_name,
        const std::string& goal_path,
        uint32_t timeout,
        uint32_t request_initial_wait,
        bool cancel_requests)
{
    // Wait for service to be discovered
    if (!wait_for_action_discovery(timeout, app_mutex_, app_cv_))
    {
        std::cerr << "Failed to discover service: " << action_name << std::endl;
        return false;
    }

    // Wait a bit before starting to publish so types and topics can be discovered
    std::this_thread::sleep_for(std::chrono::seconds(request_initial_wait));

    // Get collection of files to publish, sorted in increasing order by their name (assumed to be numeric)
    std::vector<std::pair<std::filesystem::path, int32_t>> sample_files;
    utils::get_sorted_files(goal_path, sample_files);
    uint32_t sent_requests = 0;
    for (const auto& [path, number] : sample_files)
    {
        eprosima::ddsenabler::participants::UUID request_id;
        std::ifstream file(path, std::ios::binary);
        if (file)
        {
            std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (enabler->send_action_goal(action_name, file_content, request_id))
            {
                std::cout << "Published content from file: " << path.filename() << " in service: "
                          << action_name << std::endl;
                sent_requests++;
            }
            else
            {
                std::cerr << "Failed to publish content from file: " << path.filename() << " in service: "
                          << action_name << std::endl;
                return false;
            }
        }
        else
        {
            std::cerr << "Failed to open file: " << path << std::endl;
        }
        if (cancel_requests)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate processing time
            // Cancel the request after sending it
            if (!enabler->cancel_action_goal(action_name, request_id))
            {
                std::cerr << "Failed to send action cancel request." << std::endl;
                return false;
            }
            std::cout << "Sent cancel request for action: " << action_name << std::endl;
        }

        // Wait publish period or until stop signal is received
        else if (!wait_for_action_result(timeout, app_mutex_, app_cv_, sent_requests))
        {
            std::cerr << "Failed to receive service reply." << std::endl;
            return false;
        }
    }
    return true;
}

bool server_specific_logic(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& action_name,
        uint64_t fibonacci_number,
        const eprosima::ddsenabler::participants::UUID& request_id)
{
    std::vector<uint64_t> fibonacci_sequence = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610};
    std::string json = "{\"sequence\": [";
    std::string feedback_json = "{\"partial_sequence\": [";
    for (size_t i = 0; i < fibonacci_number; ++i)
    {
        json += std::to_string(fibonacci_sequence[i]);
        feedback_json += std::to_string(fibonacci_sequence[i]);

        std::string feedback_tmp = feedback_json;
        feedback_tmp += "]}";
        if (!enabler->send_action_feedback(
                    action_name.c_str(),
                    feedback_tmp.c_str(),
                    request_id))
        {
            std::cerr << "Failed to send action feedback" << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (i != fibonacci_number - 1)
        {
            json += ", ";
            feedback_json += ", ";
        }
    }
    json += "]}";

    if (!enabler->send_action_result(
                action_name.c_str(),
                request_id,
                eprosima::ddsenabler::participants::StatusCode::SUCCEEDED,
                json.c_str()))
    {
        std::cerr << "Failed to send action result" << std::endl;
        return false;
    }

    return true;
}

bool server_routine(
        std::shared_ptr<eprosima::ddsenabler::DDSEnabler> enabler,
        const std::string& action_name,
        uint32_t expected_requests,
        uint32_t timeout)
{
    // Announce action
    if (!enabler->announce_action(action_name))
    {
        std::cerr << "Failed to announce action: " << action_name << std::endl;
        return false;
    }

    std::cout << "Action announced: " << action_name << std::endl;

    while (!stop_app_)
    {
        eprosima::ddsenabler::participants::UUID request_id;
        std::string goal_json;
        if (!wait_for_action_request(timeout, app_mutex_, app_cv_, request_id, goal_json))
        {
            std::cerr << "Timeout waiting for action request." << std::endl;
            return false;
        }

        uint64_t fibonacci_number = 5; // Default Fibonacci number, can be parsed from json if needed
        std::cout << "Received request for action: " << action_name << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate processing time

        // Send Feedback & Result
        if (!server_specific_logic(
                    enabler,
                    action_name,
                    fibonacci_number,
                    request_id))
        {
            std::cerr << "Failed to process action: " << action_name << std::endl;
            return false;
        }

        // Check if we have received the expected number of requests (or run indefinitely if expected_requests is 0)
        {
            std::lock_guard<std::mutex> lock(app_mutex_);
            if (expected_requests != 0 && ++received_results_ >= expected_requests)
            {
                break;
            }
        }
    }

    // Revoke service
    if (!enabler->revoke_action(action_name))
    {
        std::cerr << "Failed to revoke service: " << action_name << std::endl;
        return false;
    }

    return true;
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
    std::vector<std::string> subdirs = {TYPES_SUBDIR, ACTION_SUBDIR};
    utils::init_persistence(config.persistence_path, subdirs);

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
            // NOTE: Service callbacks are not used in this example, but can be added if needed
        },
        {
            test_action_notification_callback,
            test_action_goal_request_notification_callback,
            test_action_feedback_notification_callback,
            test_action_cancel_request_notification_callback,
            test_action_result_notification_callback,
            test_action_status_notification_callback,
            test_action_query_callback
        }
    };

    std::shared_ptr<DDSEnabler> enabler;
    bool enabler_created = false;
    if (config.config_file_path.empty())
        enabler_created = create_dds_enabler(yaml::EnablerConfiguration(""), callbacks, enabler);
    else
        enabler_created = create_dds_enabler(config.config_file_path.c_str(), callbacks, enabler);
    if (!enabler_created)
    {
        std::cerr << "Failed to create DDSEnabler instance." << std::endl;
        return EXIT_FAILURE;
    }

    bool ret = false;
    // Service logic based on announce_server flag
    if (config.announce_server)
    {

        ret = server_routine(enabler, config.action_name, config.expected_requests, config.timeout);
    }
    else
    {
        if (config.goals_path.empty())
        {
            std::cerr << "Request path is not set." << std::endl;
            return EXIT_FAILURE;
        }

        ret = client_routine(enabler, config.action_name, config.goals_path, config.timeout, config.request_initial_wait,
                        config.cancel_requests);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
