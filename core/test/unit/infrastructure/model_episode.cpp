#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_episode.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockBuildModel {
    MOCK_METHOD(const model_node*, build, (const bin&));
};

struct MockSizeModel {
    MOCK_METHOD(size_t, size, (const model_node*), (const));
};

struct MockTryRecordModel {
    MOCK_METHOD(void, try_record, (const model_node*, size_t));
};

struct ModelEpisodeTest : public ::testing::Test {
    ModelEpisodeTest();
    value label;
    feature label_output;
    bin training;
    model_node model;
    NiceMock<MockBuildModel> build_model;
    NiceMock<MockSizeModel> size_model;
    NiceMock<MockTryRecordModel> try_record_model;
    model_episode<NiceMock<MockBuildModel>, NiceMock<MockSizeModel>, NiceMock<MockTryRecordModel>> episode{
        build_model, size_model, try_record_model};
};

ModelEpisodeTest::ModelEpisodeTest()
    : label{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , training{.inputs = {}, .labels = {&label}}
    , label_output{feature::constant{.literal = &label}}
    , model{model_node::leaf{.output = &label_output}}
    , build_model()
    , size_model()
    , try_record_model() {}

}

TEST_F(ModelEpisodeTest, TheBuiltModelIsSizedAndOfferedAsTheBest) {
    InSequence order;
    EXPECT_CALL(build_model, build(training)).WillOnce(Return(&model));
    EXPECT_CALL(size_model, size(&model)).WillOnce(Return(7));
    EXPECT_CALL(try_record_model, try_record(&model, 7));
    EXPECT_EQ(episode.run(training), 7u);
}

TEST_F(ModelEpisodeTest, EveryRunBuildsAFreshModel) {
    EXPECT_CALL(build_model, build(training)).Times(2).WillRepeatedly(Return(&model));
    episode.run(training);
    episode.run(training);
}

TEST_F(ModelEpisodeTest, EachRunsCostIsOfferedWithItsOwnModel) {
    const model_node other{model_node::empty{}};
    EXPECT_CALL(build_model, build(training)).WillOnce(Return(&model)).WillOnce(Return(&other));
    ON_CALL(size_model, size(&model)).WillByDefault(Return(4));
    ON_CALL(size_model, size(&other)).WillByDefault(Return(0));
    InSequence order;
    EXPECT_CALL(try_record_model, try_record(&model, 4));
    EXPECT_CALL(try_record_model, try_record(&other, 0));
    EXPECT_EQ(episode.run(training), 4u);
    EXPECT_EQ(episode.run(training), 0u);
}
