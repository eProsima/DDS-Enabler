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
// limitations under the License\.

#pragma once

#include <ddspipe_participants/configuration/ParticipantConfiguration.hpp>

#include <ddsenabler_participants/library/library_dll.h>

namespace eprosima {
namespace ddsenabler {
namespace participants {

/**
 * This data struct represents a configuration for a EnablerParticipant
 */
struct EnablerParticipantConfiguration : public ddspipe::participants::ParticipantConfiguration
{
public:

    /////////////////////////
    // CONSTRUCTORS
    /////////////////////////

    DDSENABLER_PARTICIPANTS_DllAPI
    EnablerParticipantConfiguration() = default;

    /////////////////////////
    // VARIABLES
    /////////////////////////

    unsigned int initial_publish_wait {0u};
};

} /* namespace participants */
} /* namespace ddspipe */
} /* namespace eprosima */
