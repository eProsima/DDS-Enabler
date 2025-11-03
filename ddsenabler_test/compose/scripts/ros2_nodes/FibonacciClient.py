# Copyright 2025 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
ROS 2 Fibonacci Action Client used for testing.
"""

# Required for import ../utils
import inspect
import os
import sys

currentdir = os.path.dirname(
    os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0, parentdir)

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient

# Prefer action_tutorials_interfaces to match standard tutorial servers
from action_tutorials_interfaces.action import Fibonacci

from utils import print_with_timestamp, sleep_random_time


class FibonacciClient(Node):
    """Fibonacci Action Client node."""

    def __init__(self):
        super().__init__('FibonacciActionClient')
        self._client = ActionClient(self, Fibonacci, 'fibonacci')
        print_with_timestamp('Fibonacci Action Client created.')

    def run(self, samples: int, wait: bool):
        """Send `samples` goals and wait for their results."""
        print_with_timestamp('Client waiting for action server.')
        while not self._client.wait_for_server(timeout_sec=1.0):
            print_with_timestamp('Action server not available yet...')

        print_with_timestamp(f'Running Fibonacci Action Client for {samples} goals.')
        while samples > 0:
            # Optionally wait before sending a goal
            if wait:
                sleep_random_time(0.05, 0.15)

            sequence = self._send_goal_sync(order=10)

            print_with_timestamp(
                'Result { ' + ','.join(map(str, sequence)) + ' }')

            samples -= 1

        return True

    def _send_goal_sync(self, order: int):
        goal_msg = Fibonacci.Goal()
        goal_msg.order = int(order)

        send_future = self._client.send_goal_async(goal_msg)
        rclpy.spin_until_future_complete(self, send_future)
        goal_handle = send_future.result()

        if not goal_handle.accepted:
            print_with_timestamp('Goal rejected')
            return []

        get_result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self, get_result_future)
        result = get_result_future.result().result

        return list(result.sequence)
