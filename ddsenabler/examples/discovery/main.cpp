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
 * DDS Enabler discovery example: print the name of every topic, service and action
 * the Enabler discovers, and also broadcast each discovery over a WebSocket server
 * so a web dashboard can display them live. Keeps running until the user presses
 * Ctrl+C.
 */

#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>

#include "ddsenabler/dds_enabler_runner.hpp"
#include "ddsenabler/DDSEnabler.hpp"

#include "ws/WebSocketServer.hpp"

// Default port the embedded WebSocket server listens on. It can be overridden with
// the second command-line argument (after the optional config file).
constexpr uint16_t DEFAULT_WS_PORT = 8080;

// Synchronization used to keep the application alive until a stop signal arrives.
std::mutex app_mutex;
std::condition_variable app_cv;
bool stop_app = false;

// Called by the Enabler every time a new DDS type is discovered. The Enabler builds a
// JSON "data placeholder" for the type (a skeleton with default values) that shows the
// exact JSON one would need to publish a sample of this type. We hand it to the server,
// keyed by type name, so topics of that type can expose it on the dashboard.
void on_type_discovered(
        const char* type_name,
        const char* /* serialized_type */,
        const unsigned char* /* serialized_type_internal */,
        uint32_t /* serialized_type_internal_size */,
        const char* data_placeholder)
{
    eprosima::ddsenabler::examples::ws::ws_server().on_type(type_name, data_placeholder);
}

// Called by the Enabler every time a new DDS topic is discovered. We forward the topic's
// type name so the server can attach the matching JSON placeholder to the topic.
void on_topic_discovered(
        const char* topic_name,
        const eprosima::ddsenabler::participants::TopicInfo& topic_info)
{
    eprosima::ddsenabler::examples::ws::ws_server().on_topic(topic_name, topic_info.type_name.c_str());
}

// Called by the Enabler every time a new ROS 2 / DDS service is discovered. A service has
// two types (request and reply), so we forward both type names; the server attaches the
// matching JSON placeholders so the dashboard can show the JSON for requesting and replying.
void on_service_discovered(
        const char* service_name,
        const eprosima::ddsenabler::participants::ServiceInfo& service_info)
{
    eprosima::ddsenabler::examples::ws::ws_server().on_service(
        service_name,
        service_info.request.type_name.c_str(),
        service_info.reply.type_name.c_str());
}

// Called by the Enabler every time a new ROS 2 / DDS action is discovered. An action is
// modeled as several services and topics; for publishing purposes the three JSONs of
// interest are the goal request, the feedback, and the result reply, so we forward those
// three types and the server attaches their JSON placeholders.
void on_action_discovered(
        const char* action_name,
        const eprosima::ddsenabler::participants::ActionInfo& action_info)
{
    eprosima::ddsenabler::examples::ws::ws_server().on_action(
        action_name,
        action_info.goal.request.type_name.c_str(),
        action_info.feedback.type_name.c_str(),
        action_info.result.reply.type_name.c_str());
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

    // Ignore SIGPIPE so that a dashboard client closing its browser tab cannot kill
    // this process when the server writes to a now-closed socket.
    signal(SIGPIPE, SIG_IGN);

    // Start the WebSocket server before creating the Enabler, so that it is already
    // listening when the first discovery callback fires. The port defaults to
    // DEFAULT_WS_PORT and can be overridden with the second command-line argument.
    const uint16_t ws_port = (argc > 2)
            ? static_cast<uint16_t>(std::atoi(argv[2]))
            : DEFAULT_WS_PORT;
    if (!examples::ws::ws_server().start(ws_port))
    {
        std::cerr << "Failed to start the WebSocket server." << std::endl;
        return EXIT_FAILURE;
    }

    // Register only the discovery notification callbacks; everything else is left unset.
    CallbackSet callbacks{};
    callbacks.dds.type_notification = on_type_discovered;
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
        examples::ws::ws_server().stop();
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

    // Stop the WebSocket server before the Enabler is torn down, so no discovery
    // callback can fire into a half-destroyed server.
    examples::ws::ws_server().stop();
    return EXIT_SUCCESS;
}
