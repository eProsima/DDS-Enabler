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
// Connects to the example's embedded WebSocket server, receives discovery messages of the
// form
//   { "kind": "topic"|"service"|"action", "name": "...",
//     "parts": [ { "label": "...", "details": "<json-placeholder>" }, ... ] },
// deduplicates them by kind+name, and renders them into three live sections.
//
// Each "details" is a JSON data placeholder: the JSON skeleton (with default values) that
// one would fill in to publish/send that data. A topic has a single unlabelled part and is
// shown as one dropdown with its JSON. A service has two parts, "Request" and "Reply", and
// is shown as an outer dropdown (the service name) containing one JSON dropdown per part.
// Actions have no parts and are shown as plain entries.
//

(function () {
  "use strict";

  // The three kinds we render, mapped to their DOM elements.
  const KINDS = ["topic", "service", "action"];
  const sections = {};
  for (const kind of KINDS) {
    const plural = kind + "s";
    sections[kind] = {
      // name -> { li, views } for the items already shown, to deduplicate and to allow
      // back-filling placeholders that arrive after the item. "views" holds one entry per
      // part (in order), each with its current details and a setDetails() updater.
      items: new Map(),
      list: document.getElementById(plural + "-list"),
      count: document.getElementById(plural + "-count"),
      empty: document.getElementById(plural + "-empty"),
    };
  }

  // Set a disclosure's fold indicator: a right-pointing arrow when it is foldable (a JSON
  // placeholder is available, or it is a group), or an unfilled circle when there is none.
  function setHasDetails(wrapper, toggle, hasDetails) {
    wrapper.classList.toggle("has-details", hasDetails);
    toggle.textContent = hasDetails ? "▶" : "○";  // ▶ triangle / ○ empty circle
  }

  // Pretty-print the placeholder JSON; fall back to the raw string if it is not valid
  // JSON (it normally already arrives indented from the server).
  function formatDetails(details) {
    if (!details) {
      return "No JSON placeholder available for this type yet.";
    }
    try {
      return JSON.stringify(JSON.parse(details), null, 2);
    } catch (e) {
      return details;
    }
  }

  // Build a disclosure header: a row with a fold indicator (arrow / circle) on the left
  // and a label. Returns the wrapper, the row, the toggle and the label elements.
  function makeDisclosure(labelText) {
    const wrapper = document.createElement("div");
    wrapper.className = "disclosure";

    const row = document.createElement("div");
    row.className = "item-row";

    const toggle = document.createElement("span");
    toggle.className = "toggle";

    const label = document.createElement("span");
    label.className = "item-name";
    label.textContent = labelText;

    row.appendChild(toggle);
    row.appendChild(label);
    wrapper.appendChild(row);

    return { wrapper: wrapper, row: row, toggle: toggle, label: label };
  }

  // Build a foldable JSON view: header (arrow/circle + label) plus a hidden <pre> with the
  // pretty-printed placeholder. Foldable only when a placeholder is available. Returns an
  // object with the wrapper and a setDetails(details) updater (used for late back-fills).
  function makeJsonView(labelText, details) {
    const d = makeDisclosure(labelText);

    const pre = document.createElement("pre");
    pre.className = "details";
    pre.style.display = "none";
    d.wrapper.appendChild(pre);

    d.row.addEventListener("click", function (e) {
      e.stopPropagation();
      if (!d.wrapper.classList.contains("has-details")) {
        return;  // no placeholder to show
      }
      const open = pre.style.display !== "none";
      pre.style.display = open ? "none" : "block";
      d.wrapper.classList.toggle("open", !open);
    });

    function setDetails(value) {
      pre.textContent = formatDetails(value);
      setHasDetails(d.wrapper, d.toggle, !!value);
    }
    setDetails(details);

    return { wrapper: d.wrapper, setDetails: setDetails };
  }

  // Build a foldable group: header (arrow + label) plus a hidden body to hold nested
  // disclosures. Always foldable. Returns the wrapper and the body container.
  function makeGroup(labelText) {
    const d = makeDisclosure(labelText);

    const body = document.createElement("div");
    body.className = "group-body";
    body.style.display = "none";
    d.wrapper.appendChild(body);

    d.row.addEventListener("click", function (e) {
      e.stopPropagation();
      const open = body.style.display !== "none";
      body.style.display = open ? "none" : "block";
      d.wrapper.classList.toggle("open", !open);
    });

    setHasDetails(d.wrapper, d.toggle, true);  // groups are always foldable

    return { wrapper: d.wrapper, body: body };
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

  function addItem(kind, name, parts) {
    const section = sections[kind];
    if (!section) {
      return;
    }
    parts = Array.isArray(parts) ? parts : [];

    const existing = section.items.get(name);
    if (existing) {
      // Already shown. Placeholders may arrive after the item, so back-fill any part
      // (in order) whose details just became available.
      for (let i = 0; i < parts.length && i < existing.views.length; i++) {
        const view = existing.views[i];
        const incoming = parts[i] && parts[i].details ? parts[i].details : "";
        if (incoming && !view.details) {
          view.details = incoming;
          view.setDetails(incoming);
        }
      }
      return;
    }

    const li = document.createElement("li");
    li.classList.add("new");

    // One updater per part, kept in part order so back-fills can target them.
    const views = [];

    if (parts.length === 0) {
      // Actions: plain entry, no dropdown.
      li.textContent = name;
    } else if (parts.length === 1 && !parts[0].label) {
      // Topics: a single JSON dropdown labelled with the topic name.
      const view = makeJsonView(name, parts[0].details);
      li.appendChild(view.wrapper);
      views.push({ details: parts[0].details || "", setDetails: view.setDetails });
    } else {
      // Services (and any future multi-part item): an outer dropdown for the name that
      // contains one JSON dropdown per part (e.g. "Request" and "Reply").
      const group = makeGroup(name);
      for (const part of parts) {
        const view = makeJsonView(part.label, part.details);
        group.body.appendChild(view.wrapper);
        views.push({ details: part.details || "", setDetails: view.setDetails });
      }
      li.appendChild(group.wrapper);
    }

    section.list.appendChild(li);
    section.items.set(name, { li: li, views: views });

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
      addItem(msg.kind, msg.name, Array.isArray(msg.parts) ? msg.parts : []);
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
