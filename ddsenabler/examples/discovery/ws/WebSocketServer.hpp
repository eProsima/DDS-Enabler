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
 * @file WebSocketServer.hpp
 *
 * A tiny, dependency-free WebSocket server (RFC 6455) used by the discovery
 * example to broadcast discovery traces to connected web dashboards.
 *
 * It is intentionally minimal: localhost, text frames only, no TLS and no
 * permessage-deflate. It is adequate for an example and not meant for
 * production use.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace eprosima {
namespace ddsenabler {
namespace examples {
namespace ws {

/**
 * @brief Minimal WebSocket server that broadcasts discovery traces.
 *
 * The server keeps a registry of every discovered item so that a client which
 * connects later still receives a full snapshot of what has been discovered so
 * far, followed by live updates.
 *
 * All public methods are thread-safe: @ref on_discovery is called from the DDS
 * Enabler callback threads, while the accept loop and the per-client readers run
 * on their own threads.
 */
class WebSocketServer
{
public:

    WebSocketServer() = default;

    ~WebSocketServer();

    // Non-copyable, non-movable: it owns threads and sockets.
    WebSocketServer(
            const WebSocketServer&) = delete;
    WebSocketServer& operator =(
            const WebSocketServer&) = delete;

    /**
     * @brief Start listening on @p port and spawn the accept loop.
     *
     * @return @c true if the server started successfully, @c false otherwise.
     */
    bool start(
            uint16_t port);

    /**
     * @brief Register a discovered item and broadcast it to all clients.
     *
     * Items already present in the registry are ignored (deduplicated by
     * @p kind + @p name), so each item is broadcast at most once.
     *
     * @param kind "topic", "service" or "action".
     * @param name The discovered entity name.
     */
    void on_discovery(
            const char* kind,
            const char* name);

    /**
     * @brief Register a discovered DDS type and its JSON data placeholder.
     *
     * The placeholder is stored keyed by @p type_name. Any topic of this type that was
     * already discovered (placeholders may arrive after the topic) gets its placeholder
     * filled in and re-broadcast.
     *
     * @param type_name The discovered type name.
     * @param data_placeholder JSON skeleton (default values) for samples of this type.
     */
    void on_type(
            const char* type_name,
            const char* data_placeholder);

    /**
     * @brief Register a discovered topic and broadcast it to all clients.
     *
     * Like @ref on_discovery for the "topic" kind, but it also attaches the JSON data
     * placeholder of @p type_name (if already known) so the dashboard can show the JSON
     * needed to publish on the topic.
     *
     * @param topic_name The discovered topic name.
     * @param type_name The name of the topic's data type.
     */
    void on_topic(
            const char* topic_name,
            const char* type_name);

    /**
     * @brief Register a discovered service and broadcast it to all clients.
     *
     * A service has two types, so the dashboard can show two JSON placeholders: one for
     * the request and one for the reply. The placeholders of @p request_type_name and
     * @p reply_type_name are attached if already known; otherwise @ref on_type back-fills
     * them when the types are discovered.
     *
     * @param service_name The discovered service name.
     * @param request_type_name The name of the service's request type.
     * @param reply_type_name The name of the service's reply type.
     */
    void on_service(
            const char* service_name,
            const char* request_type_name,
            const char* reply_type_name);

    /**
     * @brief Stop the server: unblock the accept loop, join threads, close sockets.
     */
    void stop();

private:

    struct Client
    {
        int fd = -1;
        std::thread reader;
    };

    // Accept loop: waits for incoming connections until @ref running_ is false.
    void accept_loop();

    // Per-client handler: performs the handshake, replays the snapshot, then reads
    // inbound frames until the client disconnects.
    void handle_client(
            int fd);

    struct Item;  // forward declaration; defined below.

    // Look up a type's already-known JSON placeholder, or "" if not discovered yet.
    // The caller must hold @ref registry_mutex_.
    std::string resolve_placeholder_nts(
            const std::string& type_name) const;

    // Encode a discovery item as a WebSocket text frame ready to send.
    std::string item_frame(
            const Item& item) const;

    // Send a single discovery item as a text frame to @p fd. Returns false on error.
    bool send_item(
            int fd,
            const Item& item);

    // Broadcast a pre-encoded frame to every connected client, dropping those that error.
    void broadcast_frame(
            const std::string& frame);

    // Remove (and close) a client socket from the broadcast set.
    void drop_client(
            int fd);

    // ---- Listening / lifecycle ----
    int listen_fd_ = -1;
    std::thread accept_thread_;
    std::atomic_bool running_{false};

    // ---- Connected clients ----
    std::mutex clients_mutex_;
    std::vector<Client> clients_;

    // ---- Discovery registry (insertion-ordered, deduplicated) ----

    // A discoverable item may expose one or more JSON placeholders. A topic has a single
    // (unlabelled) part; a service has two parts, "Request" and "Reply"; actions have none.
    struct Part
    {
        std::string label;      // "" for topics; "Request"/"Reply" for services.
        std::string type_name;  // Type whose placeholder fills this part.
        std::string details;    // Resolved JSON placeholder (empty until the type is known).
    };
    struct Item
    {
        std::string kind;       // "topic", "service" or "action".
        std::string name;
        std::vector<Part> parts;
    };
    std::mutex registry_mutex_;
    std::vector<Item> registry_;
    // Type name -> JSON data placeholder, used to resolve part placeholders.
    // Guarded by registry_mutex_.
    std::unordered_map<std::string, std::string> type_placeholders_;
};

/**
 * @brief Process-wide WebSocket server instance.
 *
 * The discovery callbacks are C-style free functions and cannot capture state,
 * so they forward into this singleton.
 */
WebSocketServer& ws_server();

} // namespace ws
} // namespace examples
} // namespace ddsenabler
} // namespace eprosima
