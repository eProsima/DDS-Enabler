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
project(ddsenabler_example_service LANGUAGES CXX)

add_executable(ddsenabler_example_service main.cpp)

target_include_directories(ddsenabler_example_service PRIVATE ${PROJECT_SOURCE_DIR}/../utils)
target_link_libraries(ddsenabler_example_service PRIVATE ddsenabler utils)

# Install rule
install(TARGETS ddsenabler_example_service
    RUNTIME DESTINATION examples/service
)

# Install requests directory
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/requests
    DESTINATION examples/service
)
