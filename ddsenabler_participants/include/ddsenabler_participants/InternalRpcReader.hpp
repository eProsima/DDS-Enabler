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
 * @file InternalRpcReader.hpp
 */

#pragma once

#include <queue>
#include <memory>

#include <cpp_utils/types/Atomicable.hpp>

#include <ddspipe_core/efficiency/payload/PayloadPool.hpp>
#include <ddspipe_core/types/participant/ParticipantId.hpp>

#include <ddspipe_participants/reader/auxiliar/InternalReader.hpp>
#include <ddspipe_participants/reader/rpc/SimpleReader.hpp>

namespace eprosima {
namespace ddsenabler {
namespace participants {

/**
 * Reader implementation that allows to introduce custom data to DDS Pipe.
 */
class InternalRpcReader : public ddspipe::participants::InternalReader
{
public:

        DDSENABLER_PARTICIPANTS_DllAPI
        InternalRpcReader(
                const ddspipe::core::types::ParticipantId& participant_id,
                const ddspipe::core::types::DdsTopic& topic)
                : topic_(topic),
                  ddspipe::participants::InternalReader(participant_id)
        {

            guid_ = ddspipe::core::types::Guid::new_unique_guid();
        }

        ~InternalRpcReader() = default;

        DDSENABLER_PARTICIPANTS_DllAPI
        ddspipe::core::types::Guid guid() const override
        {
            return guid_;
        }

        DDSENABLER_PARTICIPANTS_DllAPI
        fastdds::RecursiveTimedMutex& get_rtps_mutex() const override
        {
            return rtps_mutex_;
        }

        DDSENABLER_PARTICIPANTS_DllAPI
        uint64_t get_unread_count() const override
        {
            if(unread_count_ == 0)
            {
                return 0;
            }

            return unread_count_--;
        }

        DDSENABLER_PARTICIPANTS_DllAPI
        void simulate_data_reception(
                std::unique_ptr<ddspipe::core::IRoutingData>&& data) noexcept
        {
                std::lock_guard<fastdds::RecursiveTimedMutex> lock(rtps_mutex_);
                unread_count_++;
                ddspipe::participants::InternalReader::simulate_data_reception(std::move(data));
        }

        DDSENABLER_PARTICIPANTS_DllAPI
        ddspipe::core::types::DdsTopic topic() const override
        {
            return topic_;
        }

private:
        ddspipe::core::types::Guid guid_;
        mutable fastdds::RecursiveTimedMutex rtps_mutex_;
        mutable uint64_t unread_count_ = 0;
        ddspipe::core::types::DdsTopic topic_;
};

} /* namespace participants */
} /* namespace ddsenabler */
} /* namespace eprosima */
