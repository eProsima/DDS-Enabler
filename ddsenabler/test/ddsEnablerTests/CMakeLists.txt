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
    service_client
    service_server
    action_client
    action_server
)

set(TEST_NEEDED_SOURCES
)

set(TEST_EXTRA_HEADERS
    ${PROJECT_SOURCE_DIR}/test
    ${PROJECT_SOURCE_DIR}/test/ddsEnablerTests/types
    ${PROJECT_SOURCE_DIR}/examples/utils
)

add_blackbox_executable(
    "${TEST_NAME}"
    "${TEST_SOURCES}"
    "${TEST_LIST}"
    "${TEST_NEEDED_SOURCES}"
    "${TEST_EXTRA_HEADERS}"
    "${TEST_LIBRARY_SOURCES}"
)

target_sources(blackbox_DDSEnablerTest PRIVATE ${PROJECT_SOURCE_DIR}/examples/utils/Utils.cpp)
target_include_directories(blackbox_DDSEnablerTest PRIVATE ${PROJECT_SOURCE_DIR}/examples/utils)

# Copy test resources next to the test binary so runtime can use a relative path
add_custom_command(TARGET blackbox_DDSEnablerTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/test/ddsEnablerTests/test_files
            $<TARGET_FILE_DIR:blackbox_DDSEnablerTest>/test_files
)

# Also copy test resources to the test target binary directory (configuration-agnostic)
# Rationale: On single-config generators (Linux), CTest runs from the target dir, which matches
# $<TARGET_FILE_DIR>. On multi-config generators (Windows/MSVC), CTest's working directory is usually
# the CMake current binary dir (e.g., .../ddsEnablerTests) while the executable lives under a
# configuration subfolder (e.g., .../ddsEnablerTests/Debug). The tests locate resources via
# std::filesystem::current_path()/"test_files"; thus, we ensure the resources are available from
# the CTest working dir as well.
add_custom_command(TARGET blackbox_DDSEnablerTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/test/ddsEnablerTests/test_files
            ${CMAKE_CURRENT_BINARY_DIR}/test_files
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
