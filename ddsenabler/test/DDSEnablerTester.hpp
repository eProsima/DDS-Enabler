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

#include <mutex>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilder.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/xtypes/type_representation/TypeObject.hpp>
#include <ddsenabler_participants/Utils.hpp>

#include "ddsenabler/dds_enabler_runner.hpp"

using namespace eprosima::ddspipe;
using namespace eprosima::ddsenabler;
using namespace eprosima::ddsenabler::participants;
using namespace eprosima::fastdds::dds;

#define TEST_SERVICE_NAME "add_two_ints"
#define TEST_ACTION_NAME "fibonacci/_action/"

#define TEST_FILE_DIRECTORY "/home/eugenio/Documents/enabler_suite/fast_suite/src/DDS-Enabler/ddsenabler/test/test_files/"
#define TEST_SERVICE_FILE "test_service.json"
#define TEST_ACTION_FILE "test_action.json"

namespace ddsenablertester {

struct KnownType
{
    DynamicType::_ref_type dyn_type_;
    TypeSupport type_sup_;
    DataWriter* writer_ = nullptr;
};

const unsigned int DOMAIN_ = 33;
static int num_samples_ =  1;
static int wait_after_writer_creation_ms_ =  300;
static int write_delay_ms_ =  20;
static int wait_for_ack_ns_ =  1000000000;
static int wait_after_publication_ms_ =  200;

class DDSEnablerTester : public ::testing::Test
{
public:

    // SetUp method to initialize variables before each test
    void SetUp() override
    {
        std::cout << "Setting up test..." << std::endl;
        received_types_ = 0;
        received_topics_ = 0;
        received_data_ = 0;
        current_test_instance_ = this;  // Set the current instance for callbacks
    }

    // Reset after each test
    void TearDown() override
    {
        std::cout << "Tearing down test..." << std::endl;
        std::cout << "Received types before reset: " << received_types_ << std::endl;
        std::cout << "Received topics before reset: " << received_topics_ << std::endl;
        std::cout << "Received data before reset: " << received_data_ << std::endl;
        received_types_ = 0;
        received_topics_ = 0;
        received_data_ = 0;
        current_test_instance_ = nullptr;
    }

    // Create the DDSEnabler and bind the static callbacks
    std::shared_ptr<DDSEnabler> create_ddsenabler()
    {
        YAML::Node yml;

        eprosima::ddsenabler::yaml::EnablerConfiguration configuration(yml);
        configuration.simple_configuration->domain = DOMAIN_;

        CallbackSet callbacks{
            test_log_callback,
            {
                test_type_notification_callback,
                test_topic_notification_callback,
                test_data_notification_callback,
                test_type_query_callback,
                test_topic_query_callback
            }
        };

        // Create DDS Enabler
        std::shared_ptr<DDSEnabler> enabler;
        bool result = create_dds_enabler(configuration, callbacks, enabler);

        return enabler;
    }

    // Create the DDSEnabler and bind the static callbacks
    std::shared_ptr<DDSEnabler> create_ddsenabler_w_history()
    {
        const char* yml_str =
                R"(
                dds:
                  topics:
                    - name: "*"
                      qos:
                        durability: true
                        history-depth: 10
            )";
        eprosima::Yaml yml = YAML::Load(yml_str);
        eprosima::ddsenabler::yaml::EnablerConfiguration configuration(yml);
        configuration.simple_configuration->domain = DOMAIN_;

        eprosima::utils::Formatter error_msg;
        if (!configuration.is_valid(error_msg))
        {
            return nullptr;
        }

        CallbackSet callbacks{
            test_log_callback,
            {
                test_type_notification_callback,
                test_topic_notification_callback,
                test_data_notification_callback,
                test_type_query_callback,
                test_topic_query_callback
            }
        };

        return std::make_shared<DDSEnabler>(configuration, callbacks);
    }

    bool create_publisher(
            KnownType& a_type)
    {
        DomainParticipant* participant = DomainParticipantFactory::get_instance()
                        ->create_participant(DOMAIN_, PARTICIPANT_QOS_DEFAULT);
        if (participant == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_participant" << std::endl;
            return false;
        }

        if (RETCODE_OK != a_type.type_sup_.register_type(participant))
        {
            std::cout << "ERROR DDSEnablerTester: fail to register type: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (publisher == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_publisher: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        std::ostringstream topic_name;
        topic_name << a_type.type_sup_.get_type_name() << "TopicName";
        Topic* topic = participant->create_topic(topic_name.str(), a_type.type_sup_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_topic: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        DataWriterQos wqos = publisher->get_default_datawriter_qos();
        a_type.writer_ = publisher->create_datawriter(topic, wqos);
        if (a_type.writer_ == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_datawriter: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_after_writer_creation_ms_));
        return true;
    }

    bool create_publisher_w_history(
            KnownType& a_type,
            int history_depth)
    {
        DomainParticipant* participant = DomainParticipantFactory::get_instance()
                        ->create_participant(DOMAIN_, PARTICIPANT_QOS_DEFAULT);
        if (participant == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_participant" << std::endl;
            return false;
        }

        if (RETCODE_OK != a_type.type_sup_.register_type(participant))
        {
            std::cout << "ERROR DDSEnablerTester: fail to register type: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (publisher == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_publisher: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        std::ostringstream topic_name;
        topic_name << a_type.type_sup_.get_type_name() << "TopicName";
        Topic* topic = participant->create_topic(topic_name.str(), a_type.type_sup_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_topic: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        DataWriterQos wqos = publisher->get_default_datawriter_qos();
        wqos.history().depth = history_depth;
        a_type.writer_ = publisher->create_datawriter(topic, wqos);
        if (a_type.writer_ == nullptr)
        {
            std::cout << "ERROR DDSEnablerTester: create_datawriter: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_after_writer_creation_ms_));
        return true;
    }

    bool send_samples(
            KnownType& a_type)
    {
        std::cout << "Sending samples for type: " << a_type.type_sup_.get_type_name() << std::endl;
        for (long i = 0; i < ddsenablertester::num_samples_; i++)
        {
            void* sample = a_type.type_sup_.create_data();
            if (RETCODE_OK != a_type.writer_->write(sample))
            {
                std::cout << "ERROR DDSEnablerTester: fail writing sample: " <<
                    a_type.type_sup_.get_type_name() << std::endl;
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(write_delay_ms_));
            a_type.type_sup_.delete_data(sample);
        }

        if (RETCODE_OK != a_type.writer_->wait_for_acknowledgments(Duration_t(0, wait_for_ack_ns_)))
        {
            std::cout << "ERROR DDSEnablerTester: fail waiting for acknowledgments: " <<
                a_type.type_sup_.get_type_name() << std::endl;
            return false;
        }

        return true;
    }

    // eprosima::ddsenabler::participants::DdsDataNotification data_notification;
    static void test_data_notification_callback(
            const char* topic_name,
            const char* json,
            int64_t publish_time)
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->data_received_mutex_);

            current_test_instance_->received_data_++;
            std::cout << "Data callback received: " << topic_name << ", Total data: " <<
                current_test_instance_->received_data_ << std::endl;
        }
    }

    // eprosima::ddsenabler::participants::DdsTypeNotification type_notification;
    static void test_type_notification_callback(
            const char* type_name,
            const char* serialized_type,
            const unsigned char* serialized_type_internal,
            uint32_t serialized_type_internal_size,
            const char* data_placeholder)
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->type_received_mutex_);

            current_test_instance_->received_types_++;
            std::cout << "Type callback received: " << type_name << ", Total types: " <<
                current_test_instance_->received_types_ << std::endl;
        }
    }

    // eprosima::ddsenabler::participants::DdsTopicNotification topic_notification
    static void test_topic_notification_callback(
            const char* topic_name,
            const char* type_name,
            const char* serialized_qos)
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->topic_received_mutex_);

            current_test_instance_->received_topics_++;
            std::cout << "Topic callback received: " << topic_name << ", Total topics: " <<
                current_test_instance_->received_topics_ << std::endl;
        }
    }

    // eprosima::ddsenabler::participants::DdsTopicQuery topic_query;
    static bool test_topic_query_callback(
            const char* topic_name,
            std::string& type_name,
            std::string& serialized_qos)
    {
        return false;
    }

    // eprosima::ddsenabler::participants::DdsTypeQuery type_query;
    static bool test_type_query_callback(
            const char* type_name,
            std::unique_ptr<const unsigned char []>& serialized_type_internal,
            uint32_t& serialized_type_internal_size)
    {
        return false;
    }

    //eprosima::ddsenabler::participants::DdsLogFunc log_callback;
    static void test_log_callback(
            const char* fileName,
            int lineNo,
            const char* funcName,
            int category,
            const char* msg)
    {
    }

    // eprosima::ddsenabler::participants::ServiceTypeQuery type_req_callback;
    static bool test_service_type_request_callback(
            const char* service_name,
            char*& request_type_name,
            char*& request_serialized_qos,
            char*& reply_type_name,
            char*& reply_serialized_qos)
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->rpc_mutex_);

            std::string service_file = TEST_FILE_DIRECTORY;
            std::string request_type_name_str;
            std::string reply_type_name_str;
            std::string request_serialized_qos_str;
            std::string reply_serialized_qos_str;
            if (eprosima::ddsenabler::participants::utils::load_service_from_file(
                    service_file,
                    service_name,
                    request_type_name_str,
                    reply_type_name_str,
                    request_serialized_qos_str,
                    reply_serialized_qos_str))
            {
                request_type_name = new char[request_type_name_str.size() + 1];
                std::strcpy(request_type_name, request_type_name_str.c_str());

                reply_type_name = new char[reply_type_name_str.size() + 1];
                std::strcpy(reply_type_name, reply_type_name_str.c_str());

                request_serialized_qos = new char[request_serialized_qos_str.size() + 1];
                std::strcpy(request_serialized_qos, request_serialized_qos_str.c_str());

                reply_serialized_qos = new char[reply_serialized_qos_str.size() + 1];
                std::strcpy(reply_serialized_qos, reply_serialized_qos_str.c_str());
                return true;
            }
            std::cout << "ERROR DDSEnablerTester: fail to load service from file: " << service_file << std::endl;
        }
        return false;
    }

    int get_received_types()
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->type_received_mutex_);

            return current_test_instance_->received_types_;
        }
        else
        {
            return 0;
        }
    }

    int get_received_topics()
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->topic_received_mutex_);

            return current_test_instance_->received_topics_;
        }
        else
        {
            return 0;
        }
    }

    int get_received_data()
    {
        if (current_test_instance_)
        {
            std::lock_guard<std::mutex> lock(current_test_instance_->data_received_mutex_);

            return current_test_instance_->received_data_;
        }
        else
        {
            return 0;
        }
    }

    // Pointer to the current test instance (for use in the static callback)
    static DDSEnablerTester* current_test_instance_;

    // Test-specific received counters
    int received_types_ = 0;
    int received_topics_ = 0;
    int received_data_ = 0;

    // Mutex for synchronizing access to received_types_, received_topics_ and received_data_
    std::mutex type_received_mutex_;
    std::mutex topic_received_mutex_;
    std::mutex data_received_mutex_;
    std::mutex rpc_mutex_;
};


} // namespace ddsenablertester
