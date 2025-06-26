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
 * @file Message.hpp
 */

#pragma once

#include <ddspipe_core/types/dds/Payload.hpp>
#include <ddspipe_core/types/data/RtpsPayloadData.hpp>
#include <ddspipe_core/efficiency/payload/PayloadPool.hpp>
#include <ddspipe_core/types/topic/dds/DdsTopic.hpp>

#include <ddsenabler_participants/library/library_dll.h>

namespace eprosima {
namespace ddsenabler {
namespace participants {

/**
 * Structure with Fast DDS payload and its owner (a \c PayloadPool).
 */
struct Message
{
    DDSENABLER_PARTICIPANTS_DllAPI
    Message() = default;

    /**
     * Message copy constructor
     *
     * Copy message without copying payload through PayloadPool API (copy reference and increment counter).
     *
     * @note If using instead the default destructor and copy constructor, the destruction of the copied message would
     * free the newly constructed sample (payload's data attribute), thus rendering the latter useless.
     *
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    Message(
            const Message& data);

    /**
     * Message destructor
     *
     * Releases internal payload, decrementing its reference count and freeing only when no longer referenced.
     *
     * @note Releasing the payload correctly sets payload's internal data attribute to \c nullptr , which eludes
     * the situation described in copy constructor's note.
     *
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    ~Message();

    //! DdsTopic
    ddspipe::core::types::DdsTopic topic;

    //! Instance of the message (default no instance)
    ddspipe::core::types::InstanceHandle instanceHandle{};

    //! Guid of the source entity that has transmit the data
    ddspipe::core::types::Guid source_guid{};

    //! Serialized payload
    fastdds::rtps::SerializedPayload_t payload{};

    //! Payload owner (reference to \c PayloadPool which created/reserved it)
    ddspipe::core::PayloadPool* payload_owner{nullptr};

    //! Timestamp when this message was initially published.
    eprosima::ddspipe::core::types::DataTime publish_time;

    //! Unique sequence number assigned to received messages.
    unsigned int sequence_number;
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
