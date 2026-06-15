# Discovery Dashboard (WebSocket client)

A minimal, dependency-free web application that connects to the `discovery` example's
embedded WebSocket server and displays every discovered **topic**, **service** and
**action** in three live sections.

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
{ "kind": "topic",   "name": "rt/chatter" }
{ "kind": "service", "name": "/add_two_ints" }
{ "kind": "action",  "name": "/fibonacci" }
```

When a client connects, the server first **replays a snapshot** of everything discovered
so far (so a dashboard opened late still shows the full picture) and then streams live
updates as new entities are discovered. The dashboard deduplicates by `kind` + `name`, so
reconnecting or opening a second tab is always safe.
