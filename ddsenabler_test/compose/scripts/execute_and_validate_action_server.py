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

import log

import validation

DESCRIPTION = """Script to validate Fibonacci action server output"""
USAGE = ('python3 execute_and_validate_action_server.py '
         '[-s <samples>] [--expect-cancel] [-t <timeout>] [-e <exe>] [--delay <sec>] [-d]')


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
    parser.add_argument(
        '-s',
        '--samples',
        type=int,
        default=3,
        help='Number of goals the server must process before exiting.'
    )
    parser.add_argument(
        '--expect-cancel',
        action='store_true',
        help='Expect goals to be cancelled; count cancellations towards samples.'
    )
    parser.add_argument(
        '-t',
        '--timeout',
        type=int,
        default=20,
        help='Timeout for the action server application.'
    )
    parser.add_argument(
        '-e',
        '--exe',
        type=str,
        default='/scripts/ros2_nodes/node_main.py',
        help='Path to the action server executable (python entrypoint).'
    )
    parser.add_argument(
        '--delay',
        type=float,
        default=0,
        help='Time to wait before starting execution.'
    )
    parser.add_argument(
        '-d',
        '--debug',
        action='store_true',
        help='Print test debugging info.'
    )

    return parser.parse_args()


def _server_command(args):
    """
    Build the command to execute the server.

    :param args: Arguments parsed
    :return: Command to execute the server
    """
    command = [
        'python3', args.exe,
        '--action',
        '--samples', str(args.samples)
    ]
    if args.expect_cancel:
        command.append('--expect-cancel')

    return command


if __name__ == '__main__':

    # Parse arguments
    args = parse_options()

    # Set log level
    if args.debug:
        log.activate_debug()

    command = _server_command(args)

    # Check process exits after serving the expected goals and stderr is empty
    ret_code = validation.run_and_validate(
        command=command,
        timeout=args.timeout,
        delay=args.delay,
        parse_output_function=validation.parse_default,
        validate_output_function=validation.validate_default,
        timeout_as_error=True
    )

    log.logger.info(f'Action server validator exited with code {ret_code}')

    exit(ret_code.value)
