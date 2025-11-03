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

import argparse

import rclpy

# Import service and action nodes lazily to keep optional deps
from AdditionClient import AdditionClient
from AdditionServer import AdditionServer
from FibonacciClient import FibonacciClient
from FibonacciServer import FibonacciServer

DESCRIPTION = """Script to Execute a ROS2 Node (Service or Action)"""
USAGE = ('python3 node_main.py [--action] [--client] [-s 10] [-w]')


def parse_options():
    """
    Parse arguments.

    :return: The arguments parsed.
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        add_help=True,
        description=(DESCRIPTION),
        usage=(USAGE)
    )
    required_args = parser.add_argument_group('role')
    required_args.add_argument(
        '-c',
        '--client',
        action='store_true',
        help='Execute Client instead of Server.'
    )
    parser.add_argument(
        '--action',
        action='store_true',
        help='Run Action nodes (Fibonacci) instead of Service nodes (AddTwoInts).'
    )
    parser.add_argument(
        '--expect-cancel',
        action='store_true',
        help='In Action server mode, expect goals to be cancelled and count cancellations towards samples.'
    )
    parser.add_argument(
        '-s',
        '--samples',
        type=int,
        default=10,
        help='Samples to receive.'
    )
    parser.add_argument(
        '-a',
        '--async',
        action='store_true',
        help='Use async calls.'
    )
    parser.add_argument(
        '-w',
        '--wait',
        action='store_true',
        help='Wait for reply or request.'
    )

    return parser.parse_args()


def main():
    """Execute main function."""
    # Parse arguments
    args = parse_options()

    # Initialize ROS2 RCL py
    rclpy.init()

    # Create Server or Client
    node = None
    if args.action:
        if args.client:
            node = FibonacciClient()
        else:
            node = FibonacciServer()
    else:
        if args.client:
            node = AdditionClient()
        else:
            node = AdditionServer()

    # Run nodes until finish
    if args.action and not args.client:
        # Action server: pass expect-cancel behavior
        result = node.run(args.samples, args.wait, args.expect_cancel)
    else:
        result = node.run(args.samples, args.wait)
    if not result:
        exit(1)

    # Destroy Node and stop ROS2 RCL
    node.destroy_node()
    rclpy.shutdown()

    exit(0)


if __name__ == '__main__':
    main()
