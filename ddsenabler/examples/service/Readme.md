# Service Example Readme

This example demonstrates the usage of the DDS Enabler for implementing a service. The example includes a client and a server setup for the desired service.

By default, the example is prepared for the `add_two_ints` service, providing a standard reply with a constant result of 3 for any request.

Users are encouraged to implement their own functional server logic as needed in the `server_specific_logic` function. The persistence/requests directory must be modified to suit the specific service type and its expected requests.

## Example Commands

### CLIENT
```bash
./install/ddsenabler/bin/ddsenabler_example_service -client --persistence-path $PATH_TO_WS/DDS-Enabler/ddsenabler/examples/persistence/ --config $PATH_TO_WS/DDS-Enabler/ddsenabler/DDS_ENABLER_CONFIGURATION.yaml --request-initial-wait 3
```

### SERVER
```bash
./install/ddsenabler/bin/ddsenabler_example_service -server --persistence-path $PATH_TO_WS/DDS-Enabler/ddsenabler/examples/persistence/ --config $PATH_TO_WS/DDS-Enabler/ddsenabler/DDS_ENABLER_CONFIGURATION.yaml --expected-requests 3
```