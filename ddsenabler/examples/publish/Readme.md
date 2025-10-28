# Publish Example Readme

This example demonstrates the usage of the DDS Enabler for implementing a publish-subscribe pattern. The example includes a publisher and a subscriber setup for the desired topic.

The following example commands illustrate how to run the publisher and subscriber to match with the `talker` and `listener` examples provided in the ROS2 demos.

## Example Commands

### PUBLISHER
```bash
./install/ddsenabler/examples/publish/ddsenabler_example_publish \
	--persistence-path ./install/ddsenabler/examples/persistence/ \
	--publish-path ./install/ddsenabler/examples/publish/samples \
	--publish-period 200 \
	--publish-initial-wait 1000 \
	--publish-topic rt/chatter
	<optional> --timeout 5
	<optional> --config $PATH_TO_CONFIG_FILE
```

### SUBSCRIBER
```bash
./install/ddsenabler/examples/publish/ddsenabler_example_publish \
	--persistence-path ./install/ddsenabler/examples/persistence/ \
	--expected-types 1 \
	--expected-topics 1 \
	--expected-data 5
	<optional> --timeout 5
	<optional> --config $PATH_TO_CONFIG_FILE
```
