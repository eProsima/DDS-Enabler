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
 * @file Writer.hpp
 */

#pragma once

#include <map>

#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>

#include <ddspipe_core/types/topic/dds/DdsTopic.hpp>

#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/Message.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

/**
 * @brief Helper class encapsulating the logic to write data, topics and schemas to the user's app.
 *
 * @warning This class is not thread-safe.
 */
class Writer
{

public:

    DDSENABLER_PARTICIPANTS_DllAPI
    Writer() = default;

    DDSENABLER_PARTICIPANTS_DllAPI
    ~Writer() = default;

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_data_notification_callback(
            DdsDataNotification callback)
    {
        data_notification_callback_ = callback;
    }

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_type_notification_callback(
            DdsTypeNotification callback)
    {
        type_notification_callback_ = callback;
    }

    DDSENABLER_PARTICIPANTS_DllAPI
    void set_topic_notification_callback(
            DdsTopicNotification callback)
    {
        topic_notification_callback_ = callback;
    }

    /**
     * @brief Writes the schema of a DynamicType to user's app.
     *
     * @param [in] dyn_type DynamicType containing the type information required.
     * @param [in] type_id TypeIdentifier of the DynamicType.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void write_schema(
            const fastdds::dds::DynamicType::_ref_type& dyn_type,
            const fastdds::dds::xtypes::TypeIdentifier& type_id);

    /**
     * @brief Writes the topic to user's app.
     *
     * @param [in] topic DDS topic to be added.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void write_topic(
            const ddspipe::core::types::DdsTopic& topic);

    /**
     * @brief Writes data.
     *
     * @param [in] msg Pointer to the data to be written.
     * @param [in] dyn_type DynamicType containing the type information required.
     */
    DDSENABLER_PARTICIPANTS_DllAPI
    void write_data(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type);

protected:

    /**
     * @brief Returns the dyn_data of a dyn_type.
     *
     * @param [in] msg Pointer to the data.
     * @param [in] dyn_type DynamicType containing the type information required.
     */
    fastdds::dds::DynamicData::_ref_type get_dynamic_data_(
            const Message& msg,
            const fastdds::dds::DynamicType::_ref_type& dyn_type) noexcept;

    /**
     * @brief Returns the pubsub type of a dyn_type.
     *
     * @param [in] dyn_type DynamicType from which to get the pubsub type.
     * @return The pubsub type associated to the given dyn_type.
     * @note If the pubsub type is not already created, it will be created and stored in the map.
     */
    fastdds::dds::DynamicPubSubType get_pubsub_type_(
            const fastdds::dds::DynamicType::_ref_type& dyn_type) noexcept;

    // Callbacks to notify the user's app
    DdsDataNotification data_notification_callback_;
    DdsTypeNotification type_notification_callback_;
    DdsTopicNotification topic_notification_callback_;

    // Map to store the pubsub types associated to dynamic types so they can be reused
    std::map<fastdds::dds::DynamicType::_ref_type, fastdds::dds::DynamicPubSubType> dynamic_pubsub_types_;
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
