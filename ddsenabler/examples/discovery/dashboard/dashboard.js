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

//
// WebSocket client for the DDS Enabler discovery dashboard.
//
// Connects to the example's embedded WebSocket server, receives discovery
// messages of the form { "kind": "topic"|"service"|"action", "name": "..." },
// deduplicates them by kind+name, and renders them into three live sections.
//

(function () {
  "use strict";

  // The three kinds we render, mapped to their DOM elements.
  const KINDS = ["topic", "service", "action"];
  const sections = {};
  for (const kind of KINDS) {
    const plural = kind + "s";
    sections[kind] = {
      // Keys (names) already shown, to deduplicate.
      seen: new Set(),
      list: document.getElementById(plural + "-list"),
      count: document.getElementById(plural + "-count"),
      empty: document.getElementById(plural + "-empty"),
    };
  }

  const urlInput = document.getElementById("ws-url");
  const connectBtn = document.getElementById("connect-btn");
  const statusEl = document.getElementById("status");

  let socket = null;
  let reconnectTimer = null;
  // When true, a close event should trigger an automatic reconnect.
  let autoReconnect = false;

  function setStatus(text, cls) {
    statusEl.textContent = text;
    statusEl.className = "status " + cls;
  }

  function addItem(kind, name) {
    const section = sections[kind];
    if (!section || section.seen.has(name)) {
      return;
    }
    section.seen.add(name);

    const li = document.createElement("li");
    li.textContent = name;
    li.classList.add("new");
    section.list.appendChild(li);

    section.count.textContent = String(section.seen.size);
    section.empty.style.display = "none";
  }

  function clearAll() {
    for (const kind of KINDS) {
      const section = sections[kind];
      section.seen.clear();
      section.list.innerHTML = "";
      section.count.textContent = "0";
      section.empty.style.display = "";
    }
  }

  function handleMessage(event) {
    let msg;
    try {
      msg = JSON.parse(event.data);
    } catch (e) {
      console.warn("Ignoring malformed message:", event.data);
      return;
    }
    if (msg && typeof msg.kind === "string" && typeof msg.name === "string") {
      addItem(msg.kind, msg.name);
    }
  }

  function connect() {
    disconnect();

    const url = urlInput.value.trim();
    if (!url) {
      return;
    }

    // A fresh connection starts from an empty board; the server replays a full
    // snapshot of everything discovered so far on connect.
    clearAll();
    autoReconnect = true;
    setStatus("Connecting…", "connecting");

    try {
      socket = new WebSocket(url);
    } catch (e) {
      setStatus("Disconnected", "disconnected");
      scheduleReconnect();
      return;
    }

    socket.onopen = function () {
      setStatus("Connected", "connected");
    };
    socket.onmessage = handleMessage;
    socket.onclose = function () {
      setStatus("Disconnected", "disconnected");
      scheduleReconnect();
    };
    socket.onerror = function () {
      // onclose follows; reconnect is handled there.
    };
  }

  function disconnect() {
    autoReconnect = false;
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
    if (socket) {
      socket.onclose = null;
      socket.onerror = null;
      socket.onmessage = null;
      try {
        socket.close();
      } catch (e) {
        // ignore
      }
      socket = null;
    }
  }

  function scheduleReconnect() {
    if (!autoReconnect || reconnectTimer) {
      return;
    }
    reconnectTimer = setTimeout(function () {
      reconnectTimer = null;
      if (autoReconnect) {
        connect();
      }
    }, 2000);
  }

  connectBtn.addEventListener("click", connect);
  urlInput.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      connect();
    }
  });

  // Connect automatically on page load.
  connect();
})();
