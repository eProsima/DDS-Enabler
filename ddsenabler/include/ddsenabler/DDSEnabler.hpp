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
 * @file DDSEnabler.hpp
 */

#pragma once

#include <memory>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include <cpp_utils/event/FileWatcherHandler.hpp>
#include <cpp_utils/ReturnCode.hpp>
#include <cpp_utils/thread_pool/pool/SlotThreadPool.hpp>

#include <ddspipe_core/core/DdsPipe.hpp>
#include <ddspipe_core/dynamic/AllowedTopicList.hpp>
#include <ddspipe_core/dynamic/DiscoveryDatabase.hpp>
#include <ddspipe_core/dynamic/ParticipantsDatabase.hpp>
#include <ddspipe_core/efficiency/payload/FastPayloadPool.hpp>
#include <ddspipe_core/types/topic/dds/DistributedTopic.hpp>

#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/Handler.hpp>
#include <ddsenabler_participants/HandlerConfiguration.hpp>
#include <ddsenabler_participants/DdsParticipant.hpp>
#include <ddsenabler_participants/EnablerParticipant.hpp>

#include <ddsenabler_yaml/EnablerConfiguration.hpp>

#include <ddsenabler/CallbackSet.hpp>
#include <ddsenabler/library/library_dll.h>

namespace eprosima {
namespace ddsenabler {

/**
 * Wrapper class that encapsulates all dependencies required to launch DDS Enabler.
 */
class DDSEnabler
{
public:

    /**
     * DDSEnabler constructor by required values.
     *
     * Creates DDSEnabler instance with given configuration.
     *
     * @param configuration: Structure encapsulating all enabler configuration options.
     * @param callbacks: Set of callbacks to be used by the enabler.
     */
    DDSENABLER_DllAPI
    DDSEnabler(
            const yaml::EnablerConfiguration& configuration,
            const CallbackSet& callbacks);

    /**
     * Associate the file watcher to the configuration file and establish the callback to reload the configuration.
     *
     * @param file_path: The path to the configuration file.
     *
     * @return \c true if operation was succesful, \c false otherwise.
     */
    DDSENABLER_DllAPI
    bool set_file_watcher(
            const std::string& file_path);

    /**
     * Reconfigure the Enabler with the new configuration.
     *
     * @param new_configuration: The configuration to replace the previous configuration with.
     *
     * @return \c RETCODE_OK if allowed topics list has been updated correctly
     * @return \c RETCODE_NO_DATA if new allowed topics list is the same as the previous one
     * @return \c RETCODE_ERROR if any other error has occurred.
     */
    DDSENABLER_DllAPI
    utils::ReturnCode reload_configuration(
            yaml::EnablerConfiguration& new_configuration);

    /**
     * Publish a JSON message to the specified topic.
     *
     * @param topic_name: The name of the topic to publish to.
     * @param json: The JSON message to publish.
     * @return \c true if the message was published successfully, \c false otherwise.
     */
    DDSENABLER_DllAPI
    bool publish(
            const std::string& topic_name,
            const std::string& json);

protected:

    /**
     * Load the Enabler's internal topics into a configuration object.
     *
     * @param configuration: The configuration to load the internal topics into.
     */
    void load_internal_topics_(
            yaml::EnablerConfiguration& configuration);

    /**
     * Set the internal callbacks used by the enabler.
     *
     * @param callbacks: The set of callbacks to be used by the enabler.
     */
    void set_internal_callbacks_(
            const CallbackSet& callbacks);

    //! Store reference to DomainParticipantFactory to avoid Fast-DDS singletons being destroyed before they should
    std::shared_ptr<eprosima::fastdds::dds::DomainParticipantFactory> part_factory_ =
            eprosima::fastdds::dds::DomainParticipantFactory::get_shared_instance();

    //! Configuration of the DDS Enabler
    yaml::EnablerConfiguration configuration_;

    //! Payload Pool
    std::shared_ptr<ddspipe::core::PayloadPool> payload_pool_;

    //! Thread Pool
    std::shared_ptr<utils::SlotThreadPool> thread_pool_;

    //! Discovery Database
    std::shared_ptr<ddspipe::core::DiscoveryDatabase> discovery_database_;

    //! Participants Database
    std::shared_ptr<ddspipe::core::ParticipantsDatabase> participants_database_;

    //! Handler
    std::shared_ptr<eprosima::ddsenabler::participants::Handler> handler_;

    //! DDS Participant
    std::shared_ptr<eprosima::ddsenabler::participants::DdsParticipant> dds_participant_;

    //! Enabler Participant
    std::shared_ptr<eprosima::ddsenabler::participants::EnablerParticipant> enabler_participant_;

    //! DDS Pipe
    std::unique_ptr<ddspipe::core::DdsPipe> pipe_;

    //! Config File watcher handler
    std::unique_ptr<eprosima::utils::event::FileWatcherHandler> file_watcher_handler_;

    //! Mutex to protect class attributes
    mutable std::mutex mutex_;
};

} /* namespace ddsenabler */
} /* namespace eprosima */
