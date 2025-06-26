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

# Done this to set machine architecture and be able to call cmake_utils
enable_language(CXX)

###############################################################################
# Find package cmake_utils
###############################################################################
# Package cmake_utils is required to get every cmake macro needed
find_package(cmake_utils REQUIRED)

###############################################################################
# Project
###############################################################################
# Configure project by info set in project_settings.cmake
# - Load project_settings variables
# - Read version
# - Set installation paths
configure_project()

# Call explicitly project
project(
    ${MODULE_NAME}
    VERSION
        ${MODULE_VERSION}
    DESCRIPTION
        ${MODULE_DESCRIPTION}
)


####################################################################################################
# Build Doxygen documentation
####################################################################################################
if (BUILD_DOCS)
    find_package(ddsenabler REQUIRED)
    get_target_property(ENABLER_INCLUDE_DIR ddsenabler INTERFACE_INCLUDE_DIRECTORIES)

    find_package(ddsenabler_participants REQUIRED)
    get_target_property(PARTICIPANTS_INCLUDE_DIR ddsenabler_participants INTERFACE_INCLUDE_DIRECTORIES)

    message(STATUS "DDS Enabler include directories: ${ENABLER_INCLUDE_DIR} ; ${PARTICIPANTS_INCLUDE_DIR}")
    set(SRC_FILE "${PARTICIPANTS_INCLUDE_DIR}/ddsenabler_participants/Callbacks.hpp")
    set(DST_DIR "${ENABLER_INCLUDE_DIR}/ddsenabler")
    file(COPY ${SRC_FILE} DESTINATION ${DST_DIR})
    # Collect all public headers
    file(GLOB_RECURSE HPP_FILES
        "${ENABLER_INCLUDE_DIR}/ddsenabler/**/*.h*"
    )

    find_package(Doxygen REQUIRED)
    # Create doxygen directories
    add_custom_target(doc-dirs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/doxygen
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/html
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/html/doxygen
        COMMENT "Creating documentation directories" VERBATIM)

    # Doxygen related variables
    set(DOXYGEN_INPUT_DIR
        "${ENABLER_INCLUDE_DIR}/ddsenabler"
    )
    # convert CMake list (;) to space-separated for Doxygen
    string(REPLACE ";" " " DOXYGEN_INPUT_DIR "${DOXYGEN_INPUT_DIR}")

    set(DOXYGEN_OUTPUT_DIR "${PROJECT_BINARY_DIR}/doxygen")
    set(DOXYGEN_INDEX_FILE "${PROJECT_BINARY_DIR}/doxygen/xml/index.xml")
    set(DOXYFILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/code/Doxyfile.in")
    set(DOXYFILE_OUT ${PROJECT_BINARY_DIR}/Doxyfile)

    # Configure doxygen
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    # Doxygen command
    add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        DEPENDS ${HPP_FILES}
        MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
        COMMENT "Generating doxygen documentation")

    # Generate API reference
    add_custom_target(doxygen ALL
        DEPENDS ${DOXYGEN_INDEX_FILE}
        COMMENT "Generated API documentation with doxygen" VERBATIM)
    add_dependencies(doxygen doc-dirs)

    # Install doxygen generated XML files
    install(DIRECTORY ${PROJECT_BINARY_DIR}/doxygen/xml
        DESTINATION doxygen)


    ####################################################################################################
    # Find Sphinx
    ####################################################################################################
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/code/cmake" ${CMAKE_MODULE_PATH})
    find_package(Sphinx REQUIRED)

    ####################################################################################################
    # Build Sphinx documentation
    ####################################################################################################
    # It is possible to build and run tests without generating the documentation output. This saves time
    # while developing, since generating the documentation from the RSTs takes quite some time.
    set(SPHINX_SOURCE "${CMAKE_SOURCE_DIR}/docs")

    # CMake project always build the documentaion using HTML builder. Users can always build with
    # other builders using sphinx directly
    set(DDSENABLER_DOCS_BUILDER html)

    # Generate the sphinx documentation
    add_custom_target(Sphinx ALL
        COMMAND
        ${SPHINX_EXECUTABLE}
        $<$<BOOL:${CMAKE_COMPILE_WARNING_AS_ERROR}>:-W>
        -b ${DDSENABLER_DOCS_BUILDER}
        # Tell Breathe where to find the Doxygen output
        -D breathe_projects.DDSEnabler=${DOXYGEN_OUTPUT_DIR}/xml
        -d "${PROJECT_BINARY_DIR}/doctrees"
        ${SPHINX_SOURCE}
        ${PROJECT_BINARY_DIR}/${DDSENABLER_DOCS_BUILDER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS doxygen
        COMMENT "Generating documentation with Sphinx")

    # Install the generated docs
    install(DIRECTORY ${PROJECT_BINARY_DIR}/${DDSENABLER_DOCS_BUILDER}
        DESTINATION .
        COMPONENT ${DDSENABLER_DOCS_BUILDER}
        PATTERN ".buildinfo" EXCLUDE)
    set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "DDS Enabler docs ${DDSENABLER_DOCS_BUILDER}")
    set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION
        "eProsima DDS Enabler documetation in ${DDSENABLER_DOCS_BUILDER} format")
    set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} ${DDSENABLER_DOCS_BUILDER})
endif()

###############################################################################
# Test
###############################################################################
# Compile tests if CMake options requires it
compile_test_documentation(
    "${PROJECT_SOURCE_DIR}/test" # Test directory
)

###############################################################################
# Packaging
###############################################################################
# Install package
eprosima_packaging()
