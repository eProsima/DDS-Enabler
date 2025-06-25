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

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/xtypes/type_representation/TypeObject.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilder.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/MemberDescriptor.hpp>
#include <fastdds/dds/xtypes/dynamic_types/TypeDescriptor.hpp>

#include <ddspipe_core/efficiency/payload/FastPayloadPool.hpp>

#include <Handler.hpp>
#include <HandlerConfiguration.hpp>
#include <Message.hpp>
#include <Writer.hpp>

#include "types/DDSEnablerTestTypesPubSubTypes.hpp"

using namespace eprosima;
using namespace eprosima::fastdds::dds;
using namespace eprosima::ddsenabler;

class WriterTest : public participants::Writer
{
public:

    WriterTest() = default;
    ~WriterTest() = default;

    // Expose protected methods
    using Writer::write_data;
    using Writer::write_schema;
};

class HandlerTest : public participants::Handler
{
public:

    HandlerTest(
            const participants::HandlerConfiguration& config,
            const std::shared_ptr<ddspipe::core::PayloadPool>& payload_pool)
        : participants::Handler(config, payload_pool)
    {
        current_test_instance_ = this;

        writer_ = std::make_unique<WriterTest>();

        // Set the callbacks
        set_data_notification_callback(test_data_notification_callback);
        set_type_notification_callback(test_type_notification_callback);
        set_topic_notification_callback(test_topic_notification_callback);
        set_type_query_callback(test_type_query_callback);
    }

    // Expose protected members
    using participants::Handler::schemas_;
    using participants::Handler::writer_;
    using participants::Handler::unique_sequence_number_;

    // eprosima::ddsenabler::participants::DdsTypeQuery type_query;
    static bool test_type_query_callback(
            const char* type_name,
            std::unique_ptr<const unsigned char []>& serialized_type_internal,
            uint32_t& serialized_type_internal_size)
    {
        if (current_test_instance_ == nullptr)
        {
            return false;
        }

        current_test_instance_->type_query_called++;
        return true;
    }

    // eprosima::ddsenabler::participants::DdsDataNotification data_notification;
    static void test_data_notification_callback(
            const char* topic_name,
            const char* json,
            int64_t publish_time)
    {
        if (current_test_instance_ == nullptr)
        {
            return;
        }

        current_test_instance_->data_called_++;
    }

    // eprosima::ddsenabler::participants::DdsTypeNotification type_notification;
    static void test_type_notification_callback(
            const char* type_name,
            const char* serialized_type,
            const unsigned char* serialized_type_internal,
            uint32_t serialized_type_internal_size,
            const char* data_placeholder)
    {
        if (current_test_instance_ == nullptr)
        {
            return;
        }

        current_test_instance_->type_called_++;
    }

    // eprosima::ddsenabler::participants::DdsTopicNotification topic_notification;
    static void test_topic_notification_callback(
            const char* topic_name,
            const char* type_name,
            const char* serialized_qos)
    {
        if (current_test_instance_ == nullptr)
        {
            return;
        }

        current_test_instance_->topic_called_++;
    }

    uint32_t type_query_called = 0;
    uint32_t data_called_ = 0;
    uint32_t type_called_ = 0;
    uint32_t topic_called_ = 0;


    // Pointer to the current test instance (for use in the static callback)
    static HandlerTest* current_test_instance_;
};

HandlerTest* HandlerTest::current_test_instance_ = nullptr;

void get_dynamic_type(
        int num_type,
        DynamicType::_ref_type& dynamic_type,
        xtypes::TypeIdentifier& type_identifier,
        ddspipe::core::types::DdsTopic& pipe_topic)
{
    std::shared_ptr<TopicDataType> type_support;
    switch (num_type)
    {
        case 3:
        {
            type_support.reset(new DDSEnablerTestType3PubSubType());
            break;
        }
        case 2:
        {
            type_support.reset(new DDSEnablerTestType2PubSubType());
            break;
        }
        case 1:
        default:
        {
            type_support.reset(new DDSEnablerTestType1PubSubType());
            break;
        }
    }
    type_support->register_type_object_representation();
    auto type_id_pair = type_support->type_identifiers();

    type_identifier =
            (fastdds::dds::xtypes::EK_COMPLETE ==
            type_id_pair.type_identifier1()._d()) ? type_id_pair.type_identifier1() : type_id_pair.type_identifier2();

    xtypes::TypeObject type_obj;
    ASSERT_EQ(RETCODE_OK,
            DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(type_identifier,
            type_obj));
    dynamic_type = DynamicTypeBuilderFactory::get_instance()->create_type_w_type_object(type_obj)->build();

    std::ostringstream topic_name;
    topic_name << type_support->get_name() << "_topic_name_" << std::to_string(num_type);
    pipe_topic.m_topic_name = topic_name.str();
    pipe_topic.type_name = type_support->get_name();
    pipe_topic.type_identifiers = type_id_pair;
}

void get_dynamic_type(
        int num_type,
        DynamicType::_ref_type& dynamic_type,
        xtypes::TypeIdentifier& type_identifier)
{
    ddspipe::core::types::DdsTopic _;
    get_dynamic_type(num_type, dynamic_type, type_identifier, _);
}

void get_data_payload(
        int num_type,
        eprosima::ddspipe::core::types::Payload& payload)
{
    std::shared_ptr<TopicDataType> type_support;
    switch (num_type)
    {
        case 3:
        {
            type_support.reset(new DDSEnablerTestType3PubSubType());
            break;
        }
        case 2:
        {
            type_support.reset(new DDSEnablerTestType2PubSubType());
            break;
        }
        case 1:
        default:
        {
            type_support.reset(new DDSEnablerTestType1PubSubType());
            break;
        }
    }
    void* data = type_support->create_data();
    ASSERT_TRUE(type_support->serialize(data, payload, DataRepresentationId::XCDR2_DATA_REPRESENTATION));
    type_support->delete_data(data);
}

TEST(DdsEnablerParticipantsTest, ddsenabler_participants_handler_creation)
{
    // Create Payload Pool
    auto payload_pool_ = std::make_shared<ddspipe::core::FastPayloadPool>();
    ASSERT_NE(payload_pool_, nullptr);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create Handler
    auto handler_ = std::make_shared<participants::Handler>(handler_config, payload_pool_);

    ASSERT_NE(handler_, nullptr);
}

TEST(DdsEnablerParticipantsTest, ddsenabler_participants_add_new_schemas)
{
    // Create Payload Pool
    auto payload_pool_ = std::make_shared<ddspipe::core::FastPayloadPool>();
    ASSERT_NE(payload_pool_, nullptr);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create Handler
    auto handler_ = std::make_shared<HandlerTest>(handler_config, payload_pool_);
    ASSERT_NE(handler_, nullptr);

    // Add a new schema
    xtypes::TypeIdentifier type_id;
    DynamicType::_ref_type dynamic_type;
    get_dynamic_type(1, dynamic_type, type_id);

    ASSERT_TRUE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 0);
    handler_->add_schema(dynamic_type, type_id);
    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 1);
    ASSERT_EQ(handler_->type_called_, 1);

    // Add another schema for a different type
    xtypes::TypeIdentifier type_id2;
    DynamicType::_ref_type dynamic_type2;
    get_dynamic_type(2, dynamic_type2, type_id2);

    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 1);
    handler_->add_schema(dynamic_type2, type_id2);
    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 2);
    ASSERT_EQ(handler_->type_called_, 2);
}

TEST(DdsEnablerParticipantsTest, ddsenabler_participants_add_same_type_schema)
{
    // Create Payload Pool
    auto payload_pool_ = std::make_shared<ddspipe::core::FastPayloadPool>();
    ASSERT_NE(payload_pool_, nullptr);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create Handler
    auto handler_ = std::make_shared<HandlerTest>(handler_config, payload_pool_);
    ASSERT_NE(handler_, nullptr);

    // Add a new schema
    xtypes::TypeIdentifier type_id;
    DynamicType::_ref_type dynamic_type;
    get_dynamic_type(1, dynamic_type, type_id);

    ASSERT_TRUE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 0);
    handler_->add_schema(dynamic_type, type_id);
    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 1);
    ASSERT_EQ(handler_->type_called_, 1);

    // Add the same schema again
    xtypes::TypeIdentifier type_id2;
    DynamicType::_ref_type dynamic_type2;
    get_dynamic_type(1, dynamic_type2, type_id2);

    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 1);
    handler_->add_schema(dynamic_type2, type_id2);
    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 1);
    ASSERT_EQ(handler_->type_called_, 1);
}

TEST(DdsEnablerParticipantsTest, ddsenabler_participants_add_data_with_schema)
{
    // Create Payload Pool
    auto payload_pool_ = std::make_shared<ddspipe::core::FastPayloadPool>();
    ASSERT_NE(payload_pool_, nullptr);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create Handler
    auto handler_ = std::make_shared<HandlerTest>(handler_config, payload_pool_);
    ASSERT_NE(handler_, nullptr);

    xtypes::TypeIdentifier type_identifier;
    DynamicType::_ref_type dynamic_type;
    ddspipe::core::types::DdsTopic pipe_topic;
    get_dynamic_type(1, dynamic_type, type_identifier, pipe_topic);

    ASSERT_TRUE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 0);
    handler_->add_schema(dynamic_type, type_identifier);
    ASSERT_FALSE(handler_->schemas_.empty());
    ASSERT_EQ(handler_->schemas_.size(), 1);
    ASSERT_EQ(handler_->type_called_, 1);

    auto data = std::make_unique<eprosima::ddspipe::core::types::RtpsPayloadData>();

    payload_pool_->get_payload(1000, data->payload);
    data->payload_owner = payload_pool_.get();
    get_data_payload(1, data->payload);

    ASSERT_NO_THROW(handler_->add_data(pipe_topic, *data));
    // If the data has been added, the sequence number should be 1
    ASSERT_TRUE(handler_->unique_sequence_number_ == 1);
    ASSERT_EQ(handler_->data_called_, 1);
}

TEST(DdsEnablerParticipantsTest, ddsenabler_participants_add_data_without_schema)
{
    // Create Payload Pool
    auto payload_pool_ = std::make_shared<ddspipe::core::FastPayloadPool>();
    ASSERT_NE(payload_pool_, nullptr);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create Handler
    auto handler_ = std::make_shared<HandlerTest>(handler_config, payload_pool_);
    ASSERT_NE(handler_, nullptr);

    xtypes::TypeIdentifier type_id;
    DynamicType::_ref_type dynamic_type;
    ddspipe::core::types::DdsTopic pipe_topic;
    get_dynamic_type(1, dynamic_type, type_id, pipe_topic);

    auto data = std::make_unique<eprosima::ddspipe::core::types::RtpsPayloadData>();

    payload_pool_->get_payload(1000, data->payload);
    data->payload_owner = payload_pool_.get();
    get_data_payload(1, data->payload);

    ASSERT_NO_THROW(handler_->add_data(pipe_topic, *data));
    // As there is no schema associated, the sequence number should still be 0
    ASSERT_TRUE(handler_->unique_sequence_number_ == 0);
    ASSERT_EQ(handler_->data_called_, 0);
}

TEST(DdsEnablerParticipantsTest, ddsenabler_participants_write_schema_first_time)
{
    // Create Payload Pool
    auto payload_pool_ = std::make_shared<ddspipe::core::FastPayloadPool>();
    ASSERT_NE(payload_pool_, nullptr);

    // Create Handler configuration
    participants::HandlerConfiguration handler_config;

    // Create Handler
    auto handler_ = std::make_shared<HandlerTest>(handler_config, payload_pool_);
    ASSERT_NE(handler_, nullptr);

    xtypes::TypeIdentifier type_id;
    DynamicType::_ref_type dynamic_type;
    ddspipe::core::types::DdsTopic pipe_topic;
    get_dynamic_type(1, dynamic_type, type_id, pipe_topic);

    auto data = std::make_unique<eprosima::ddspipe::core::types::RtpsPayloadData>();

    payload_pool_->get_payload(1000, data->payload);
    data->payload_owner = payload_pool_.get();
    get_data_payload(1, data->payload);

    participants::Message msg;
    msg.sequence_number = 1;
    msg.publish_time = data->source_timestamp;
    msg.topic = pipe_topic;
    msg.instanceHandle = data->instanceHandle;
    msg.source_guid = data->source_guid;
    payload_pool_->get_payload(data->payload, msg.payload);
    msg.payload_owner = payload_pool_.get();

    handler_->writer_->write_data(msg, dynamic_type);
    // The data will be successfully written as the dynamic type exists and we are bypassing the handler schema check
    ASSERT_EQ(handler_->data_called_, 1);

    xtypes::TypeIdentifier type_id2;
    DynamicType::_ref_type dynamic_type2;
    ddspipe::core::types::DdsTopic pipe_topic2;
    get_dynamic_type(2, dynamic_type2, type_id2, pipe_topic2);

    auto data2 = std::make_unique<eprosima::ddspipe::core::types::RtpsPayloadData>();

    payload_pool_->get_payload(1000, data2->payload);
    data2->payload_owner = payload_pool_.get();
    get_data_payload(2, data2->payload);

    participants::Message msg2;
    msg2.sequence_number = 1;
    msg2.publish_time = data2->source_timestamp;
    msg2.topic = pipe_topic2;
    msg2.instanceHandle = data2->instanceHandle;
    msg2.source_guid = data2->source_guid;
    payload_pool_->get_payload(data2->payload, msg2.payload);
    msg2.payload_owner = payload_pool_.get();

    handler_->writer_->write_data(msg2, dynamic_type2);
    ASSERT_EQ(handler_->data_called_, 2);
}

int main(
        int argc,
        char** argv)
{
    eprosima::fastdds::dds::Log::SetVerbosity(Log::Kind::Warning);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
