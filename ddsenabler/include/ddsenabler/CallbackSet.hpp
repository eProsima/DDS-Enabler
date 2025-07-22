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

struct ServiceCallbacks
{
    //! Callback for notifying the discovery of DDS services
    participants::ServiceNotification service_notification{nullptr};

    //! Callback for notifying the reception of service requests
    participants::ServiceRequestNotification service_request_notification{nullptr};

    //! Callback for notifying the reception of service replies
    participants::ServiceReplyNotification service_reply_notification{nullptr};

    //! Callback for requesting information of a DDS service
    participants::ServiceQuery service_query{nullptr};
};

struct ActionCallbacks
{
    //! Callback for notifying the discovery of DDS actions
    participants::ActionNotification action_notification{nullptr};

    //! Callback for notifying the reception of action goal requests
    participants::ActionGoalRequestNotification action_goal_request_notification{nullptr};

    //! Callback for notifying the reception of action feedback
    participants::ActionFeedbackNotification action_feedback_notification{nullptr};

    //! Callback for notifying the reception of action cancel requests
    participants::ActionCancelRequestNotification action_cancel_request_notification{nullptr};

    //! Callback for notifying the reception of action results
    participants::ActionResultNotification action_result_notification{nullptr};

    //! Callback for notifying the reception of action status notifications
    participants::ActionStatusNotification action_status_notification{nullptr};

    //! Callback for requesting information of a DDS action
    participants::ActionQuery action_query{nullptr};
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
    ServiceCallbacks service;
    ActionCallbacks action;
};

} /* namespace ddsenabler */
} /* namespace eprosima */
