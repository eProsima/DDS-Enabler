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
 * @file RpcTypes.hpp
 */

#include <array>
#include <cstdint>
#include <string>

#pragma once

namespace eprosima {
namespace ddsenabler {
namespace participants {

using UUID = std::array<uint8_t, 16>;

enum STATUS_CODE {
        STATUS_UNKNOWN = 0,
        STATUS_ACCEPTED,
        STATUS_EXECUTING,
        STATUS_CANCELING,
        STATUS_SUCCEEDED,
        STATUS_CANCELED,
        STATUS_ABORTED,
        STATUS_REJECTED,
        STATUS_TIMEOUT,
        STATUS_FAILED,
        STATUS_CANCEL_REQUEST_FAILED
    };

enum CANCEL_CODE {
        ERROR_NONE = 0,
        ERROR_REJECTED,
        ERROR_UNKNOWN_GOAL_ID,
        ERROR_GOAL_TERMINATED
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
