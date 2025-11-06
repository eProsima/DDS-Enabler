// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/**
 * @file EnablerConfiguration.cpp
 *
 */

#include <fstream>

#include <nlohmann/json.hpp>

#include <cpp_utils/Log.hpp>
#include <cpp_utils/utils.hpp>

#include <ddspipe_core/configuration/DdsPipeLogConfiguration.hpp>
#include <ddspipe_core/types/dynamic_types/types.hpp>
#include <ddspipe_core/types/topic/filter/ManualTopic.hpp>
#include <ddspipe_core/types/topic/filter/WildcardDdsFilterTopic.hpp>
#include <ddspipe_participants/types/address/Address.hpp>

#include <ddspipe_yaml/yaml_configuration_tags.hpp>
#include <ddspipe_yaml/Yaml.hpp>
#include <ddspipe_yaml/YamlManager.hpp>

#include <ddsenabler_yaml/yaml_configuration_tags.hpp>

#include <ddsenabler_yaml/EnablerConfiguration.hpp>

namespace eprosima {
namespace ddsenabler {
namespace yaml {

using namespace eprosima::ddspipe::core;
using namespace eprosima::ddspipe::core::types;
using namespace eprosima::ddspipe::participants;
using namespace eprosima::ddspipe::participants::types;
using namespace eprosima::ddspipe::yaml;

// Helper method to check if the configuration file is in JSON format
bool is_json(
        const std::string& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;
    }
    catch (nlohmann::json::parse_error& e)
    {
        return false;
    }

    return true;
}

// Helper method to handle nlohmann::json to YAML conversion
YAML::Node convert_json_to_yaml(
        const nlohmann::json& json)
{
    YAML::Node yaml_node;

    for (auto& element : json.items())
    {
        if (element.value().is_object())
        {
            // Recursively convert nested objects
            yaml_node[element.key()] = convert_json_to_yaml(element.value());
        }
        else if (element.value().is_array())
        {
            // Handle arrays
            YAML::Node arrayNode;
            for (const auto& item : element.value())
            {
                arrayNode.push_back(convert_json_to_yaml(item));
            }
            yaml_node[element.key()] = arrayNode;
        }
        else
        {
            // Handle basic data types explicitly
            if (element.value().is_string())
            {
                yaml_node[element.key()] = element.value().get<std::string>();
            }
            else if (element.value().is_number_integer())
            {
                yaml_node[element.key()] = element.value().get<int>();
            }
            else if (element.value().is_number_float())
            {
                yaml_node[element.key()] = element.value().get<double>();
            }
            else if (element.value().is_boolean())
            {
                yaml_node[element.key()] = element.value().get<bool>();
            }
            else
            {
                // Fallback for any other types (like null)
                // Skip element to avoid exception when parsing YAML (YamlReader::is_tag_present expects non-null value)
            }
        }
    }

    return yaml_node;
}

EnablerConfiguration::EnablerConfiguration(
        const std::string& file_path)
{
    if (is_json(file_path))
    {
        load_ddsenabler_configuration_from_json_file_(file_path);
    }
    else
    {
        if (file_path.size() >= 5 &&
                (file_path.substr(file_path.size() - 5) == ".json" ||
                file_path.substr(file_path.size() - 5) == ".JSON"))
        {
            EPROSIMA_LOG_WARNING(DDSENABLER_YAML,
                    "Failed to parse JSON configuration, treating as YAML.");
        }
        load_ddsenabler_configuration_from_yaml_file_(file_path);
    }
}

EnablerConfiguration::EnablerConfiguration(
        const Yaml& yml)
{
    load_ddsenabler_configuration_(yml);
}

bool EnablerConfiguration::is_valid(
        utils::Formatter& error_msg) const noexcept
{
    return true;
}

void EnablerConfiguration::load_ddsenabler_configuration_(
        const Yaml& yml)
{
    try
    {
        YamlReaderVersion version = LATEST;

        ////////////////////////////////////////
        // Create participants configurations //
        ////////////////////////////////////////

        /////
        // Create Simple Participant Configuration
        simple_configuration = std::make_shared<SimpleParticipantConfiguration>();
        simple_configuration->id = "SimpleEnablerParticipant";
        simple_configuration->app_id = "DDS_ENABLER";

        /////
        // Create Enabler Participant Configuration
        enabler_configuration = std::make_shared<participants::EnablerParticipantConfiguration>();
        enabler_configuration->id = "EnablerEnablerParticipant";
        enabler_configuration->app_id = "DDS_ENABLER";

        /////
        // Get optional Enabler configuration options
        if (YamlReader::is_tag_present(yml, ENABLER_ENABLER_TAG))
        {
            auto enabler_yml = YamlReader::get_value_in_tag(yml, ENABLER_ENABLER_TAG);
            load_enabler_configuration_(enabler_yml, version);
        }

        /////
        // Get optional specs configuration
        // WARNING: Parse builtin topics (dds tag) AFTER specs, as some topic-specific default values are set there
        if (YamlReader::is_tag_present(yml, SPECS_TAG))
        {
            auto specs_yml = YamlReader::get_value_in_tag(yml, SPECS_TAG);
            load_specs_configuration_(specs_yml, version);
        }

        /////
        // Get optional DDS configuration options
        if (YamlReader::is_tag_present(yml, ENABLER_DDS_TAG))
        {
            auto dds_yml = YamlReader::get_value_in_tag(yml, ENABLER_DDS_TAG);
            load_dds_configuration_(dds_yml, version);
        }

        // Enable manually after all callbacks are set to avoid missing notifications
        ddspipe_configuration.init_enabled = false;

        // Trigger discovery also with readers in order to enable the publish feature in their associated topics
        // NOTE: discovering a reader is not a requirement for the publish feature, as the related type and topic can be
        // requested to the user if the corresponding callbacks are set.
        ddspipe_configuration.discovery_trigger = DiscoveryTrigger::ANY;
    }
    catch (const std::exception& e)
    {
        throw eprosima::utils::ConfigurationException(
                  utils::Formatter() << "Error loading DDS Enabler configuration from yaml:\n " << e.what());
    }

}

void EnablerConfiguration::load_enabler_configuration_(
        const Yaml& yml,
        const YamlReaderVersion& version)
{
    // Get initial publish wait
    if (YamlReader::is_tag_present(yml, ENABLER_INITIAL_PUBLISH_WAIT_TAG))
    {
        enabler_configuration->initial_publish_wait = YamlReader::get_nonnegative_int(yml,
                        ENABLER_INITIAL_PUBLISH_WAIT_TAG);
    }
}

void EnablerConfiguration::load_specs_configuration_(
        const Yaml& yml,
        const YamlReaderVersion& version)
{
    // Get number of threads
    if (YamlReader::is_tag_present(yml, NUMBER_THREADS_TAG))
    {
        n_threads = YamlReader::get_positive_int(yml, NUMBER_THREADS_TAG);
    }

    /////
    // Get optional Log Configuration
    if (YamlReader::is_tag_present(yml, LOG_CONFIGURATION_TAG))
    {
        ddspipe_configuration.log_configuration = YamlReader::get<DdsPipeLogConfiguration>(yml, LOG_CONFIGURATION_TAG,
                        version);
    }
}

void EnablerConfiguration::load_dds_configuration_(
        const Yaml& yml,
        const YamlReaderVersion& version)
{
    // Get optional DDS domain
    if (YamlReader::is_tag_present(yml, DOMAIN_ID_TAG))
    {
        simple_configuration->domain = YamlReader::get<DomainId>(yml, DOMAIN_ID_TAG, version);
    }

    /////
    // Get optional whitelist interfaces
    if (YamlReader::is_tag_present(yml, WHITELIST_INTERFACES_TAG))
    {
        simple_configuration->whitelist = YamlReader::get_set<IpType>(yml, WHITELIST_INTERFACES_TAG, version);
    }

    // Optional get Transport protocol
    if (YamlReader::is_tag_present(yml, TRANSPORT_DESCRIPTORS_TRANSPORT_TAG))
    {
        simple_configuration->transport = YamlReader::get<TransportDescriptors>(yml,
                        TRANSPORT_DESCRIPTORS_TRANSPORT_TAG, version);
    }
    else
    {
        simple_configuration->transport = TransportDescriptors::builtin;
    }

    // Optional get ignore participant flags
    if (YamlReader::is_tag_present(yml, IGNORE_PARTICIPANT_FLAGS_TAG))
    {
        simple_configuration->ignore_participant_flags = YamlReader::get<IgnoreParticipantFlags>(yml,
                        IGNORE_PARTICIPANT_FLAGS_TAG, version);
    }
    else
    {
        simple_configuration->ignore_participant_flags = IgnoreParticipantFlags::no_filter;
    }

    /////
    // Get optional allowlist
    if (YamlReader::is_tag_present(yml, ALLOWLIST_TAG))
    {
        ddspipe_configuration.allowlist = YamlReader::get_set<utils::Heritable<IFilterTopic>>(yml, ALLOWLIST_TAG,
                        version);
    }

    /////
    // Get optional blocklist
    if (YamlReader::is_tag_present(yml, BLOCKLIST_TAG))
    {
        ddspipe_configuration.blocklist = YamlReader::get_set<utils::Heritable<IFilterTopic>>(yml, BLOCKLIST_TAG,
                        version);
    }

    /////
    // Get optional topics
    if (YamlReader::is_tag_present(yml, TOPICS_TAG))
    {
        const auto& manual_topics = YamlReader::get_list<ManualTopic>(yml, TOPICS_TAG, version);
        ddspipe_configuration.manual_topics = std::vector<ManualTopic>(manual_topics.begin(), manual_topics.end());
    }

    /////
    // Get optional builtin topics
    if (YamlReader::is_tag_present(yml, BUILTIN_TAG))
    {
        // WARNING: Parse builtin topics AFTER specs and enabler, as some topic-specific default values are set there
        ddspipe_configuration.builtin_topics = YamlReader::get_set<utils::Heritable<DistributedTopic>>(yml, BUILTIN_TAG,
                        version);
    }
}

void EnablerConfiguration::load_ddsenabler_configuration_from_yaml_file_(
        const std::string& file_path)
{
    Yaml yml;
    if (!file_path.empty())
    {
        // Load file
        try
        {
            yml = YamlManager::load_file(file_path);
        }
        catch (const std::exception& e)
        {
            throw eprosima::utils::ConfigurationException(
                      utils::Formatter() << "Error loading DDS Enabler configuration from file: <" << file_path <<
                          "> :\n " << e.what());
        }
    }
    else
    {
        EPROSIMA_LOG_WARNING(DDSENABLER_YAML,
                "No configuration file specified, using default values.");
    }
    EnablerConfiguration::load_ddsenabler_configuration_(yml);
}

void EnablerConfiguration::load_ddsenabler_configuration_from_json_file_(
        const std::string& file_path)
{
    Yaml yml;
    if (!file_path.empty())
    {
        try
        {
            // Load the JSON file
            std::ifstream file(file_path);
            if (!file.is_open())
            {
                throw eprosima::utils::ConfigurationException(
                          utils::Formatter() << "Could not open JSON file");
            }

            // Parse the JSON file
            nlohmann::json json;
            file >> json;

            // Close the file
            file.close();

            // Extract the "dds" part which contains "ddsmodule"
            if (json.contains("dds") && json["dds"].is_object())
            {
                if (json["dds"].contains("ddsmodule") && json["dds"]["ddsmodule"].is_object())
                {
                    // Convert the "ddsmodule" content to YAML
                    yml = convert_json_to_yaml(json["dds"]["ddsmodule"]);
                }
                else
                {
                    throw eprosima::utils::ConfigurationException(
                              utils::Formatter() <<
                                  "\"ddsmodule\" not found or is not an object within \"dds\" in the JSON file");
                }
            }
            else
            {
                throw eprosima::utils::ConfigurationException(
                          utils::Formatter() << "\"dds\" not found or is not an object in the JSON file");
            }

        }
        catch (const std::exception& e)
        {
            throw eprosima::utils::ConfigurationException(
                      utils::Formatter() << "Error loading DDS Enabler configuration from file: <" << file_path <<
                          "> :\n " << e.what());
        }
    }
    else
    {
        EPROSIMA_LOG_WARNING(DDSENABLER_YAML,
                "No configuration file specified, using default values.");
    }
    EnablerConfiguration::load_ddsenabler_configuration_(yml);
}

} /* namespace yaml */
} /* namespace ddsenabler */
} /* namespace eprosima */
