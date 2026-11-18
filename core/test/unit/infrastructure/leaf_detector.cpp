#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/leaf_detector.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeVarFeature {
    MOCK_METHOD(const feature*, make_var, (var_id));
};

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column&), (const));
};

struct MockMakeConstantFeature {
    MOCK_METHOD(const feature*, make_constant, (const value*));
};

struct LeafDetectorTest : public ::testing::Test {
    LeafDetectorTest();
    value falsity;
    value truth;
    feature first_variable;
    feature second_variable;
    feature true_constant;
    NiceMock<MockMakeVarFeature> make_var_feature;
    NiceMock<MockFindUniformValue> find_uniform_value;
    NiceMock<MockMakeConstantFeature> make_constant_feature;
    leaf_detector<NiceMock<MockMakeVarFeature>, NiceMock<MockFindUniformValue>, NiceMock<MockMakeConstantFeature>>
        detector{make_var_feature, find_uniform_value, make_constant_feature};

    void SetUp() override {
        ON_CALL(make_var_feature, make_var(0)).WillByDefault(Return(&first_variable));
        ON_CALL(make_var_feature, make_var(1)).WillByDefault(Return(&second_variable));
        ON_CALL(make_constant_feature, make_constant(&truth)).WillByDefault(Return(&true_constant));
        ON_CALL(find_uniform_value, find(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(find_uniform_value, find(column{&truth, &truth})).WillByDefault(Return(&truth));
    }
};

LeafDetectorTest::LeafDetectorTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , first_variable{feature::var{.index = 0}}
    , second_variable{feature::var{.index = 1}}
    , true_constant{feature::constant{.literal = &truth}}
    , make_var_feature()
    , find_uniform_value()
    , make_constant_feature() {}

}

TEST_F(LeafDetectorTest, LabelsThatAgreeGiveTheirLabelAsAConstant) {
    const bin data{.scope = {typed_column{.type = 0, .cells = {&truth, &falsity}}}, .labels = {&truth, &truth}};
    EXPECT_EQ(detector.detect(data), std::optional<const feature*>(&true_constant));
}

TEST_F(LeafDetectorTest, LabelsThatDisagreeAndMatchNoVariableGiveNothing) {
    const bin data{.scope = {typed_column{.type = 0, .cells = {&truth, &truth}}}, .labels = {&truth, &falsity}};
    EXPECT_FALSE(detector.detect(data).has_value());
}

TEST_F(LeafDetectorTest, AVariableWhoseColumnIsTheLabelsIsTheOutput) {
    const bin data{
        .scope = {typed_column{.type = 0, .cells = {&truth, &truth}},
                  typed_column{.type = 0, .cells = {&falsity, &truth}}},
        .labels = {&falsity, &truth}};
    EXPECT_EQ(detector.detect(data), std::optional<const feature*>(&second_variable));
}

TEST_F(LeafDetectorTest, TheFirstMatchingVariableWins) {
    const bin data{
        .scope = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                  typed_column{.type = 0, .cells = {&falsity, &truth}}},
        .labels = {&falsity, &truth}};
    EXPECT_EQ(detector.detect(data), std::optional<const feature*>(&first_variable));
}

TEST_F(LeafDetectorTest, AMatchingVariableIsPreferredToAgreeingLabels) {
    const bin data{.scope = {typed_column{.type = 0, .cells = {&truth, &truth}}}, .labels = {&truth, &truth}};
    EXPECT_CALL(make_constant_feature, make_constant(_)).Times(0);
    EXPECT_EQ(detector.detect(data), std::optional<const feature*>(&first_variable));
}

TEST_F(LeafDetectorTest, ABinWithoutVariablesCanStillBeAConstantLeaf) {
    const bin data{.scope = {}, .labels = {&truth, &truth}};
    EXPECT_EQ(detector.detect(data), std::optional<const feature*>(&true_constant));
}
