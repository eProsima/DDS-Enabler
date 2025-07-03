#!/bin/bash

# Start the ROS2 server in the background
source /opt/vulcanexus/jazzy/setup.bash
ros2 run action_tutorials_cpp fibonacci_action_server &
SERVER_PID=$!

# Run for 25 seconds
sleep 25

# Gracefully terminate the server
kill -9 $SERVER_PID

echo "Service server with PID $SERVER_PID terminated after 25 seconds"

# Wait for the process to fully stop
wait $SERVER_PID 2>/dev/null

# Return 0
exit 0
