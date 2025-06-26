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
 * @file EnablerParticipant.hpp
 */

#pragma once

#include <condition_variable>
#include <map>
#include <mutex>

#include <ddspipe_participants/participant/dynamic_types/SchemaParticipant.hpp>
#include <ddspipe_participants/reader/auxiliar/InternalReader.hpp>

#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/EnablerParticipantConfiguration.hpp>
#include <ddsenabler_participants/library/library_dll.h>

namespace eprosima {
namespace ddsenabler {
namespace participants {

class EnablerParticipant : public ddspipe::participants::SchemaParticipant
{
public:

    DDSENABLER_PARTICIPANTS_DllAPI
    EnablerParticipant(
            std::shared_ptr<EnablerParticipantConfiguration> participant_configuration,
            std::shared_ptr<ddspipe::core::PayloadPool> payload_pool,
            std::shared_ptr<ddspipe::core::DiscoveryDatabase> discovery_database,
            std::shared_ptr<ddspipe::participants::ISchemaHandler> schema_handler);

    DDSENABLER_PARTICIPANTS_DllAPI
    std::shared_ptr<ddspipe::core::IReader> create_reader(
            const ddspipe::core::ITopic& topic) override;

    DDSENABLER_PARTICIPANTS_DllAPI
    bool publish(
            const std::string& topic_name,
            const std::string& json);

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_topic_query_callback(
            participants::DdsTopicQuery callback)
    {
        topic_query_callback_ = callback;
    }

protected:

    std::shared_ptr<ddspipe::participants::InternalReader> lookup_reader_nts_(
            const std::string& topic_name,
            std::string& type_name) const;

    std::shared_ptr<ddspipe::participants::InternalReader> lookup_reader_nts_(
            const std::string& topic_name) const;

    std::map<ddspipe::core::types::DdsTopic, std::shared_ptr<ddspipe::participants::InternalReader>> readers_;

    std::mutex mtx_;

    std::condition_variable cv_;

    DdsTopicQuery topic_query_callback_;
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
