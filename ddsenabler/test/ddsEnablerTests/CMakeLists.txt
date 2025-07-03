# Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

set(TEST_NAME DDSEnablerTest)

file(
    GLOB_RECURSE TEST_SOURCES
    DdsEnablerTest.cpp
    types/*.cxx
)

file(
    GLOB_RECURSE TEST_LIBRARY_SOURCES
    "${PROJECT_SOURCE_DIR}/src/cpp/*.c*"
)

list(FILTER TEST_LIBRARY_SOURCES EXCLUDE REGEX "/main.cpp")

set(TEST_LIST
    ddsenabler_creation
    ddsenabler_reload_configuration
    send_type1
    send_many_type1
    send_type2
    send_type3
    send_type4
    send_multiple_types
    send_repeated_type
    send_history_bigger_than_writer
    send_history_smaller_than_writer
    send_history_multiple_types
    manual_service_discovery
    manual_request
    manual_reply
    manual_action_discovery
    manual_action_client
    manual_action_client_cancel
    manual_action_server
    manual_action_server_cancel
)

set(TEST_NEEDED_SOURCES
)

set(TEST_EXTRA_HEADERS
    ${PROJECT_SOURCE_DIR}/test
    ${PROJECT_SOURCE_DIR}/test/ddsEnablerTests/types
)

add_blackbox_executable(
    "${TEST_NAME}"
    "${TEST_SOURCES}"
    "${TEST_LIST}"
    "${TEST_NEEDED_SOURCES}"
    "${TEST_EXTRA_HEADERS}"
    "${TEST_LIBRARY_SOURCES}"
)

##################################################################################


set(TEST_NAME ReloadConfig)

file(
    GLOB_RECURSE TEST_SOURCES
    ReloadConfig.cpp
)

set(TEST_LIST
    json
    yaml
)

list(FILTER TEST_LIBRARY_SOURCES EXCLUDE REGEX "/main.cpp")

add_blackbox_executable(
    "${TEST_NAME}"
    "${TEST_SOURCES}"
    "${TEST_LIST}"
    "${TEST_NEEDED_SOURCES}"
    "${TEST_EXTRA_HEADERS}"
    "${TEST_LIBRARY_SOURCES}"
)
