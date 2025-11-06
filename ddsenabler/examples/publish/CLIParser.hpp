// Copyright 2025 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <fastdds/dds/log/Log.hpp>

#pragma once

class CLIParser
{
public:

    CLIParser() = delete;

    struct example_config
    {
        uint32_t expected_types = 0;
        uint32_t expected_topics = 0;
        uint32_t expected_data = 0;
        uint32_t timeout = 30;
        std::string config_file_path = "";
        std::string persistence_path = "";
        std::string publish_path = "";
        std::string publish_topic = "";
        uint32_t publish_period = 500;
        uint32_t publish_initial_wait = 0;
    };

    /**
     * @brief Print usage help message and exit with the given return code
     *
     * @param return_code return code to exit with
     *
     * @warning This method finishes the execution of the program with the input return code
     */
    static void print_help(
            uint8_t return_code)
    {
        std::cout << "Usage: ddsenabler_example_publish [options]"                                      << std::endl;
        std::cout << ""                                                                                 << std::endl;
        std::cout << "--config <str>                        Path to the configuration file"             << std::endl;
        std::cout << "                                      (Default: '')"                              << std::endl;
        std::cout << "--expected-types <num>                Number of types expected to be received"    << std::endl;
        std::cout << "                                      (Default: 0)"                               << std::endl;
        std::cout << "--expected-topics <num>               Number of topics expected to be received"   << std::endl;
        std::cout << "                                      (Default: 0)"                               << std::endl;
        std::cout << "--expected-data <num>                 Number of samples expected to be received"  << std::endl;
        std::cout << "                                      (Default: 0)"                               << std::endl;
        std::cout << "--timeout <num>                       Time (seconds) to wait before stopping the" << std::endl;
        std::cout << "                                      program if expectations are not met"        << std::endl;
        std::cout << "                                      (Default: 30)"                              << std::endl;
        std::cout << "--persistence-path <str>              Path to the persistence directory"          << std::endl;
        std::cout << "                                      (Default: '')"                              << std::endl;
        std::cout << "--publish-path <str>                  Path to the directory with the samples"     << std::endl;
        std::cout << "                                      to be published"                            << std::endl;
        std::cout << "                                      (Default: '')"                              << std::endl;
        std::cout << "--publish-topic <str>                 Topic name in which samples are published"  << std::endl;
        std::cout << "                                      (Default: '')"                              << std::endl;
        std::cout << "--publish-period <num>                Data publication period in milliseconds"    << std::endl;
        std::cout << "                                      (Default: 500)"                             << std::endl;
        std::cout << "--publish-initial-wait <num>          Time (seconds) to wait before starting"     << std::endl;
        std::cout << "                                      data publication"                           << std::endl;
        std::cout << "                                      (Default: 0)"                               << std::endl;
        std::exit(return_code);
    }

    /**
     * @brief Parse the command line options and return the configuration_config object
     *
     * @param argc number of arguments
     * @param argv array of arguments
     * @return configuration_config object with the parsed options
     *
     * @warning This method finishes the execution of the program if the input arguments are invalid
     */
    static example_config parse_cli_options(
            int argc,
            char* argv[])
    {
        example_config config;

        if (argc < 2)
        {
            std::cerr << "Configuration file path is required" << std::endl;
            print_help(EXIT_FAILURE);
        }

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help")
            {
                print_help(EXIT_SUCCESS);
            }
            else if (arg == "--config")
            {
                if (++i < argc)
                {
                    config.config_file_path = argv[i];
                    if (!std::filesystem::exists(config.config_file_path))
                    {
                        std::cerr << "Invalid configuration file path: " << config.config_file_path << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --config argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--expected-types")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.expected_types = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --expected-types argument " << std::string(argv[i]) << ": " <<
                            std::string(e.what()) << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --expected-types argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--expected-topics")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.expected_topics = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --expected-topics argument " << std::string(argv[i]) << ": " <<
                            std::string(e.what()) << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --expected-topics argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--expected-data")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.expected_data = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --expected-data argument " << std::string(argv[i]) << ": " << std::string(
                            e.what()) << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --expected-data argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--timeout")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.timeout = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --timeout argument " << std::string(argv[i]) << ": " << std::string(
                            e.what()) << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --timeout argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--persistence-path")
            {
                if (++i < argc)
                {
                    config.persistence_path = argv[i];
                }
                else
                {
                    std::cerr << "Failed to parse --persistence-path argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--publish-path")
            {
                if (++i < argc)
                {
                    config.publish_path = argv[i];
                    if (!std::filesystem::exists(config.publish_path) ||
                            !std::filesystem::is_directory(config.publish_path))
                    {
                        std::cerr << "Invalid publish path: " << config.publish_path << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --publish-path argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--publish-topic")
            {
                if (++i < argc)
                {
                    config.publish_topic = argv[i];
                }
                else
                {
                    std::cerr << "Failed to parse --publish-topic argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--publish-period")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.publish_period = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --publish-period argument " << std::string(argv[i]) << ": " <<
                            std::string(
                            e.what()) << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --publish-period argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--publish-initial-wait")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.publish_initial_wait = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --publish-initial-wait argument " << std::string(argv[i]) << ": " <<
                            std::string(
                            e.what()) << std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --publish-initial-wait argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "Failed to parse unknown argument: " << arg << std::endl;
                print_help(EXIT_FAILURE);
            }
        }

        // Check that if publish path is set, publish topic is also set
        if (!config.publish_path.empty() && config.publish_topic.empty())
        {
            std::cerr << "Publish topic is required when publish path is set" << std::endl;
            print_help(EXIT_FAILURE);
        }

        return config;
    }

    /**
     * @brief Parse the signal number into the signal name
     *
     * @param signum signal number
     * @return std::string signal name
     */
    static std::string parse_signal(
            const int& signum)
    {
        switch (signum)
        {
            case SIGINT:
                return "SIGINT";
            case SIGTERM:
                return "SIGTERM";
#ifndef _WIN32
            case SIGQUIT:
                return "SIGQUIT";
            case SIGHUP:
                return "SIGHUP";
#endif // _WIN32
            default:
                return "UNKNOWN SIGNAL";
        }
    }

};
