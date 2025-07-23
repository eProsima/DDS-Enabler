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
 * @file DDSEnabler.hpp
 */

#pragma once

#include <memory>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include <cpp_utils/event/FileWatcherHandler.hpp>
#include <cpp_utils/ReturnCode.hpp>
#include <cpp_utils/thread_pool/pool/SlotThreadPool.hpp>

#include <ddspipe_core/core/DdsPipe.hpp>
#include <ddspipe_core/dynamic/AllowedTopicList.hpp>
#include <ddspipe_core/dynamic/DiscoveryDatabase.hpp>
#include <ddspipe_core/dynamic/ParticipantsDatabase.hpp>
#include <ddspipe_core/efficiency/payload/FastPayloadPool.hpp>
#include <ddspipe_core/types/topic/dds/DistributedTopic.hpp>

#include <ddsenabler_participants/Callbacks.hpp>
#include <ddsenabler_participants/Handler.hpp>
#include <ddsenabler_participants/HandlerConfiguration.hpp>
#include <ddsenabler_participants/DdsParticipant.hpp>
#include <ddsenabler_participants/EnablerParticipant.hpp>

#include <ddsenabler_yaml/EnablerConfiguration.hpp>

#include <ddsenabler/CallbackSet.hpp>
#include <ddsenabler/library/library_dll.h>

namespace eprosima {
namespace ddsenabler {

/**
 * Wrapper class that encapsulates all dependencies required to launch DDS Enabler.
 */
class DDSEnabler
{
public:

    /**
     * DDSEnabler constructor by required values.
     *
     * Creates DDSEnabler instance with given configuration.
     *
     * @param configuration: Structure encapsulating all enabler configuration options.
     * @param callbacks: Set of callbacks to be used by the enabler.
     */
    DDSENABLER_DllAPI
    DDSEnabler(
            const yaml::EnablerConfiguration& configuration,
            const CallbackSet& callbacks);

    /**
     * Associate the file watcher to the configuration file and establish the callback to reload the configuration.
     *
     * @param file_path: The path to the configuration file.
     *
     * @return \c true if operation was succesful, \c false otherwise.
     */
    DDSENABLER_DllAPI
    bool set_file_watcher(
            const std::string& file_path);

    /**
     * Reconfigure the Enabler with the new configuration.
     *
     * @param new_configuration: The configuration to replace the previous configuration with.
     *
     * @return \c RETCODE_OK if allowed topics list has been updated correctly
     * @return \c RETCODE_NO_DATA if new allowed topics list is the same as the previous one
     * @return \c RETCODE_ERROR if any other error has occurred.
     */
    DDSENABLER_DllAPI
    utils::ReturnCode reload_configuration(
            yaml::EnablerConfiguration& new_configuration);

    /**
     * Publish a JSON message to the specified topic.
     *
     * @param topic_name: The name of the topic to publish to.
     * @param json: The JSON message to publish.
     * @return \c true if the message was published successfully, \c false otherwise.
     */
    DDSENABLER_DllAPI
    bool publish(
            const std::string& topic_name,
            const std::string& json);

    /*****************************************/
    /*               SERVICE                 */
    /*****************************************/

    /**
     * @brief Creates a server for the given service.
     *
     * This function announces a service by setting up a server for it.
     * It returns a boolean indicating whether the operation was successful.
     * Failure may occur if there is an issue requesting the data types to user's app
     * for the corresponding request and reply topics.
     *
     * @param service_name The name of the service to be announced.
     * @return true if the service was successfully announced, false otherwise.
     */
    bool announce_service(
            const std::string& service_name);

    /**
     * @brief Stops the server for the given service.
     *
     * This function revokes the server associated with the specified service.
     * It returns a boolean indicating whether the operation was successful.
     * The operation will fail if the service was not previously announced.
     *
     * @param service_name The name of the service to be stopped.
     * @return true if the service was successfully stopped, false otherwise.
     */
    bool revoke_service(
            const std::string& service_name);

    /**
     * @brief Sends a request to a specified service.
     *
     * Sends a request containing the given JSON data to the specified service.
     * If successful, the request identifier is stored in the provided reference
     * and will match the one received in the reply callback.
     *
     * The function fails if there is no server for the service or if the request is malformed.
     * The request identifier is incremented sequentially across all services.
     *
     * @param service_name The target service name.
     * @param json The JSON-formatted request data.
     * @param request_id Reference to store the unique request identifier.
     * @return true if the request was successfully sent, false otherwise.
     */
    bool send_service_request(
            const std::string& service_name,
            const std::string& json,
            uint64_t& request_id);

    /**
     * @brief Sends a reply to the given service.
     *
     * This function sends a reply to the specified service with the provided JSON data.
     * It returns a boolean indicating whether the operation was successful.
     * Failure may occur if the service was not previously created or if the request ID does not match any request.
     *
     * @param service_name The name of the service to send the reply to.
     * @param json The JSON data to be sent with the reply.
     * @param request_id The unique identifier of the request to which this reply corresponds.
     * @return true if the reply was successfully sent, false otherwise.
     *
     * @note The request_id must coincide with the one received in the request.
     */
    bool send_service_reply(
            const std::string& service_name,
            const std::string& json,
            const uint64_t request_id);

    /*****************************************/
    /*            ACTION SERVER              */
    /*****************************************/

    /**
     * @brief Creates server for the given action.
     *
     * This function announces an action by setting up a server for it.
     * It returns a boolean indicating whether the operation was successful.
     * Failure may occur if there is an issue requesting the data types to user's app.
     *
     * @param action_name The name of the action to be announced.
     *
     * @return true if the action was successfully announced, false otherwise.
     */
    bool announce_action(
            const std::string& action_name);

    /**
     * @brief Stops the server for the given action.
     *
     * This function revokes the server associated with the specified action.
     * It returns a boolean indicating whether the operation was successful.
     * Failure may occur if the action was not previously announced.
     *
     * @param action_name The name of the action to be stopped.
     *
     * @return true if the action was successfully stopped, false otherwise.
     */
    bool revoke_action(
            const std::string& action_name);

    /**
     * @brief Send feedback for the specified goal_id action.
     *
     * This function sends feedback for the specified action goal identified by the given goal ID.
     * It returns a boolean indicating whether the operation was successful.
     *
     * @param action_name The name of the action for which the feedback is being sent.
     * @param json The JSON data to be sent as feedback.
     * @param goal_id The unique identifier of the action goal for which the feedback is being sent.
     *
     * @return true if the feedback was successfully sent, false otherwise.
     */
    bool send_action_feedback(
        const char* action_name,
        const char* json,
        const participants::UUID& goal_id);

    /**
     * @brief Send result for the specified goal_id action.
     *
     * This function sends the result for the specified action goal identified by the given goal ID.
     * It returns a boolean indicating whether the operation was successful.
     *
     * @param action_name The name of the action for which the result is being sent.
     * @param goal_id The unique identifier of the action goal for which the result is being sent.
     * @param status_code The status code representing the result of the action.
     * @param json The JSON data to be sent as the result.
     *
     * @return true if the result was successfully sent, false otherwise.
     */
    bool send_action_result(
        const char* action_name,
        const participants::UUID& goal_id,
        const participants::STATUS_CODE& status_code,
        const char* json);

    bool send_action_cancel_goal_reply(
        const char* action_name,
        const std::vector<participants::UUID>& goal_ids,
        const participants::CANCEL_CODE& cancel_code,
        const uint64_t request_id);

     /**
      * @brief Publishes an update for the status of an action.
      *
      * This function publishes a msg update for the status of an action.
      * It returns a boolean indicating whether the operation was successful.
      * Failure may occur if the action was not previously announced or if the goal ID is not currently active.
      *
      * @param action_name The name of the action for which the status is being updated.
      * @param goal_id The unique identifier of the action goal for which the status is being updated.
      * @param status_code The status code representing the current state of the action.
      *
      * @return true if the status update was successfully sent, false otherwise.
      *
     */
    bool update_action_status(
        const std::string& action_name,
        const participants::UUID& goal_id,
        const participants::STATUS_CODE& status_code);

    /*****************************************/
    /*            ACTION CLIENT              */
    /*****************************************/

    /**
     * @brief Sends an action goal to the specified action.
     *
     * This function sends an action goal to the specified action with the provided JSON data.
     * It returns a boolean indicating whether the operation was successful.
     * Failure may occur if the action server was not previously created, if the action types are unknown
     * or if the action goal is malformed.
     *
     * The goal_id is a unique identifier for the action goal and is generated by the function.
     *
     * @param action_name The name of the action to send the goal to.
     * @param json The JSON data to be sent with the action goal.
     * @param goal_id Reference to store the unique identifier of the action goal.
     * @return true if the action goal was successfully sent, false otherwise.
     */
    bool send_action_goal(
	    const std::string& action_name,
	    const std::string& json,
            participants::UUID& goal_id);

    /**
     * @brief Cancels an action goal for the specified action.
     *
     * This function sends a request to an action server to cancel:
     *     If the goal ID is empty and timestamp is zero, cancel all goals
     *     If the goal ID is empty and timestamp is not zero, cancel all goals accepted at or before the timestamp
     *     If the goal ID is not empty and timestamp is zero, cancel the goal with the given ID regardless of the time it was accepted
     *     If the goal ID is not empty and timestamp is not zero, cancel the goal with the given ID and all goals accepted at or before the timestamp
     * The success of the operation only indicates that the cancel request was sent, not that it was processed.
     * The actual cancellation of the goals would be notified via a status update of each individual action.
     *
     * @param action_name The name of the action for which the goal is to be canceled.
     * @param goal_id The unique identifier of the action goal to be canceled or empty to cancel all goals.
     * @param timestamp The acceptance timestamp of the goal to be canceled.
     *
     * @return true if the cancel action request was successfully sent, false otherwise.
     */
    bool cancel_action_goal(
            const std::string& action_name,
            const participants::UUID& goal_id,
            const int64_t timestamp = 0);

protected:

    /**
     * @brief Sends a request to get the result of an action.
     *
     * This function sends a request to get the result of an action identified by the given action name and goal ID.
     * It returns a boolean indicating whether the operation was successful.
     *
     * It is invoked via lambda from the writer_ when the send_goal_reply is received and positive.
     *
     * @param action_name The name of the action for which the result is being requested.
     * @param goal_id The unique identifier of the action goal for which the result is being requested.
     *
     * @return true if the request to get the result was successfully sent, false otherwise.
     */
    bool send_action_get_result_request(
        const std::string& action_name,
        const participants::UUID& goal_id);

    /**
     * @brief Sends a reply to an action goal request.
     *
     * This function sends a reply to an action goal request, indicating whether the goal was accepted or not.
     *
     * It is invoked via lambda from the writer_ when request is notified.
     *
     * @param action_name The name of the action for which the goal reply is being sent.
     * @param goal_id The unique identifier of the action goal for which the reply is being sent.
     * @param accepted A boolean indicating whether the goal was accepted (true) or rejected (false).
     */
    void send_action_send_goal_reply(
        const std::string& action_name,
        const uint64_t goal_id,
        bool accepted);

    /**
     * @brief Actually sends the result reply for an action.
     *
     * This function is only called when all the conditions are met to send the result reply.
     *
     * @param action_name The name of the action for which the result reply is being sent.
     * @param goal_id The unique identifier of the action goal for which the result reply is being sent.
     * @param reply_json The JSON data to be sent as the result reply.
     * @param request_id The unique identifier of the request to which this reply corresponds.
     */
    bool send_action_get_result_reply(
        const std::string& action_name,
        const participants::UUID& goal_id,
        const std::string& reply_json,
        const uint64_t request_id);

    /**
     * Load the Enabler's internal topics into a configuration object.
     *
     * @param configuration: The configuration to load the internal topics into.
     */
    void load_internal_topics_(
            yaml::EnablerConfiguration& configuration);

    /**
     * Set the internal callbacks used by the enabler.
     *
     * @param callbacks: The set of callbacks to be used by the enabler.
     */
    void set_internal_callbacks_(
            const CallbackSet& callbacks);

    //! Store reference to DomainParticipantFactory to avoid Fast-DDS singletons being destroyed before they should
    std::shared_ptr<eprosima::fastdds::dds::DomainParticipantFactory> part_factory_ =
            eprosima::fastdds::dds::DomainParticipantFactory::get_shared_instance();

    //! Configuration of the DDS Enabler
    yaml::EnablerConfiguration configuration_;

    //! Payload Pool
    std::shared_ptr<ddspipe::core::PayloadPool> payload_pool_;

    //! Discovery Database
    std::shared_ptr<ddspipe::core::DiscoveryDatabase> discovery_database_;

    //! Participants Database
    std::shared_ptr<ddspipe::core::ParticipantsDatabase> participants_database_;

    //! Thread Pool
    std::shared_ptr<utils::SlotThreadPool> thread_pool_;

    //! Handler
    std::shared_ptr<eprosima::ddsenabler::participants::Handler> handler_;

    //! DDS Participant
    std::shared_ptr<eprosima::ddsenabler::participants::DdsParticipant> dds_participant_;

    //! Enabler Participant
    std::shared_ptr<eprosima::ddsenabler::participants::EnablerParticipant> enabler_participant_;

    //! DDS Pipe
    std::unique_ptr<ddspipe::core::DdsPipe> pipe_;

    //! Config File watcher handler
    std::unique_ptr<eprosima::utils::event::FileWatcherHandler> file_watcher_handler_;

    //! Mutex to protect class attributes
    mutable std::mutex mutex_;
};

} /* namespace ddsenabler */
} /* namespace eprosima */
