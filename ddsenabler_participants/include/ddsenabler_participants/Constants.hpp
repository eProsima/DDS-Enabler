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
 * @file Constants.hpp
 */

#pragma once

namespace eprosima {
namespace ddsenabler {
namespace participants {

// QoS serialization
constexpr const char* QOS_SERIALIZATION_RELIABILITY("reliability");
constexpr const char* QOS_SERIALIZATION_DURABILITY("durability");
constexpr const char* QOS_SERIALIZATION_OWNERSHIP("ownership");
constexpr const char* QOS_SERIALIZATION_KEYED("keyed");

// Topic mangling
constexpr const char* ROS_TOPIC_PREFIX("rt/");
constexpr const char* FASTDDS_TOPIC_PREFIX("");

// Service mangling
constexpr const char* ROS_REQUEST_PREFIX("rq/");
constexpr const char* ROS_REQUEST_SUFFIX("Request");
constexpr const char* ROS_REPLY_PREFIX("rr/");
constexpr const char* ROS_REPLY_SUFFIX("Reply");

constexpr const char* FASTDDS_REQUEST_PREFIX("");
constexpr const char* FASTDDS_REQUEST_SUFFIX("_Request");
constexpr const char* FASTDDS_REPLY_PREFIX("");
constexpr const char* FASTDDS_REPLY_SUFFIX("_Reply");

// Action mangling
constexpr const char* ACTION_GOAL_SUFFIX("send_goal");
constexpr const char* ACTION_RESULT_SUFFIX("get_result");
constexpr const char* ACTION_CANCEL_SUFFIX("cancel_goal");
constexpr const char* ACTION_FEEDBACK_SUFFIX("feedback");
constexpr const char* ACTION_STATUS_SUFFIX("status");

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
