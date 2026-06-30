# Discovery Dashboard (WebSocket client)

A minimal, dependency-free web application that connects to the `discovery` example's
embedded WebSocket server and displays every discovered **topic**, **service** and
**action** in three live sections.

Topics, services and actions are expandable:

- A **topic** opens a dropdown showing the **JSON data placeholder** for its type — the
  JSON skeleton (with default values) that you would fill in to publish a sample on it.
- A **service** opens a dropdown with two nested entries, **Request** and **Reply**, each
  of which opens its own JSON placeholder dropdown.
- An **action** opens a dropdown with three nested entries, **Goal Request**, **Feedback**
  and **Result Reply**, each of which opens its own JSON placeholder dropdown.

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
{ "kind": "topic",   "name": "rt/chatter",
  "parts": [ { "label": "", "details": "{\n    \"data\": \"\"\n}" } ] }
{ "kind": "service", "name": "/add_two_ints",
  "parts": [ { "label": "Request", "details": "..." }, { "label": "Reply", "details": "..." } ] }
{ "kind": "action",  "name": "/fibonacci",
  "parts": [ { "label": "Goal Request", "details": "..." }, { "label": "Feedback", "details": "..." }, { "label": "Result Reply", "details": "..." } ] }
```

Each entry in `parts` carries a JSON data placeholder (as an escaped string) under
`details`, populated from the Enabler's type-discovery callback. A topic has a single
unlabelled part, a service has `Request` and `Reply` parts, and an action has
`Goal Request`, `Feedback` and `Result Reply` parts. The dashboard parses and
pretty-prints each `details` inside its dropdown.

Because the type and topic/service discovery notifications are independent, an item may be
announced before its placeholder(s) are known. In that case the server first sends the item
with empty `details`, then re-sends it with the placeholder(s) filled in once the
type(s) are discovered; the dashboard updates the dropdowns in place.

When a client connects, the server first **replays a snapshot** of everything discovered
so far (so a dashboard opened late still shows the full picture) and then streams live
updates as new entities are discovered. The dashboard deduplicates by `kind` + `name`, so
reconnecting or opening a second tab is always safe.
