# Discovery Dashboard (WebSocket client)

A minimal, dependency-free web application that connects to the `discovery` example's
embedded WebSocket server and displays every discovered **topic**, **service** and
**action** in three live sections.

Each discovered **topic** is expandable: click it to open a dropdown showing the **JSON
data placeholder** for the topic's type — the JSON skeleton (with default values) that you
would fill in to publish a sample on that topic.

It is plain HTML + CSS + JavaScript — there is **no build step** and no `npm install`.

## Files

- `index.html` — the page layout (three sections: Topics, Services, Actions).
- `dashboard.js` — the WebSocket client: connects, deduplicates by name, renders updates.
- `style.css` — styling.

## Running

First, start the `discovery` example so the WebSocket server is listening (default
`ws://localhost:8080` — see the example's `README.md`).

Then open the dashboard. Either:

- **Open the file directly:** double-click `index.html`, or open it in your browser
  (`file://.../dashboard/index.html`).

- **Serve it over HTTP** (recommended; avoids any browser `file://` restrictions):

  ```bash
  cd dashboard
  python3 -m http.server 8000
  ```

  Then browse to <http://localhost:8000>.

The dashboard connects automatically on load. The WebSocket URL can be changed in the
field at the top of the page (use this if you started the example on a non-default port
or on another host). If the connection drops, the dashboard retries automatically every
couple of seconds.

## How it works

The server sends one small JSON message per discovered item:

```json
{ "kind": "topic",   "name": "rt/chatter",   "details": "{\n    \"data\": \"\"\n}" }
{ "kind": "service", "name": "/add_two_ints", "details": "" }
{ "kind": "action",  "name": "/fibonacci",    "details": "" }
```

`details` carries the JSON data placeholder as an escaped string. It is populated for
**topics** (from the Enabler's type-discovery callback) and empty for services and actions.
The dashboard parses and pretty-prints it inside the topic's dropdown.

Because the type and topic discovery notifications are independent, a topic may be
announced before its placeholder is known. In that case the server first sends the topic
with an empty `details`, then re-sends the same topic with the placeholder filled in once
the type is discovered; the dashboard updates the dropdown in place.

When a client connects, the server first **replays a snapshot** of everything discovered
so far (so a dashboard opened late still shows the full picture) and then streams live
updates as new entities are discovered. The dashboard deduplicates by `kind` + `name`, so
reconnecting or opening a second tab is always safe.
