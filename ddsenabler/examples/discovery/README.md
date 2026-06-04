# Discovery Example Readme

This is the simplest DDS Enabler example and a good starting point.
It launches a DDS Enabler that listens to the DDS network and prints the **name** of every topic, service and action it discovers — nothing else (no types, no QoS, no data).

The application keeps running until it is stopped with `Ctrl+C`.

It registers only the three discovery notification callbacks of the `CallbackSet`:

- `dds.topic_notification` → prints discovered topic names.
- `service.service_notification` → prints discovered service names.
- `action.action_notification` → prints discovered action names.

## Example Command

```bash
./ddsenabler_example_discovery config.yml
```

Run it and then start any DDS / ROS 2 application (for instance the ROS 2 `talker`, or another DDS Enabler example such as `publish`, `service` or `action`).
Each newly discovered topic, service or action prints a line like:

```
[Discovery] Topic discovered: rt/chatter
[Discovery] Service discovered: /add_two_ints
[Discovery] Action discovered: /fibonacci
```

Press `Ctrl+C` to stop the application cleanly.

An optional YAML configuration file may be passed as the first argument to customize the DDS behavior (domain, allow/deny topic lists, etc.).
Without it, the Enabler uses its default configuration (DDS domain 0).
