# Action Example Readme

This example demonstrates the usage of the DDS Enabler for implementing an action. The example includes a client and a server setup for the desired action.

By default, the example is prepared for the `Fibonacci` action, providing a standard reply with a constant result of the Fibonacci sequence of order 5 for any request.

Users are encouraged to implement their own functional server logic as needed in the `server_specific_logic` function. The persistence/goals directory must be modified to suit the specific action type and its expected goal requests.

## Example Commands

### CLIENT
```bash
./install/ddsenabler/bin/ddsenabler_example_action --announce-server false --persistence-path <path_to_ws>/DDS-Enabler/ddsenabler/examples/persistence/ --config <path_to_ws>/DDS-Enabler/ddsenabler/DDS_ENABLER_CONFIGURATION.yaml --request-initial-wait 3  --cancel-requests false
```

### SERVER
```bash
./install/ddsenabler/bin/ddsenabler_example_action --announce-server true --persistence-path <path_to_ws>/DDS-Enabler/ddsenabler/examples/persistence/ --config <path_to_ws>/DDS-Enabler/ddsenabler/DDS_ENABLER_CONFIGURATION.yaml --expected-requests 3
```