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

#include <cpp_utils/exception/InitializationException.hpp>
#include <cpp_utils/utils.hpp>

#include <ddspipe_core/types/dynamic_types/types.hpp>

#include "ddsenabler/DDSEnabler.hpp"
#include "ddsenabler_participants/RpcUtils.hpp"

namespace eprosima {
namespace ddsenabler {

using namespace eprosima::ddspipe::core;
using namespace eprosima::ddspipe::core::types;
using namespace eprosima::ddspipe::participants;
using namespace eprosima::ddspipe::participants::rtps;
using namespace eprosima::ddsenabler::participants;
using namespace eprosima::utils;

DDSEnabler::DDSEnabler(
        const yaml::EnablerConfiguration& configuration,
        const CallbackSet& callbacks)
    : configuration_(configuration)
{
    // Load the Enabler's internal topics from a configuration object.
    load_internal_topics_(configuration_);

    // Create Discovery Database
    discovery_database_ = std::make_shared<DiscoveryDatabase>();

    // Create Payload Pool
    payload_pool_ = std::make_shared<FastPayloadPool>();

    // Create Thread Pool
    thread_pool_ = std::make_shared<SlotThreadPool>(configuration_.n_threads);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create DDS Participant
    dds_participant_ = std::make_shared<DdsParticipant>(
        configuration_.simple_configuration,
        payload_pool_,
        discovery_database_);
    dds_participant_->init();

    // Create Handler
    handler_ = std::make_shared<participants::Handler>(
        handler_config,
        payload_pool_);

    handler_->set_send_action_get_result_request_callback(
        [this](const std::string& action_name, const UUID& action_id)
        {
            if (this->send_action_get_result_request(action_name, action_id))
            {
                return true;
            }
            this->cancel_action_goal(action_name, action_id, 0);
            return false;
        });

    handler_->set_send_action_send_goal_reply_callback(
        [this](const std::string& action_name, const uint64_t goal_id, bool accepted)
        {
            return this->send_action_send_goal_reply(action_name, goal_id, accepted);
        });

    handler_->set_send_action_get_result_reply_callback(
        [this](const std::string& action_name, const UUID& goal_id, const std::string& reply_json,
        const uint64_t request_id)
        {
            return this->send_action_get_result_reply(action_name, goal_id, reply_json, request_id);
        });

    // Create Enabler Participant
    enabler_participant_ = std::make_shared<EnablerParticipant>(
        configuration_.enabler_configuration,
        payload_pool_,
        discovery_database_,
        handler_);

    // Create Participant Database
    participants_database_ = std::make_shared<ParticipantsDatabase>();

    // Populate Participant Database
    participants_database_->add_participant(
        dds_participant_->id(),
        dds_participant_);

    participants_database_->add_participant(
        enabler_participant_->id(),
        enabler_participant_);

    // Create DDS Pipe
    // NOTE: Create disabled, and enable after all callbacks are set to avoid missing notifications
    pipe_ = std::make_unique<DdsPipe>(
        configuration_.ddspipe_configuration,
        discovery_database_,
        payload_pool_,
        participants_database_,
        thread_pool_);

    // Set user defined callbacks in all internal entities requiring it
    set_internal_callbacks_(callbacks);

    // Enable DDS Pipe after having set all callbacks
    if (pipe_->enable() != utils::ReturnCode::RETCODE_OK)
    {
        throw utils::InitializationException(
                  utils::Formatter() << "Failed to enable DDS Pipe.");
    }
}

bool DDSEnabler::set_file_watcher(
        const std::string& file_path)
{
    if (file_path.empty())
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to set file watcher. Configuration file path is empty.");
        return false;
    }

    // Callback will reload configuration and pass it to DdsPipe
    // WARNING: it is needed to pass file_path, as FileWatcher only retrieves file_name
    std::function<void(std::string)> file_watcher_callback =
            [this, file_path]
                (std::string file_name)
            {
                EPROSIMA_LOG_INFO(DDSENABLER_EXECUTION,
                        "FileWatcher notified changes in file " << file_path << ". Reloading configuration");
                try
                {
                    eprosima::ddsenabler::yaml::EnablerConfiguration new_configuration(file_path);
                    auto ret = this->reload_configuration(new_configuration);
                    if (ret == utils::ReturnCode::RETCODE_OK)
                    {
                        EPROSIMA_LOG_INFO(DDSENABLER_EXECUTION, "Configuration reloaded successfully");
                    }
                    else if (ret == utils::ReturnCode::RETCODE_NO_DATA)
                    {
                        EPROSIMA_LOG_INFO(DDSENABLER_EXECUTION,
                                "No relevant changes in configuration file " << file_path);
                    }
                    else
                    {
                        EPROSIMA_LOG_WARNING(DDSENABLER_EXECUTION,
                                "Failed to reload configuration from file " << file_path);
                    }
                }
                catch (const std::exception& e)
                {
                    EPROSIMA_LOG_WARNING(DDSENABLER_EXECUTION,
                            "Error reloading configuration file " << file_path << " with error: " << e.what());
                }
            };

    // Creating FileWatcher event handler
    file_watcher_handler_ = std::make_unique<eprosima::utils::event::FileWatcherHandler>(file_watcher_callback,
                    file_path);

    return true;
}

utils::ReturnCode DDSEnabler::reload_configuration(
        yaml::EnablerConfiguration& new_configuration)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Load the Enabler's internal topics from a configuration object.
    load_internal_topics_(new_configuration);

    auto ret = pipe_->reload_configuration(new_configuration.ddspipe_configuration);
    if (ret == utils::ReturnCode::RETCODE_OK)
    {
        // Update the Enabler's configuration
        configuration_ = new_configuration;
    }
    return ret;
}

void DDSEnabler::load_internal_topics_(
        yaml::EnablerConfiguration& configuration)
{
    // Create an internal topic to transmit the dynamic types
    configuration.ddspipe_configuration.builtin_topics.insert(
        utils::Heritable<DdsTopic>::make_heritable(type_object_topic()));

    if (!configuration.ddspipe_configuration.allowlist.empty())
    {
        // The allowlist is not empty. Add the internal topic.
        WildcardDdsFilterTopic internal_topic;
        internal_topic.topic_name.set_value(TYPE_OBJECT_TOPIC_NAME);

        configuration.ddspipe_configuration.allowlist.insert(
            utils::Heritable<WildcardDdsFilterTopic>::make_heritable(internal_topic));
    }
}

void DDSEnabler::set_internal_callbacks_(
        const CallbackSet& callbacks)
{
    if (callbacks.dds.type_notification)
    {
        handler_->set_type_notification_callback(callbacks.dds.type_notification);
    }
    if (callbacks.dds.topic_notification)
    {
        handler_->set_topic_notification_callback(callbacks.dds.topic_notification);
    }
    if (callbacks.dds.data_notification)
    {
        handler_->set_data_notification_callback(callbacks.dds.data_notification);
    }
    if (callbacks.dds.type_query)
    {
        handler_->set_type_query_callback(callbacks.dds.type_query);
    }
    if (callbacks.dds.topic_query)
    {
        enabler_participant_->set_topic_query_callback(callbacks.dds.topic_query);
    }
    if (callbacks.service.service_notification)
    {
        handler_->set_service_notification_callback(callbacks.service.service_notification);
    }
    if (callbacks.service.service_request_notification)
    {
        handler_->set_service_request_notification_callback(callbacks.service.service_request_notification);
    }
    if (callbacks.service.service_reply_notification)
    {
        handler_->set_service_reply_notification_callback(callbacks.service.service_reply_notification);
    }
    if (callbacks.service.service_query)
    {
        enabler_participant_->set_service_query_callback(callbacks.service.service_query);
    }
    if (callbacks.action.action_notification)
    {
        handler_->set_action_notification_callback(callbacks.action.action_notification);
    }
    if (callbacks.action.action_goal_request_notification)
    {
        handler_->set_action_goal_request_notification_callback(
            callbacks.action.action_goal_request_notification);
    }
    if (callbacks.action.action_feedback_notification)
    {
        handler_->set_action_feedback_notification_callback(
            callbacks.action.action_feedback_notification);
    }
    if (callbacks.action.action_cancel_request_notification)
    {
        handler_->set_action_cancel_request_notification_callback(
            callbacks.action.action_cancel_request_notification);
    }
    if (callbacks.action.action_result_notification)
    {
        handler_->set_action_result_notification_callback(
            callbacks.action.action_result_notification);
    }
    if (callbacks.action.action_status_notification)
    {
        handler_->set_action_status_notification_callback(
            callbacks.action.action_status_notification);
    }
    if (callbacks.action.action_query)
    {
        enabler_participant_->set_action_query_callback(callbacks.action.action_query);
    }
}

bool DDSEnabler::publish(
        const std::string& topic_name,
        const std::string& json)
{
    return enabler_participant_->publish(topic_name, json);
}

bool DDSEnabler::send_service_request(
        const std::string& service_name,
        const std::string& json,
        uint64_t& request_id,
        participants::RPC_PROTOCOL rpc_protocol)
{
    std::string prefix, suffix;
    switch (rpc_protocol)
    {
        case participants::RPC_PROTOCOL::ROS2:
            prefix = participants::ROS_REQUEST_PREFIX;
            suffix = participants::ROS_REQUEST_SUFFIX;
            break;
        case participants::RPC_PROTOCOL::FASTDDS:
            prefix = participants::FASTDDS_REQUEST_PREFIX;
            suffix = participants::FASTDDS_REQUEST_SUFFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send service request to service " << service_name << ": unknown RPC protocol.");
            return false;
    }

    request_id = handler_->get_new_request_id();
    if (!enabler_participant_->publish_rpc(
                prefix + service_name + suffix,
                json,
                request_id))
    {
        return false;
    }

    return true;
}

bool DDSEnabler::announce_service(
        const std::string& service_name,
        participants::RPC_PROTOCOL rpc_protocol)
{
    return enabler_participant_->announce_service(service_name, rpc_protocol);
}

bool DDSEnabler::revoke_service(
        const std::string& service_name)
{
    return enabler_participant_->revoke_service(service_name);
}

bool DDSEnabler::send_service_reply(
        const std::string& service_name,
        const std::string& json,
        const uint64_t request_id)
{
    RPC_PROTOCOL rpc_protocol = enabler_participant_->get_service_rpc_protocol(service_name);
    std::string prefix, suffix;
    switch (rpc_protocol)
    {
        case participants::RPC_PROTOCOL::ROS2:
            prefix = participants::ROS_REPLY_PREFIX;
            suffix = participants::ROS_REPLY_SUFFIX;
            break;
        case participants::RPC_PROTOCOL::FASTDDS:
            prefix = participants::FASTDDS_REPLY_PREFIX;
            suffix = participants::FASTDDS_REPLY_SUFFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send service reply to unknown service " << service_name);
            return false;
    }

    return enabler_participant_->publish_rpc(
        prefix + service_name + suffix,
        json,
        request_id);
}

bool DDSEnabler::send_action_goal(
        const std::string& action_name,
        const std::string& json,
        UUID& action_id,
        participants::RPC_PROTOCOL rpc_protocol)
{
    std::string goal_json = RpcUtils::create_goal_request_msg(json, action_id);
    std::string goal_request_topic = action_name + participants::ACTION_GOAL_SUFFIX;
    uint64_t goal_request_id = 0;

    if (!send_service_request(
                goal_request_topic,
                goal_json,
                goal_request_id,
                rpc_protocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action goal request to action " << action_name);
        return false;
    }

    if (!handler_->store_action_request(
                action_name,
                action_id,
                goal_request_id,
                ACTION_TYPE::GOAL,
                rpc_protocol))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to store action goal request to action " << action_name);
        handler_->erase_action_UUID(action_id, ActionEraseReason::FORCED);
        return false;
    }

    return true;
}

bool DDSEnabler::send_action_get_result_request(
        const std::string& action_name,
        const UUID& action_id)
{
    std::string json = participants::RpcUtils::create_result_request_msg(action_id);

    std::string get_result_request_topic = action_name + participants::ACTION_RESULT_SUFFIX;
    uint64_t get_result_request_id = 0;

    if (!send_service_request(
                get_result_request_topic,
                json,
                get_result_request_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action get result request to action " << action_name);
        return false;
    }

    if (!handler_->store_action_request(
                action_name,
                action_id,
                get_result_request_id,
                ACTION_TYPE::RESULT))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to store action get result request to action " << action_name
                                                                       << ": cancelling.");
        cancel_action_goal(action_name, action_id, 0);
        return false;
    }

    return true;
}

bool DDSEnabler::cancel_action_goal(
        const std::string& action_name,
        const participants::UUID& goal_id,
        const int64_t timestamp)
{
    if (goal_id != participants::UUID() &&
            !handler_->is_UUID_active(action_name, goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to cancel action goal for action " << action_name
                                                           << ": goal id not found.");
        return false;
    }

    std::string cancel_json = participants::RpcUtils::create_cancel_request_msg(goal_id, timestamp);

    uint64_t cancel_request_id = 0;
    std::string cancel_request_topic = action_name + participants::ACTION_CANCEL_SUFFIX;

    if (send_service_request(
                cancel_request_topic,
                cancel_json,
                cancel_request_id))
    {
        return true;
    }

    EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
            "Failed to send action cancel goal to action " << action_name);
    return false;
}

bool DDSEnabler::announce_action(
        const std::string& action_name,
        participants::RPC_PROTOCOL rpc_protocol)
{
    return enabler_participant_->announce_action(action_name, rpc_protocol);
}

bool DDSEnabler::revoke_action(
        const std::string& action_name)
{
    return enabler_participant_->revoke_action(action_name);
}

void DDSEnabler::send_action_send_goal_reply(
        const std::string& action_name,
        const uint64_t goal_id,
        bool accepted)
{
    std::string reply_json = participants::RpcUtils::create_goal_reply_msg(accepted);

    if (!send_service_reply(
                action_name + participants::ACTION_GOAL_SUFFIX,
                reply_json,
                goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action goal reply to action " << action_name
                                                              << ": goal id not found.");
    }
    return;
}

bool DDSEnabler::send_action_cancel_goal_reply(
        const char* action_name,
        const std::vector<participants::UUID>& goal_ids,
        const participants::CANCEL_CODE& cancel_code,
        const uint64_t request_id)
{
    std::vector<std::pair<participants::UUID, std::chrono::system_clock::time_point>> cancelling_goals;
    for (const auto& goal_id : goal_ids)
    {
        std::chrono::system_clock::time_point timestamp;
        if (!handler_->is_UUID_active(action_name, goal_id, &timestamp))
        {
            continue;
        }

        cancelling_goals.emplace_back(goal_id, timestamp);
    }
    std::string reply_json = participants::RpcUtils::create_cancel_reply_msg(cancelling_goals, cancel_code);

    if (!send_service_reply(
                std::string(action_name) + participants::ACTION_CANCEL_SUFFIX,
                reply_json,
                request_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action cancel reply to action " << action_name
                                                                << ": request id not found.");
        return false;
    }
    return true;
}

bool DDSEnabler::send_action_result(
        const char* action_name,
        const participants::UUID& goal_id,
        const participants::STATUS_CODE& status_code,
        const char* json)
{
    if (!handler_->is_UUID_active(action_name, goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action result to action " << action_name
                                                          << ": goal id not found.");
        return false;
    }

    std::string reply_json = participants::RpcUtils::create_result_reply_msg(status_code, json);

    return handler_->handle_action_result(action_name, goal_id, reply_json);
}

bool DDSEnabler::send_action_get_result_reply(
        const std::string& action_name,
        const participants::UUID& goal_id,
        const std::string& reply_json,
        const uint64_t request_id)
{
    std::string result_topic = action_name + participants::ACTION_RESULT_SUFFIX;

    if (send_service_reply(
                result_topic,
                reply_json,
                request_id))
    {
        handler_->erase_action_UUID(goal_id, ActionEraseReason::FORCED);
        return true;
    }

    return false;
}

bool DDSEnabler::send_action_feedback(
        const char* action_name,
        const char* json,
        const participants::UUID& goal_id)
{
    if (!handler_->is_UUID_active(action_name, goal_id))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to send action feedback to action " << action_name
                                                            << ": goal id not found.");
        return false;
    }

    RPC_PROTOCOL protocol = handler_->get_action_rpc_protocol(action_name, goal_id);

    std::string prefix;
    switch (protocol)
    {
        case RPC_PROTOCOL::ROS2:
            prefix = participants::ROS_TOPIC_PREFIX;
            break;
        case RPC_PROTOCOL::FASTDDS:
            prefix = participants::FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send feedback to action " << action_name
                                                         << ": unsupported RPC protocol.");
            return false;
    }

    std::string feedback_json = participants::RpcUtils::create_feedback_msg(json, goal_id);
    std::string feedback_topic = prefix + std::string(action_name) + participants::ACTION_FEEDBACK_SUFFIX;

    return enabler_participant_->publish(feedback_topic, feedback_json);
}

bool DDSEnabler::update_action_status(
        const std::string& action_name,
        const participants::UUID& goal_id,
        const participants::STATUS_CODE& status_code)
{
    std::chrono::system_clock::time_point goal_accepted_stamp;
    if (!handler_->is_UUID_active(action_name, goal_id, &goal_accepted_stamp))
    {
        EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                "Failed to update action status to action " << action_name
                                                            << ": goal id not found.");
        return false;
    }

    RPC_PROTOCOL protocol = handler_->get_action_rpc_protocol(action_name, goal_id);

    std::string prefix;
    switch (protocol)
    {
        case RPC_PROTOCOL::ROS2:
            prefix = participants::ROS_TOPIC_PREFIX;
            break;
        case RPC_PROTOCOL::FASTDDS:
            prefix = participants::FASTDDS_TOPIC_PREFIX;
            break;
        default:
            EPROSIMA_LOG_ERROR(DDSENABLER_EXECUTION,
                    "Failed to send status to action " << action_name
                                                       << ": unsupported RPC protocol.");
            return false;
    }

    std::string status_json = participants::RpcUtils::create_status_msg(goal_id, status_code, goal_accepted_stamp);
    std::string status_topic = prefix + action_name + participants::ACTION_STATUS_SUFFIX;
    return enabler_participant_->publish(status_topic, status_json);
}

} /* namespace ddsenabler */
} /* namespace eprosima */
