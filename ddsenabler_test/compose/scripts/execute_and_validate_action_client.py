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
import re
from typing import List, Tuple

import log

import validation

DESCRIPTION = """Script to validate Fibonacci action client output"""
USAGE = ('python3 execute_and_validate_action_client.py '
         '[-s <samples>] [-t <timeout>] [-e <exe>] [--delay <sec>] [-d]')


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
        help='Number of goal requests to send/expect.'
    )
    parser.add_argument(
        '-t',
        '--timeout',
        type=int,
        default=20,
        help='Timeout for the action client application.'
    )
    parser.add_argument(
        '-e',
        '--exe',
        type=str,
        default='/scripts/ros2_nodes/node_main.py',
        help='Path to the action client executable (python entrypoint).'
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


def _client_command(args):
    """
    Build the command to execute the client.

    :param args: Arguments parsed
    :return: Command to execute the client
    """
    command = [
        'python3', args.exe,
        '--action',
        '--client',
        '--samples', str(args.samples)
    ]

    return command


# ------------------------- Parsing & Validation ------------------------- #

def _extract_number_sequences_from_line(line: str) -> List[int]:
    """Extract a list of ints from a line, supporting formats like:
    - Result { 0,1,1,2 }
    - Result: [0, 1, 1, 2]
    - result -> 0 1 1 2
    The function returns the first reasonable list of ints found.
    """
    # Prefer content inside brackets or braces
    for opening, closing in [('[', ']'), ('{', '}')]:
        if opening in line and closing in line:
            content = line.split(opening, 1)[1].split(closing, 1)[0]
            nums = re.findall(r'-?\d+', content)
            return [int(n) for n in nums]

    # Otherwise, extract from the tail of the line
    tail = line.split(':', 1)[-1]
    nums = re.findall(r'-?\d+', tail)
    return [int(n) for n in nums]


def _client_parse_output(stdout: str, stderr: str) -> Tuple[List[List[int]], str]:
    """
    Transform the output of the program into a list of Fibonacci sequences.

    We look for lines containing the word 'Result' (case-insensitive) and
    extract number sequences from them. Each sequence corresponds to one goal.
    """
    lines = stdout.splitlines()

    sequences: List[List[int]] = []
    for line in lines:
        if 'result' in line.lower():
            seq = _extract_number_sequences_from_line(line)
            if len(seq) >= 2:  # minimal Fibonacci
                sequences.append(seq)

    return sequences, stderr


def _is_fibonacci(seq: List[int]) -> bool:
    """Check that a sequence is a valid Fibonacci progression starting at 0, 1."""
    if len(seq) < 2:
        return False
    if not (seq[0] == 0 and seq[1] == 1):
        return False
    for i in range(2, len(seq)):
        if seq[i] != seq[i - 1] + seq[i - 2]:
            return False
    return True


def _client_validate(stdout_parsed: List[List[int]], stderr_parsed: str):
    # First apply the default validator
    ret_code = validation.validate_default(stdout_parsed, stderr_parsed)
    if ret_code != validation.ReturnCode.SUCCESS:
        return ret_code

    # Ensure we have at least one sequence
    if len(stdout_parsed) == 0:
        log.logger.error('No result sequences were found in client output.')
        return validation.ReturnCode.NOT_VALID_MESSAGES

    # Validate each result sequence as Fibonacci
    for seq in stdout_parsed:
        if not _is_fibonacci(seq):
            log.logger.error(f'Non-Fibonacci sequence detected: {seq}')
            return validation.ReturnCode.NOT_VALID_MESSAGES

    return ret_code


if __name__ == '__main__':

    # Parse arguments
    args = parse_options()

    # Set log level
    if args.debug:
        log.activate_debug()

    command = _client_command(args)

    ret_code = validation.run_and_validate(
        command=command,
        timeout=args.timeout,
        delay=args.delay,
        parse_output_function=_client_parse_output,
        validate_output_function=_client_validate)

    log.logger.info(f'Action client validator exited with code {ret_code}')

    exit(ret_code.value)
