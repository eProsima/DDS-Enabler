# Discovery Example README

This is the simplest DDS Enabler example and a good starting point.
It launches a DDS Enabler that listens to the DDS network and reports the **name** of every topic, service and action it discovers. For topics it additionally exposes the **JSON data placeholder** of the topic's type: the JSON skeleton (with default values) you would fill in to publish a sample on that topic.

Each discovery is both **printed to the terminal** and **broadcast over a WebSocket server** embedded in the example, so a [web dashboard](dashboard/README.md) can display the discovered topics, services and actions live. In the dashboard, clicking a topic opens a dropdown with its JSON placeholder.

The application keeps running until it is stopped with `Ctrl+C`.

It registers the discovery notification callbacks of the `CallbackSet`:

- `dds.topic_notification` → prints / broadcasts discovered topic names.
- `dds.type_notification` → captures each type's JSON data placeholder, attached to topics of that type.
- `service.service_notification` → prints / broadcasts discovered service names.
- `action.action_notification` → prints / broadcasts discovered action names.

## Live Dashboard

On startup the example opens a small, dependency-free WebSocket server (default `ws://localhost:8080`).
Each discovered entity is broadcast to all connected clients as a JSON message:

```json
{ "kind": "topic",   "name": "rt/chatter",   "details": "{\n    \"data\": \"\"\n}" }
{ "kind": "service", "name": "/add_two_ints", "details": "" }
{ "kind": "action",  "name": "/fibonacci",    "details": "" }
```

The `details` field carries the JSON data placeholder (as an escaped string) for topics, and is empty for services and actions. Since type and topic notifications are independent, a topic may be broadcast first with an empty `details` and then re-broadcast with the placeholder filled in once its type is discovered.

When a client connects, the server first replays a **snapshot** of everything discovered so far, then streams live updates. A late-joining dashboard therefore still sees the full picture.

The web dashboard (a WebSocket client) lives in the [`dashboard/`](dashboard/) folder — see its [README](dashboard/README.md) for how to open it. It renders three live sections: Topics, Services and Actions.

The WebSocket server is a minimal hand-rolled RFC 6455 implementation (POSIX sockets, text frames only, no TLS) intended for this example, not for production. It vendors small public-domain SHA-1 (`ws/sha1.hpp`) and base64 (`ws/base64.hpp`) helpers to compute the handshake key.

## Compilation

The example is compiled together with the DDS Enabler when the project is built with the `-DCOMPILE_EXAMPLES=ON` CMake option.
It can also be compiled standalone against an already installed DDS Enabler.

First, source the environment where the DDS Enabler is installed.

```bash
source <ddsenabler-installation-path>/install/setup.bash
```

Then configure and build the example from its own directory.

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The `ddsenabler_example_discovery` executable is generated inside the `build` directory.

## Example Command

```bash
./ddsenabler_example_discovery config.yml
```

Run it and then start any DDS / ROS 2 application (for instance the ROS 2 `talker`, or another DDS Enabler example such as `publish`, `service` or `action`).
When a topic, service, or action is discovered, the application prints a line like:

```text
[WebSocket] Server listening on ws://localhost:8080
[Discovery] Topic discovered: rt/chatter
[Discovery] Service discovered: /add_two_ints
[Discovery] Action discovered: /fibonacci
```

Open the [dashboard](dashboard/README.md) in a browser to see the same discoveries rendered live.

Press `Ctrl+C` to stop the application cleanly.

An optional YAML configuration file may be passed as the first argument to customize the DDS behavior (domain, allow/deny topic lists, etc.).
Without it, the Enabler uses its default configuration (DDS domain 0).

The WebSocket port can be overridden with an optional second argument (default `8080`):

```bash
./ddsenabler_example_discovery config.yml 9000
```
