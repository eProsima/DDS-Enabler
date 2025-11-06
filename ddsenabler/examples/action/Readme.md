# Action Example Readme

This example demonstrates the usage of the DDS Enabler for implementing an action. The example includes a client and a server setup for the desired action.

By default, the example is prepared for the `Fibonacci` action, providing a standard reply with a constant result of the Fibonacci sequence of order 5 for any request.

Users are encouraged to implement their own functional server logic as needed in the `server_specific_logic` function. The persistence/goals directory must be modified to suit the specific action type and its expected goal requests.

## Example Commands

### CLIENT
```bash
./install/ddsenabler/examples/action/ddsenabler_example_action client \
	--persistence-path ./install/ddsenabler/examples/persistence/ \
	--goals-path ./install/ddsenabler/examples/action/goals/ \
	--request-initial-wait 3
	<optional> --cancel-requests false
	<optional> --config $PATH_TO_CONFIG_FILE
	<optional> --action-name $ACTION_NAME
	<optional> --timeout N
```

### SERVER
```bash
./install/ddsenabler/examples/action/ddsenabler_example_action server \
	--persistence-path ./install/ddsenabler/examples/persistence/
	<optional> --config $PATH_TO_CONFIG_FILE
	<optional> --expected-requests N
	<optional> --action-name $ACTION_NAME
	<optional> --timeout N
```
