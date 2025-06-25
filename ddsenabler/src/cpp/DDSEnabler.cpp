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

#include <cpp_utils/exception/InitializationException.hpp>
#include <cpp_utils/utils.hpp>

#include <ddspipe_core/types/dynamic_types/types.hpp>

#include "ddsenabler/DDSEnabler.hpp"

namespace eprosima {
namespace ddsenabler {

using namespace eprosima::ddspipe::core;
using namespace eprosima::ddspipe::core::types;
using namespace eprosima::ddspipe::participants;
using namespace eprosima::ddspipe::participants::rtps;
using namespace eprosima::ddsenabler::participants;
using namespace eprosima::utils;

DDSEnabler::DDSEnabler(
        const yaml::EnablerConfiguration& configuration,
        const CallbackSet& callbacks)
    : configuration_(configuration)
{
    // Load the Enabler's internal topics from a configuration object.
    load_internal_topics_(configuration_);

    // Create Discovery Database
    discovery_database_ = std::make_shared<DiscoveryDatabase>();

    // Create Payload Pool
    payload_pool_ = std::make_shared<FastPayloadPool>();

    // Create Thread Pool
    thread_pool_ = std::make_shared<SlotThreadPool>(configuration_.n_threads);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create DDS Participant
    dds_participant_ = std::make_shared<DdsParticipant>(
        configuration_.simple_configuration,
        payload_pool_,
        discovery_database_);
    dds_participant_->init();

    // Create Handler
    handler_ = std::make_shared<participants::Handler>(
        handler_config,
        payload_pool_);

    // Create Enabler Participant
    enabler_participant_ = std::make_shared<EnablerParticipant>(
        configuration_.enabler_configuration,
        payload_pool_,
        discovery_database_,
        handler_);

    // Create Participant Database
    participants_database_ = std::make_shared<ParticipantsDatabase>();

    // Populate Participant Database
    participants_database_->add_participant(
        dds_participant_->id(),
        dds_participant_);

    participants_database_->add_participant(
        enabler_participant_->id(),
        enabler_participant_);

    // Create DDS Pipe
    // NOTE: Create disabled, and enable after all callbacks are set to avoid missing notifications
    pipe_ = std::make_unique<DdsPipe>(
        configuration_.ddspipe_configuration,
        discovery_database_,
        payload_pool_,
        participants_database_,
        thread_pool_);

    // Set user defined callbacks in all internal entities requiring it
    set_internal_callbacks_(callbacks);

    // Enable DDS Pipe after having set all callbacks
    if (pipe_->enable() != utils::ReturnCode::RETCODE_OK)
    {
        throw utils::InitializationException(
                  utils::Formatter() << "Failed to enable DDS Pipe.");
    }
}

bool DDSEnabler::set_file_watcher(
        const std::string& file_path)
{
    if (file_path.empty())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to set file watcher. Configuration file path is empty.");
        return false;
    }

    // Callback will reload configuration and pass it to DdsPipe
    // WARNING: it is needed to pass file_path, as FileWatcher only retrieves file_name
    std::function<void(std::string)> file_watcher_callback =
            [this, file_path]
                (std::string file_name)
            {
                EPROSIMA_LOG_INFO(DDSENABLER_EXECUTION,
                        "FileWatcher notified changes in file " << file_path << ". Reloading configuration");
                try
                {
                    eprosima::ddsenabler::yaml::EnablerConfiguration new_configuration(file_path);
                    auto ret = this->reload_configuration(new_configuration);
                    if (ret == utils::ReturnCode::RETCODE_OK)
                    {
                        EPROSIMA_LOG_INFO(DDSENABLER_EXECUTION, "Configuration reloaded successfully");
                    }
                    else if (ret == utils::ReturnCode::RETCODE_NO_DATA)
                    {
                        EPROSIMA_LOG_INFO(DDSENABLER_EXECUTION,
                                "No relevant changes in configuration file " << file_path);
                    }
                    else
                    {
                        EPROSIMA_LOG_WARNING(DDSENABLER_EXECUTION,
                                "Failed to reload configuration from file " << file_path);
                    }
                }
                catch (const std::exception& e)
                {
                    EPROSIMA_LOG_WARNING(DDSENABLER_EXECUTION,
                            "Error reloading configuration file " << file_path << " with error: " << e.what());
                }
            };

    // Creating FileWatcher event handler
    file_watcher_handler_ = std::make_unique<eprosima::utils::event::FileWatcherHandler>(file_watcher_callback,
                    file_path);

    return true;
}

utils::ReturnCode DDSEnabler::reload_configuration(
        yaml::EnablerConfiguration& new_configuration)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Load the Enabler's internal topics from a configuration object.
    load_internal_topics_(new_configuration);

    auto ret = pipe_->reload_configuration(new_configuration.ddspipe_configuration);
    if (ret == utils::ReturnCode::RETCODE_OK)
    {
        // Update the Enabler's configuration
        configuration_ = new_configuration;
    }
    return ret;
}

void DDSEnabler::load_internal_topics_(
        yaml::EnablerConfiguration& configuration)
{
    // Create an internal topic to transmit the dynamic types
    configuration.ddspipe_configuration.builtin_topics.insert(
        utils::Heritable<DdsTopic>::make_heritable(type_object_topic()));

    if (!configuration.ddspipe_configuration.allowlist.empty())
    {
        // The allowlist is not empty. Add the internal topic.
        WildcardDdsFilterTopic internal_topic;
        internal_topic.topic_name.set_value(TYPE_OBJECT_TOPIC_NAME);

        configuration.ddspipe_configuration.allowlist.insert(
            utils::Heritable<WildcardDdsFilterTopic>::make_heritable(internal_topic));
    }
}

void DDSEnabler::set_internal_callbacks_(
        const CallbackSet& callbacks)
{
    if (callbacks.dds.type_notification)
    {
        handler_->set_type_notification_callback(callbacks.dds.type_notification);
    }
    if (callbacks.dds.topic_notification)
    {
        handler_->set_topic_notification_callback(callbacks.dds.topic_notification);
    }
    if (callbacks.dds.data_notification)
    {
        handler_->set_data_notification_callback(callbacks.dds.data_notification);
    }
    if (callbacks.dds.type_query)
    {
        handler_->set_type_query_callback(callbacks.dds.type_query);
    }
    if (callbacks.dds.topic_query)
    {
        enabler_participant_->set_topic_query_callback(callbacks.dds.topic_query);
    }
}

bool DDSEnabler::publish(
        const std::string& topic_name,
        const std::string& json)
{
    return enabler_participant_->publish(topic_name, json);
}

} /* namespace ddsenabler */
} /* namespace eprosima */
