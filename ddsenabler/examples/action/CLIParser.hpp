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
        std::string config_file_path = "";
        std::string action_name = "fibonacci/_action/";
        bool announce_server = true;
        uint32_t timeout = 30;
        std::string persistence_path = "";
        uint32_t expected_requests = 0;
        uint32_t request_initial_wait = 0;
        bool cancel_requests = false;
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
        std::cout << "Usage: ddsenabler_example_action <client|server> [options]"                                 <<
            std::endl;
        std::cout << ""                                                                                            <<
            std::endl;
        std::cout << "--config <str>                        Path to the configuration file"                        <<
            std::endl;
        std::cout << "                                      (Default: '')"                                         <<
            std::endl;
        std::cout << "--action-name <str>                  Name of the action to be registered"                    <<
            std::endl;
        std::cout << "                                      (Default: 'Fibonacci/_action/')"                       <<
            std::endl;
        std::cout << "--timeout <num>                       Time (seconds) to wait before stopping the"            <<
            std::endl;
        std::cout << "                                      program if expectations are not met"                   <<
            std::endl;
        std::cout << "                                      (Default: 30)"                                         <<
            std::endl;
        std::cout << "--persistence-path <str>              Path to the persistence directory"                     <<
            std::endl;
        std::cout << "                                      (Default: '')"                                         <<
            std::endl;
        std::cout << "\n-------------------------------------SERVER OPTIONS------------------------------------\n" <<
            std::endl;
        std::cout << "--expected-requests <num>              Number of requests expected to be received"           <<
            std::endl;
        std::cout << "\n-------------------------------------CLIENT OPTIONS------------------------------------\n" <<
            std::endl;
        std::cout << "--request-initial-wait <num>          Time (seconds) to wait before starting"                <<
            std::endl;
        std::cout << "                                      requests publication since server matching"            <<
            std::endl;
        std::cout << "                                      (Default: 0)"                                          <<
            std::endl;
        std::cout << "--cancel-requests <bool>              Whether to cancel requests after sending them"         <<
            std::endl;
        std::cout << "                                      (Default: false)"                                      <<
            std::endl;
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
            std::cerr << "Mode is required as the first argument: 'client' or 'server'" << std::endl;
            print_help(EXIT_FAILURE);
        }

        bool client_flag = false;
        bool server_flag = false;

        // First positional argument must be the mode
        std::string mode = argv[1];
        if (mode == "-h" || mode == "--help")
        {
            print_help(EXIT_SUCCESS);
        }
        else if (mode == "client")
        {
            client_flag = true;
            config.announce_server = false;
        }
        else if (mode == "server")
        {
            server_flag = true;
            config.announce_server = true;
        }
        else
        {
            std::cerr << "Invalid mode '" << mode << "'. First argument must be 'client' or 'server'" << std::endl;
            print_help(EXIT_FAILURE);
        }

        // Parse optional arguments starting from index 2
        for (int i = 2; i < argc; ++i)
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
            else if (arg == "--action-name")
            {
                if (++i < argc)
                {
                    config.action_name = argv[i];
                }
                else
                {
                    std::cerr << "Failed to parse --action-name argument" << std::endl;
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
                        std::cerr << "Invalid --timeout argument " << argv[i] << ": " << e.what() << std::endl;
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
            else if (arg == "--expected-requests")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.expected_requests = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --expected-requests argument " << argv[i] << ": " << e.what() <<
                            std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --expected-requests argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--request-initial-wait")
            {
                if (++i < argc)
                {
                    try
                    {
                        config.request_initial_wait = static_cast<uint32_t>(std::stoi(argv[i]));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Invalid --request-initial-wait argument " << argv[i] << ": " << e.what() <<
                            std::endl;
                        print_help(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Failed to parse --request-initial-wait argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else if (arg == "--cancel-requests")
            {
                if (++i < argc)
                {
                    config.cancel_requests = (std::string(argv[i]) == "true") ||
                            (std::string(argv[i]) == "True") ||
                            (std::string(argv[i]) == "1");
                }
                else
                {
                    std::cerr << "Failed to parse --cancel-requests argument" << std::endl;
                    print_help(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "Failed to parse unknown argument: " << arg << std::endl;
                print_help(EXIT_FAILURE);
            }
        }

        // Sanity check (should be set from the positional mode)
        if (!client_flag && !server_flag)
        {
            std::cerr << "Either 'client' or 'server' must be specified as the first argument" << std::endl;
            print_help(EXIT_FAILURE);
        }

        if (config.config_file_path.empty())
        {
            std::cerr << "Configuration file path is required" << std::endl;
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
