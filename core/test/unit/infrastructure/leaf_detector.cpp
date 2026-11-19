#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/leaf_detector.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column&), (const));
};

struct LeafDetectorTest : public ::testing::Test {
    LeafDetectorTest();
    value falsity;
    value truth;
    feature first_variable;
    feature second_variable;
    feature true_constant;
    NiceMock<MockFindUniformValue> find_uniform_value;
    leaf_detector<NiceMock<MockFindUniformValue>> detector{find_uniform_value};

    void SetUp() override {
        ON_CALL(find_uniform_value, find(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(find_uniform_value, find(column{&truth, &truth})).WillByDefault(Return(&truth));
    }
};

LeafDetectorTest::LeafDetectorTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , first_variable{feature::var{.index = 0}}
    , second_variable{feature::var{.index = 1}}
    , true_constant{feature::constant{.literal = &truth}}
    , find_uniform_value() {}

}

TEST_F(LeafDetectorTest, LabelsThatAgreeGiveTheirLabelAsAConstant) {
    const bin data{.inputs = {typed_column{.type = 0, .cells = {&truth, &falsity}}},
                   .labels = typed_column{.type = 0, .cells = {&truth, &truth}}};
    EXPECT_EQ(detector.detect(data), std::optional<feature>(true_constant));
}

TEST_F(LeafDetectorTest, LabelsThatDisagreeAndMatchNoVariableGiveNothing) {
    const bin data{.inputs = {typed_column{.type = 0, .cells = {&truth, &truth}}},
                   .labels = typed_column{.type = 0, .cells = {&truth, &falsity}}};
    EXPECT_FALSE(detector.detect(data).has_value());
}

TEST_F(LeafDetectorTest, AVariableWhoseColumnIsTheLabelsIsTheOutput) {
    const bin data{
        .inputs = {typed_column{.type = 0, .cells = {&truth, &truth}},
                  typed_column{.type = 0, .cells = {&falsity, &truth}}},
        .labels = typed_column{.type = 0, .cells = {&falsity, &truth}}};
    EXPECT_EQ(detector.detect(data), std::optional<feature>(second_variable));
}

TEST_F(LeafDetectorTest, TheFirstMatchingVariableWins) {
    const bin data{
        .inputs = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                  typed_column{.type = 0, .cells = {&falsity, &truth}}},
        .labels = typed_column{.type = 0, .cells = {&falsity, &truth}}};
    EXPECT_EQ(detector.detect(data), std::optional<feature>(first_variable));
}

TEST_F(LeafDetectorTest, AMatchingVariableIsPreferredToAgreeingLabels) {
    const bin data{.inputs = {typed_column{.type = 0, .cells = {&truth, &truth}}},
                   .labels = typed_column{.type = 0, .cells = {&truth, &truth}}};
    EXPECT_EQ(detector.detect(data), std::optional<feature>(first_variable));
}

TEST_F(LeafDetectorTest, ABinWithoutVariablesCanStillBeAConstantLeaf) {
    const bin data{.inputs = {}, .labels = typed_column{.type = 0, .cells = {&truth, &truth}}};
    EXPECT_EQ(detector.detect(data), std::optional<feature>(true_constant));
}

TEST_F(LeafDetectorTest, AVariableOfAnotherTypeIsNotTheOutputEvenWithTheSameCells) {
    const bin data{
        .inputs = {typed_column{.type = 4, .cells = {&falsity, &truth}}},
        .labels = typed_column{.type = 0, .cells = {&falsity, &truth}}};
    EXPECT_FALSE(detector.detect(data).has_value());
}
