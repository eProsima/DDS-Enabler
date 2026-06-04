// Copyright 2026 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * Minimal DDS Enabler example: print the name of every topic, service and action
 * the Enabler discovers, and keep running until the user presses Ctrl+C.
 */

#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>

#include "ddsenabler/dds_enabler_runner.hpp"
#include "ddsenabler/DDSEnabler.hpp"

// Synchronization used to keep the application alive until a stop signal arrives.
std::mutex app_mutex;
std::condition_variable app_cv;
bool stop_app = false;

// Called by the Enabler every time a new DDS topic is discovered.
void on_topic_discovered(
        const char* topic_name,
        const eprosima::ddsenabler::participants::TopicInfo& /* topic_info */)
{
    std::cout << "[Discovery] Topic discovered: " << topic_name << std::endl;
}

// Called by the Enabler every time a new ROS 2 / DDS service is discovered.
void on_service_discovered(
        const char* service_name,
        const eprosima::ddsenabler::participants::ServiceInfo& /* service_info */)
{
    std::cout << "[Discovery] Service discovered: " << service_name << std::endl;
}

// Called by the Enabler every time a new ROS 2 / DDS action is discovered.
void on_action_discovered(
        const char* action_name,
        const eprosima::ddsenabler::participants::ActionInfo& /* action_info */)
{
    std::cout << "[Discovery] Action discovered: " << action_name << std::endl;
}

// Stops the application cleanly when Ctrl+C (or another termination signal) is received.
void signal_handler(
        int /* signum */)
{
    {
        std::lock_guard<std::mutex> lock(app_mutex);
        stop_app = true;
    }
    app_cv.notify_all();
}

int main(
        int argc,
        char** argv)
{
    using namespace eprosima::ddsenabler;

    // Register only the discovery notification callbacks; everything else is left unset.
    CallbackSet callbacks{};
    callbacks.dds.topic_notification = on_topic_discovered;
    callbacks.service.service_notification = on_service_discovered;
    callbacks.action.action_notification = on_action_discovered;

    // Create the Enabler. An optional YAML configuration file may be passed as the first argument.
    std::shared_ptr<DDSEnabler> enabler;
    bool enabler_created = (argc > 1)
            ? create_dds_enabler(argv[1], callbacks, enabler)
            : create_dds_enabler(yaml::EnablerConfiguration(""), callbacks, enabler);

    if (!enabler_created)
    {
        std::cerr << "Failed to create DDSEnabler instance." << std::endl;
        return EXIT_FAILURE;
    }

    // Install signal handlers for a clean shutdown.
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "DDS Enabler running. Listening for topics, services and actions. "
              << "Press Ctrl+C to stop." << std::endl;

    // Block until a stop signal is received.
    {
        std::unique_lock<std::mutex> lock(app_mutex);
        app_cv.wait(lock, []
                {
                    return stop_app;
                });
    }

    std::cout << "Stopping DDS Enabler..." << std::endl;
    return EXIT_SUCCESS;
}
