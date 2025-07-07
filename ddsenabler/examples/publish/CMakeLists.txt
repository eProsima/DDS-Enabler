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
project(ddsenabler_example_publish LANGUAGES CXX)

add_executable(ddsenabler_example_publish main.cpp)

target_link_libraries(ddsenabler_example_publish PRIVATE ddsenabler)

# Install rule
install(TARGETS ddsenabler_example_publish
    RUNTIME DESTINATION bin
)

# Copy the json files over to the build directory
file(GLOB_RECURSE JSON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
foreach(JSON_FILE_COMPLETE_PATH ${JSON_FILES})
    get_filename_component(JSON_FILE ${JSON_FILE_COMPLETE_PATH} NAME_WE)
    configure_file(
        ${JSON_FILE_COMPLETE_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${JSON_FILE}.json
        COPYONLY)
    install(FILES ${JSON_FILE_COMPLETE_PATH}
        DESTINATION ${DATA_INSTALL_DIR}/ddsenabler/examples/ddsenabler/${BIN_INSTALL_DIR})
endforeach()

# Copy the yaml files over to the build directory
file(GLOB_RECURSE YAML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.yml)
foreach(YAML_FILE_COMPLETE_PATH ${YAML_FILES})
    get_filename_component(YAML_FILE ${YAML_FILE_COMPLETE_PATH} NAME_WE)
    configure_file(
        ${YAML_FILE_COMPLETE_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/${YAML_FILE}.yml
        COPYONLY)
    install(FILES ${YAML_FILE_COMPLETE_PATH}
        DESTINATION ${DATA_INSTALL_DIR}/ddsenabler/examples/ddsenabler/${BIN_INSTALL_DIR})
endforeach()
