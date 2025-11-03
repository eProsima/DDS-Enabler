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
ROS 2 Fibonacci Action Server used for testing.
"""

import inspect
import os
import sys
import time

currentdir = os.path.dirname(
    os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0, parentdir)

import rclpy
from rclpy.node import Node
from rclpy.action import ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.executors import MultiThreadedExecutor
import time

# Prefer action_tutorials_interfaces to match standard tutorial servers
from action_tutorials_interfaces.action import Fibonacci

from utils import print_with_timestamp


class FibonacciServer(Node):
    """Fibonacci Action Server node."""

    def __init__(self):
        super().__init__('FibonacciActionServer')

        self._cb_group = ReentrantCallbackGroup()

        self._action_server = ActionServer(
            self,
            Fibonacci,
            'fibonacci',
            self.execute_callback,
            goal_callback=self.goal_callback,
            cancel_callback=self.cancel_callback,
            callback_group=self._cb_group)

        self.goals_executed = 0
        self.goals_canceled = 0
        self.expect_cancel_ = False

        print_with_timestamp('Fibonacci Action Server created.')

    def goal_callback(self, goal_request: Fibonacci.Goal):
        if goal_request.order < 0:
            return GoalResponse.REJECT
        return GoalResponse.ACCEPT

    def cancel_callback(self, goal_handle):
        return CancelResponse.ACCEPT

    async def execute_callback(self, goal_handle):
        """Execute received goal to compute Fibonacci sequence."""
        order = int(goal_handle.request.order)

        # Build Fibonacci sequence
        sequence = []
        if order <= 0:
            sequence = []
        elif order == 1:
            sequence = [0]
        else:
            sequence = [0, 1]
            feedback_msg = Fibonacci.Feedback()
            for i in range(2, order):
                # If cancel requested, cancel goal and return partial sequence
                if goal_handle.is_cancel_requested:
                    goal_handle.canceled()
                    self.goals_canceled += 1
                    result = Fibonacci.Result()
                    result.sequence = sequence
                    print_with_timestamp(
                        'Goal canceled with partial { ' + ','.join(map(str, sequence)) + ' }')
                    return result
                sequence.append(sequence[i - 1] + sequence[i - 2])
                # Publish feedback so clients can track progress
                feedback_msg.partial_sequence = sequence.copy()
                goal_handle.publish_feedback(feedback_msg)
                time.sleep(1)

        if goal_handle.is_cancel_requested:
            goal_handle.canceled()
            self.goals_canceled += 1
            result = Fibonacci.Result()
            result.sequence = sequence
            print_with_timestamp(
                'Goal canceled with partial { ' + ','.join(map(str, sequence)) + ' }')
            return result

        goal_handle.succeed()
        result = Fibonacci.Result()
        result.sequence = sequence

        self.goals_executed += 1

        print_with_timestamp(
            'Result { ' + ','.join(map(str, sequence)) + ' }')

        return result

    def run(self, samples: int, wait: bool, expect_cancel: bool = False):
        """Spin until a number of goals have been processed.

        If expect_cancel is True, count canceled goals towards the target.
        Otherwise, count successfully executed goals.
        """
        self.expect_cancel_ = expect_cancel
        print_with_timestamp(
            f'Running Fibonacci Action Server, waiting for {samples} ' +
            ('canceled' if self.expect_cancel_ else 'executed') + ' goals.')

        executor = MultiThreadedExecutor(num_threads=2)
        executor.add_node(self)

        try:
            while True:
                executor.spin_once(timeout_sec=0.1)
                if self.expect_cancel_:
                    if self.goals_canceled >= samples:
                        break
                else:
                    if self.goals_executed >= samples:
                        break
        finally:
            executor.shutdown()

        time.sleep(0.5)
        return True
