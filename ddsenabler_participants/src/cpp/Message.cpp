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
 * @file Message.cpp
 */

#include <ddsenabler_participants/Message.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

Message::Message(
        const Message& msg)
{
    payload_owner = msg.payload_owner;
    payload_owner->get_payload(
        msg.payload,
        this->payload);
    topic = msg.topic;
    instanceHandle = msg.instanceHandle;
    source_guid = msg.source_guid;
    sequence_number = msg.sequence_number;
    publish_time = msg.publish_time;
}

Message::~Message()
{
    // If payload owner exists and payload has size, release it correctly in pool
    if (payload_owner && payload.length > 0)
    {
        payload_owner->release_payload(payload);
    }
}

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
