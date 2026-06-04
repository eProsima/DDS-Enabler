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

cmake_minimum_required(VERSION 3.20)
project(ddsenabler_example_discovery LANGUAGES CXX)

# When built standalone (not as a subdirectory of the DDS Enabler project), the
# 'ddsenabler' target does not exist yet, so locate the installed library. The
# imported 'ddsenabler' target links transitively against the packages below, so
# they must be found as well for the standalone build to configure and link.
if(NOT TARGET ddsenabler)
    find_package(cpp_utils REQUIRED)
    find_package(ddspipe_core REQUIRED)
    find_package(ddspipe_participants REQUIRED)
    find_package(ddspipe_yaml REQUIRED)
    find_package(ddsenabler_participants REQUIRED)
    find_package(ddsenabler_yaml REQUIRED)
    find_package(ddsenabler REQUIRED)
endif()

add_executable(ddsenabler_example_discovery main.cpp)

target_link_libraries(ddsenabler_example_discovery PRIVATE ddsenabler)

# Install rule
install(TARGETS ddsenabler_example_discovery
    RUNTIME DESTINATION examples/discovery
)
