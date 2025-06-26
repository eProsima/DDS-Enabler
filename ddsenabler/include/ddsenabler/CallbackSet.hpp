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

/**
 * @file CallbackSet.hpp
 */

#pragma once

#include <ddsenabler_participants/Callbacks.hpp>

namespace eprosima {
namespace ddsenabler {

/**
 * @brief Struct that encapsulates all the DDS related callbacks used by the DDS Enabler.
 */
struct DdsCallbacks
{
    //! Callback for notifying the reception of DDS types
    participants::DdsTypeNotification type_notification{nullptr};

    //! Callback for notifying the reception of DDS topics
    participants::DdsTopicNotification topic_notification{nullptr};

    //! Callback for notifying the reception of DDS data
    participants::DdsDataNotification data_notification{nullptr};

    //! Callback for requesting information of a DDS type
    participants::DdsTypeQuery type_query{nullptr};

    //! Callback for requesting information of a DDS topic
    participants::DdsTopicQuery topic_query{nullptr};
};

/**
 * @brief Struct that encapsulates all the callbacks used by the DDS Enabler.
 */
struct CallbackSet
{
    //! Callback executed when consuming log messages
    participants::DdsLogFunc log{nullptr};

    //! DDS related callbacks
    DdsCallbacks dds;
};

} /* namespace ddsenabler */
} /* namespace eprosima */
