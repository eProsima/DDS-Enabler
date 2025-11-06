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

#include <cpp_utils/testing/gtest_aux.hpp>
#include <gtest/gtest.h>

#include "dds_enabler_runner.hpp"
#include "ddsenabler_participants/Callbacks.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

static uint32_t data_callback_count = 0;

// Empty callbacks just to create the enabler being tested

// eprosima::ddsenabler::participants::DdsDataNotification data_notification;
void test_data_notification_callback(
        const char* topic_name,
        const char* json,
        int64_t publish_time)
{
}

// eprosima::ddsenabler::participants::DdsTypeNotification type_notification;
void test_type_notification_callback(
        const char* type_name,
        const char* serialized_type,
        const unsigned char* serialized_type_internal,
        uint32_t serialized_type_internal_size,
        const char* data_placeholder)
{
}

// eprosima::ddsenabler::participants::DdsTopicNotification topic_notification;
void test_topic_notification_callback(
        const char* topic_name,
        const eprosima::ddsenabler::participants::TopicInfo& topic_info)
{
}

// eprosima::ddsenabler::participants::DdsTopicQuery topic_query;
bool test_topic_query_callback(
        const char* topic_name,
        eprosima::ddsenabler::participants::TopicInfo& topic_info)
{
    return false;
}

// eprosima::ddsenabler::participants::DdsTypeQuery type_query;
bool test_type_query_callback(
        const char* type_name,
        std::unique_ptr<const unsigned char []>& serialized_type_internal,
        uint32_t& serialized_type_internal_size)
{
    return false;
}

//eprosima::ddsenabler::participants::DdsLogFunc log_callback;
void test_log_callback(
        const char* file_name,
        int line_no,
        const char* func_name,
        int category,
        const char* msg)
{
}

void write_json_file(
        const std::string filePath,
        bool block)
{

    // The JSON template with a placeholder for the domain
    std::string jsonTemplate =
            R"({
  "dds": {
    "ddsmodule": {
      "dds": {
        "domain": 0,
        "allowlist": [
          {
            "name": "*"
          }
        ],%s
        "topics": [
          {
            "name": "*",
            "qos": {
            "durability": true,
            "history-depth": 10
            }
          }
        ]
      }
    }
  }
})";

    // Replace the %d placeholder with the provided domain
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
            jsonTemplate.c_str(),
            block ? "\n    \"blocklist\": [\n        {\n            \"name\": \"*\"\n        }\n    ]," : "");

    // Write the formatted JSON to the file
    std::ofstream outFile(filePath);
    if (outFile.is_open())
    {
        outFile << buffer;
        outFile.close();
    }
    else
    {
        throw std::ios_base::failure("Failed to open the file for writing.");
    }
}

void write_yaml_file(
        const std::string filePath,
        bool block)
{

    // The JSON template with a placeholder for the domain
    std::string Template =
            R"(
dds:
    domain: 0

    %s

topics:
    name: "*"
    qos:
    durability: TRANSIENT_LOCAL
    history-depth: 10
)";

    // Replace the %s placeholder with the provided domain
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), Template.c_str(), block ? "blocklist: \n        - name: \"*\" " : "");

    // Write the formatted YAML to the file
    std::ofstream outFile(filePath);
    if (outFile.is_open())
    {
        outFile << buffer;
        outFile.close();
    }
    else
    {
        throw std::ios_base::failure("Failed to open the file for writing.");
    }
}

using namespace eprosima::ddspipe;

namespace eprosima {
namespace ddsenabler {

// Class that exposes the protected attribute configuration_
class DDSEnablerAccessor : public DDSEnabler
{
public:

    using DDSEnabler::DDSEnabler;
    const void get_allowed_topics(
            std::shared_ptr<ddspipe::core::AllowedTopicList>& ptr) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ptr = std::make_shared<ddspipe::core::AllowedTopicList>(
            this->configuration_.ddspipe_configuration.allowlist,
            this->configuration_.ddspipe_configuration.blocklist);
    }

};

// Test the configuration reload when the json configuration file is modified
TEST(ReloadConfig, json)
{
    auto configfile = "./file_watcher_test.json";
    write_json_file(configfile, false);

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
    ASSERT_TRUE(create_dds_enabler(configfile, callbacks, enabler));

    // Create DDSEnablerAccessor to access protected configuration
    auto enabler_accessor = static_cast<DDSEnablerAccessor*>(enabler.get());

    // Take initial configuration (allowed topics)
    std::shared_ptr<ddspipe::core::AllowedTopicList> allowed_topics_init;
    enabler_accessor->get_allowed_topics(allowed_topics_init);

    // Modify configuration file
    write_json_file(configfile, true);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Take final configuration (allowed topics)
    std::shared_ptr<ddspipe::core::AllowedTopicList> allowed_topics_final;
    enabler_accessor->get_allowed_topics(allowed_topics_final);

    ASSERT_TRUE(!(*allowed_topics_init == *allowed_topics_final));

    // Delete test file
    std::filesystem::remove(configfile);
}

// Test the configuration reload when the yaml configuration file is modified
TEST(ReloadConfig, yaml)
{
    auto configfile = "./file_watcher_test.yaml";
    write_yaml_file(configfile, 0);

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
    ASSERT_TRUE(create_dds_enabler(configfile, callbacks, enabler));

    // Create DDSEnablerAccessor to access protected configuration
    auto enabler_accessor = static_cast<DDSEnablerAccessor*>(enabler.get());

    // Take initial configuration (allowed topics)
    std::shared_ptr<ddspipe::core::AllowedTopicList> allowed_topics_init;
    enabler_accessor->get_allowed_topics(allowed_topics_init);

    // Modify configuration file
    write_yaml_file(configfile, 1);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Take final configuration (allowed topics)
    std::shared_ptr<ddspipe::core::AllowedTopicList> allowed_topics_final;
    enabler_accessor->get_allowed_topics(allowed_topics_final);

    ASSERT_TRUE(!(*allowed_topics_init == *allowed_topics_final));

    // Delete test file
    std::filesystem::remove(configfile);
}

} /* namespace ddsenabler */
} /* namespace eprosima */


int main(
        int argc,
        char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
