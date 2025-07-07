# Service Example Readme

This example demonstrates the usage of the FIWARE DDS Enabler for implementing a service. The example includes a client and a server setup for the desired service.

By default, the example is prepared for the `Fibonacci` service, providing a standard reply with a constant result of the Fibonacci sequence of order 5 for any request.

Users are encouraged to implement their own functional server logic as needed in the `server_specific_logic` function. The Persistence/goals directory must be modified to suit the specific action type and its expected goal requests.

## Example Commands

### CLIENT
```bash
./install/ddsenabler/bin/ddsenabler_example_action --announce-server false --persistence-path <path_to_ws>/FIWARE-DDS-Enabler/ddsenabler/examples/Persistence/ --config <path_to_ws>/FIWARE-DDS-Enabler/ddsenabler/DDS_ENABLER_CONFIGURATION.yaml --request-initial-wait 3  --cancel-requests false
```

### SERVER
```bash
./install/ddsenabler/bin/ddsenabler_example_action --announce-server true --persistence-path <path_to_ws>/FIWARE-DDS-Enabler/ddsenabler/examples/Persistence/ --config <path_to_ws>/FIWARE-DDS-Enabler/ddsenabler/DDS_ENABLER_CONFIGURATION.yaml --expected-requests 3
```