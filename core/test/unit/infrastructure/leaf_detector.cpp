#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/leaf_detector.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column&), (const));
};

struct LeafDetectorTest : public ::testing::Test {
    LeafDetectorTest();
    value truth;
    value falsity;
    bin data;
    NiceMock<MockFindUniformValue> find_uniform_value;
    leaf_detector<NiceMock<MockFindUniformValue>> detector{find_uniform_value};
};

LeafDetectorTest::LeafDetectorTest()
    : truth{.ctor = 1, .fields = {}}
    , falsity{.ctor = 0, .fields = {}}
    , data{.scope = {typed_column{.type = 0, .cells = {&truth, &falsity}}}, .labels = {&truth, &truth}}
    , find_uniform_value() {}

}

TEST_F(LeafDetectorTest, LabelsThatAgreeGiveTheirLabel) {
    EXPECT_CALL(find_uniform_value, find(data.labels)).WillOnce(Return(&truth));
    EXPECT_EQ(detector.detect(data), std::optional<const value*>(&truth));
}

TEST_F(LeafDetectorTest, LabelsThatDisagreeGiveNoLabel) {
    EXPECT_CALL(find_uniform_value, find(data.labels)).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(detector.detect(data).has_value());
}
