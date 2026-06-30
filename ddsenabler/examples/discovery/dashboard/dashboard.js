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
// messages of the form
//   { "kind": "topic"|"service"|"action", "name": "...", "details": "..." },
// deduplicates them by kind+name, and renders them into three live sections.
//
// For topics, "details" carries the JSON data placeholder for the topic's type: the
// JSON skeleton (with default values) that one would fill in to publish a sample on
// that topic. Clicking a topic toggles a dropdown showing this JSON.
//

(function () {
  "use strict";

  // The three kinds we render, mapped to their DOM elements.
  const KINDS = ["topic", "service", "action"];
  const sections = {};
  for (const kind of KINDS) {
    const plural = kind + "s";
    sections[kind] = {
      // name -> { li, pre, details } for the items already shown, to deduplicate
      // and to allow back-filling a placeholder that arrives after its topic.
      items: new Map(),
      list: document.getElementById(plural + "-list"),
      count: document.getElementById(plural + "-count"),
      empty: document.getElementById(plural + "-empty"),
    };
  }

  // Set the topic's fold indicator: a right-pointing arrow when a JSON placeholder is
  // available (foldable), or an unfilled circle when there is none (not foldable).
  function setHasDetails(li, toggle, hasDetails) {
    li.classList.toggle("has-details", hasDetails);
    toggle.textContent = hasDetails ? "▶" : "○";  // ▶ triangle / ○ empty circle
  }

  // Pretty-print the placeholder JSON; fall back to the raw string if it is not valid
  // JSON (it normally already arrives indented from the server).
  function formatDetails(details) {
    if (!details) {
      return "No JSON placeholder available for this topic's type yet.";
    }
    try {
      return JSON.stringify(JSON.parse(details), null, 2);
    } catch (e) {
      return details;
    }
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

  function addItem(kind, name, details) {
    const section = sections[kind];
    if (!section) {
      return;
    }

    const existing = section.items.get(name);
    if (existing) {
      // Already shown. The placeholder may arrive after the topic, so update the
      // dropdown and switch the indicator from "no JSON" to an arrow if details
      // just became available.
      if (details && !existing.details && existing.pre) {
        existing.details = details;
        existing.pre.textContent = formatDetails(details);
        setHasDetails(existing.li, existing.toggle, true);
      }
      return;
    }

    const li = document.createElement("li");
    li.classList.add("new");

    let pre = null;
    let toggle = null;
    if (kind === "topic") {
      // Topics with a placeholder are expandable: a clickable row (arrow on the
      // left of the name) plus a hidden JSON view. Topics with no placeholder show
      // an unfilled circle and are not expandable.
      const row = document.createElement("div");
      row.className = "item-row";

      toggle = document.createElement("span");
      toggle.className = "toggle";

      const nameSpan = document.createElement("span");
      nameSpan.className = "item-name";
      nameSpan.textContent = name;

      // Arrow on the left, then the topic name.
      row.appendChild(toggle);
      row.appendChild(nameSpan);

      pre = document.createElement("pre");
      pre.className = "details";
      pre.style.display = "none";
      pre.textContent = formatDetails(details);

      row.addEventListener("click", function () {
        // Only foldable when there is a JSON placeholder to show.
        if (!li.classList.contains("has-details")) {
          return;
        }
        const open = pre.style.display !== "none";
        pre.style.display = open ? "none" : "block";
        li.classList.toggle("open", !open);
      });

      setHasDetails(li, toggle, !!details);

      li.appendChild(row);
      li.appendChild(pre);
    } else {
      li.textContent = name;
    }

    section.list.appendChild(li);
    section.items.set(name, { li: li, pre: pre, toggle: toggle, details: details || "" });

    section.count.textContent = String(section.items.size);
    section.empty.style.display = "none";
  }

  function clearAll() {
    for (const kind of KINDS) {
      const section = sections[kind];
      section.items.clear();
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
      addItem(msg.kind, msg.name, typeof msg.details === "string" ? msg.details : "");
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
