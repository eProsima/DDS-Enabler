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
 * @file WebSocketServer.cpp
 */

#include "WebSocketServer.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <utility>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "base64.hpp"
#include "sha1.hpp"

namespace eprosima {
namespace ddsenabler {
namespace examples {
namespace ws {

namespace {

// The GUID defined by RFC 6455 used to compute Sec-WebSocket-Accept.
constexpr const char* WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// JSON-escape a string value (enough for entity names: quotes, backslash, control chars).
std::string json_escape(
        const std::string& in)
{
    std::string out;
    out.reserve(in.size() + 2);
    for (const char c : in)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                }
                else
                {
                    out += c;
                }
        }
    }
    return out;
}

// Build the per-item JSON message:
//   {"kind":"<kind>","name":"<name>",
//    "parts":[{"label":"<label>","details":"<json-placeholder>"}, ...]}
// Each part's "details" carries a JSON data placeholder as an escaped string (empty until
// the type is known). A topic has a single unlabelled part, a service has "Request" and
// "Reply" parts, and actions have none. The dashboard parses each "details" for display.
std::string make_item_message(
        const std::string& kind,
        const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& parts)
{
    std::string msg = "{\"kind\":\"" + json_escape(kind) +
            "\",\"name\":\"" + json_escape(name) + "\",\"parts\":[";
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        if (i != 0)
        {
            msg += ",";
        }
        msg += "{\"label\":\"" + json_escape(parts[i].first) +
                "\",\"details\":\"" + json_escape(parts[i].second) + "\"}";
    }
    msg += "]}";
    return msg;
}

// Read a full line (terminated by CRLF or LF) from @p fd. Returns false on EOF/error.
bool read_line(
        int fd,
        std::string& line)
{
    line.clear();
    char c;
    while (true)
    {
        const ssize_t n = ::recv(fd, &c, 1, 0);
        if (n <= 0)
        {
            return false;
        }
        if (c == '\n')
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            return true;
        }
        line.push_back(c);
        if (line.size() > 8192)
        {
            // Guard against an unbounded request line.
            return false;
        }
    }
}

// Encode @p payload as a single unmasked WebSocket text frame.
std::string encode_text_frame(
        const std::string& payload)
{
    std::string frame;
    frame.push_back(static_cast<char>(0x81));  // FIN + text opcode

    const std::size_t len = payload.size();
    if (len < 126)
    {
        frame.push_back(static_cast<char>(len));
    }
    else if (len <= 0xFFFF)
    {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    }
    else
    {
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i)
        {
            frame.push_back(static_cast<char>((static_cast<uint64_t>(len) >> (i * 8)) & 0xFF));
        }
    }
    frame += payload;
    return frame;
}

// Send all bytes of @p data to @p fd. Returns false on error.
bool send_all(
        int fd,
        const std::string& data)
{
    std::size_t sent = 0;
    while (sent < data.size())
    {
        const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n <= 0)
        {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

} // anonymous namespace

WebSocketServer::~WebSocketServer()
{
    stop();
}

bool WebSocketServer::start(
        uint16_t port)
{
    if (running_.load())
    {
        return true;
    }

    // Open an IPv6 socket and clear IPV6_V6ONLY so it accepts both IPv6 and IPv4
    // (via IPv4-mapped addresses) clients. Otherwise a client connecting to
    // "ws://localhost:..." — where "localhost" commonly resolves to the IPv6 "::1"
    // first — would stall on the IPv6 attempt before falling back to IPv4.
    listen_fd_ = ::socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_fd_ < 0)
    {
        std::cerr << "[WebSocket] Failed to create socket." << std::endl;
        return false;
    }

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int v6only = 0;
    if (::setsockopt(listen_fd_, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0)
    {
        std::cerr << "[WebSocket] Warning: could not enable dual-stack (IPv4-mapped) mode; "
                  << "IPv4 clients may be unable to connect." << std::endl;
    }

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        std::cerr << "[WebSocket] Failed to bind to port " << port << "." << std::endl;
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    if (::listen(listen_fd_, 8) < 0)
    {
        std::cerr << "[WebSocket] Failed to listen on port " << port << "." << std::endl;
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    running_.store(true);
    accept_thread_ = std::thread(&WebSocketServer::accept_loop, this);

    std::cout << "[WebSocket] Server listening on ws://localhost:" << port << std::endl;
    return true;
}

void WebSocketServer::accept_loop()
{
    while (running_.load())
    {
        // select() with a timeout so we can periodically re-check running_ and exit.
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_fd_, &read_fds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 200000;  // 200 ms

        const int ready = ::select(listen_fd_ + 1, &read_fds, nullptr, nullptr, &tv);
        if (ready <= 0)
        {
            continue;
        }

        const int client_fd = ::accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0)
        {
            continue;
        }

        Client client;
        client.fd = client_fd;
        client.reader = std::thread(&WebSocketServer::handle_client, this, client_fd);

        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.push_back(std::move(client));
    }
}

void WebSocketServer::handle_client(
        int fd)
{
    // ---- Handshake: read HTTP request headers, find Sec-WebSocket-Key. ----
    std::string key;
    std::string line;
    bool first = true;
    while (read_line(fd, line))
    {
        if (first)
        {
            first = false;  // request line, e.g. "GET / HTTP/1.1"
            continue;
        }
        if (line.empty())
        {
            break;  // end of headers
        }

        const std::string prefix = "sec-websocket-key:";
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(std::tolower(c));
                });
        if (lower.rfind(prefix, 0) == 0)
        {
            key = line.substr(prefix.size());
            // Trim surrounding whitespace.
            const auto start = key.find_first_not_of(" \t");
            const auto end = key.find_last_not_of(" \t");
            key = (start == std::string::npos) ? "" : key.substr(start, end - start + 1);
        }
    }

    if (key.empty())
    {
        drop_client(fd);
        return;
    }

    const std::string accept_key = Base64::encode(Sha1::digest(key + WS_GUID));
    const std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + accept_key + "\r\n\r\n";

    if (!send_all(fd, response))
    {
        drop_client(fd);
        return;
    }

    // ---- Replay snapshot of everything discovered so far. ----
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        for (const Item& item : registry_)
        {
            if (!send_item(fd, item))
            {
                drop_client(fd);
                return;
            }
        }
    }

    // ---- Read inbound frames only to detect a close/disconnect. ----
    // The dashboard never sends meaningful data; we just drain until the client
    // closes the connection.
    while (running_.load())
    {
        unsigned char header[2];
        const ssize_t n = ::recv(fd, header, 2, MSG_WAITALL);
        if (n <= 0)
        {
            break;  // client disconnected
        }

        const uint8_t opcode = header[0] & 0x0F;
        const bool masked = (header[1] & 0x80) != 0;
        uint64_t payload_len = header[1] & 0x7F;

        if (payload_len == 126)
        {
            unsigned char ext[2];
            if (::recv(fd, ext, 2, MSG_WAITALL) <= 0)
            {
                break;
            }
            payload_len = (static_cast<uint64_t>(ext[0]) << 8) | ext[1];
        }
        else if (payload_len == 127)
        {
            unsigned char ext[8];
            if (::recv(fd, ext, 8, MSG_WAITALL) <= 0)
            {
                break;
            }
            payload_len = 0;
            for (int i = 0; i < 8; ++i)
            {
                payload_len = (payload_len << 8) | ext[i];
            }
        }

        // Read and discard the masking key + payload.
        if (masked)
        {
            unsigned char mask[4];
            if (::recv(fd, mask, 4, MSG_WAITALL) <= 0)
            {
                break;
            }
        }

        // Drain the payload in chunks.
        char buf[1024];
        uint64_t remaining = payload_len;
        bool error = false;
        while (remaining > 0)
        {
            const std::size_t to_read =
                    static_cast<std::size_t>(std::min<uint64_t>(remaining, sizeof(buf)));
            const ssize_t r = ::recv(fd, buf, to_read, 0);
            if (r <= 0)
            {
                error = true;
                break;
            }
            remaining -= static_cast<uint64_t>(r);
        }

        if (error || opcode == 0x8)  // 0x8 == close frame
        {
            break;
        }
    }

    drop_client(fd);
}

std::string WebSocketServer::resolve_placeholder_nts(
        const std::string& type_name) const
{
    const auto it = type_placeholders_.find(type_name);
    return (it != type_placeholders_.end()) ? it->second : std::string();
}

std::string WebSocketServer::item_frame(
        const Item& item) const
{
    std::vector<std::pair<std::string, std::string>> parts;
    parts.reserve(item.parts.size());
    for (const Part& part : item.parts)
    {
        parts.emplace_back(part.label, part.details);
    }
    return encode_text_frame(make_item_message(item.kind, item.name, parts));
}

bool WebSocketServer::send_item(
        int fd,
        const Item& item)
{
    return send_all(fd, item_frame(item));
}

void WebSocketServer::broadcast_frame(
        const std::string& frame)
{
    std::vector<int> to_drop;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (const Client& client : clients_)
        {
            if (!send_all(client.fd, frame))
            {
                to_drop.push_back(client.fd);
            }
        }
    }

    for (const int fd : to_drop)
    {
        drop_client(fd);
    }
}

void WebSocketServer::on_type(
        const char* type_name,
        const char* data_placeholder)
{
    if (type_name == nullptr)
    {
        return;
    }

    const std::string type_str(type_name);
    const std::string placeholder_str = (data_placeholder != nullptr) ? data_placeholder : "";

    if (placeholder_str.empty())
    {
        // Nothing to resolve; still record the (empty) placeholder for completeness.
        std::lock_guard<std::mutex> lock(registry_mutex_);
        type_placeholders_[type_str] = placeholder_str;
        return;
    }

    // Store the placeholder and back-fill any item part of this type that was discovered
    // before the placeholder became available (the type and topic/service notifications
    // are independent, so either may arrive first).
    std::vector<std::string> frames;
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        type_placeholders_[type_str] = placeholder_str;

        for (Item& item : registry_)
        {
            bool changed = false;
            for (Part& part : item.parts)
            {
                if (part.type_name == type_str && part.details.empty())
                {
                    part.details = placeholder_str;
                    changed = true;
                }
            }
            if (changed)
            {
                frames.push_back(item_frame(item));
            }
        }
    }

    // Re-broadcast the back-filled items so connected dashboards pick up the placeholder.
    for (const std::string& frame : frames)
    {
        broadcast_frame(frame);
    }
}

void WebSocketServer::on_topic(
        const char* topic_name,
        const char* type_name)
{
    if (topic_name == nullptr)
    {
        return;
    }

    const std::string name_str(topic_name);
    const std::string type_str = (type_name != nullptr) ? type_name : "";

    Item item;
    // Deduplicate and register (insertion-ordered) before broadcasting.
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        const bool exists = std::any_of(registry_.begin(), registry_.end(),
                        [&](const Item& it)
                        {
                            return it.kind == "topic" && it.name == name_str;
                        });
        if (exists)
        {
            return;
        }

        // A topic has a single, unlabelled part holding its type's placeholder.
        item = Item{"topic", name_str, {Part{"", type_str, resolve_placeholder_nts(type_str)}}};
        registry_.push_back(item);
    }

    broadcast_frame(item_frame(item));
}

void WebSocketServer::on_service(
        const char* service_name,
        const char* request_type_name,
        const char* reply_type_name)
{
    if (service_name == nullptr)
    {
        return;
    }

    const std::string name_str(service_name);
    const std::string request_type = (request_type_name != nullptr) ? request_type_name : "";
    const std::string reply_type = (reply_type_name != nullptr) ? reply_type_name : "";

    Item item;
    // Deduplicate and register (insertion-ordered) before broadcasting.
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        const bool exists = std::any_of(registry_.begin(), registry_.end(),
                        [&](const Item& it)
                        {
                            return it.kind == "service" && it.name == name_str;
                        });
        if (exists)
        {
            return;
        }

        // A service has two parts: the request and the reply, each with its own placeholder.
        item = Item{"service", name_str, {
                        Part{"Request", request_type, resolve_placeholder_nts(request_type)},
                        Part{"Reply", reply_type, resolve_placeholder_nts(reply_type)}
                    }};
        registry_.push_back(item);
    }

    broadcast_frame(item_frame(item));
}

void WebSocketServer::on_action(
        const char* action_name,
        const char* goal_request_type_name,
        const char* feedback_type_name,
        const char* result_reply_type_name)
{
    if (action_name == nullptr)
    {
        return;
    }

    const std::string name_str(action_name);
    const std::string goal_request_type = (goal_request_type_name != nullptr) ? goal_request_type_name : "";
    const std::string feedback_type = (feedback_type_name != nullptr) ? feedback_type_name : "";
    const std::string result_reply_type = (result_reply_type_name != nullptr) ? result_reply_type_name : "";

    Item item;
    // Deduplicate and register (insertion-ordered) before broadcasting.
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        const bool exists = std::any_of(registry_.begin(), registry_.end(),
                        [&](const Item& it)
                        {
                            return it.kind == "action" && it.name == name_str;
                        });
        if (exists)
        {
            return;
        }

        // An action exposes the three placeholders relevant for driving it: the goal
        // request, the feedback, and the result reply.
        item = Item{"action", name_str, {
                        Part{"Goal Request", goal_request_type, resolve_placeholder_nts(goal_request_type)},
                        Part{"Feedback", feedback_type, resolve_placeholder_nts(feedback_type)},
                        Part{"Result Reply", result_reply_type, resolve_placeholder_nts(result_reply_type)}
                    }};
        registry_.push_back(item);
    }

    broadcast_frame(item_frame(item));
}

void WebSocketServer::drop_client(
        int fd)
{
    std::thread reader_to_join;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        const auto it = std::find_if(clients_.begin(), clients_.end(),
                        [fd](const Client& c)
                        {
                            return c.fd == fd;
                        });
        if (it == clients_.end())
        {
            return;
        }

        ::shutdown(it->fd, SHUT_RDWR);
        ::close(it->fd);

        // If we are running on the client's own reader thread, detach it; otherwise
        // move it out so we can join it after releasing the lock.
        if (it->reader.get_id() == std::this_thread::get_id())
        {
            it->reader.detach();
        }
        else
        {
            reader_to_join = std::move(it->reader);
        }
        clients_.erase(it);
    }

    if (reader_to_join.joinable())
    {
        reader_to_join.join();
    }
}

void WebSocketServer::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    // Unblock and join the accept loop.
    if (accept_thread_.joinable())
    {
        accept_thread_.join();
    }

    if (listen_fd_ >= 0)
    {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }

    // Close all client sockets and join their reader threads.
    std::vector<Client> clients;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients = std::move(clients_);
        clients_.clear();
    }

    for (Client& client : clients)
    {
        ::shutdown(client.fd, SHUT_RDWR);
        ::close(client.fd);
        if (client.reader.joinable())
        {
            client.reader.join();
        }
    }
}

WebSocketServer& ws_server()
{
    static WebSocketServer instance;
    return instance;
}

} // namespace ws
} // namespace examples
} // namespace ddsenabler
} // namespace eprosima
