#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/operand_table.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetConstants {
    MOCK_METHOD(const std::vector<constant_operand>&, constants, (), (const));
};

struct MockGetConstantsOfType {
    MOCK_METHOD(const std::vector<size_t>&, constants_of_type, (type_id), (const));
};

struct MockGetFeatures {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockGetFeaturesOfType {
    MOCK_METHOD(const std::vector<size_t>&, features_of_type, (type_id), (const));
};

// Constants: true (type 0), zero (type 1), false (type 0). Features: x0 (type 0), x1 (type 1).
// Operands 0..2 are the constants and 3..4 the features.
struct OperandTableTest : public ::testing::Test {
    OperandTableTest();
    value falsity;
    value truth;
    value zero;
    feature true_constant;
    feature zero_constant;
    feature false_constant;
    feature first_variable;
    feature second_variable;
    std::vector<constant_operand> constants;
    std::vector<candidate> features;
    std::vector<size_t> boolean_constants;
    std::vector<size_t> nat_constants;
    std::vector<size_t> boolean_features;
    std::vector<size_t> nat_features;
    std::vector<size_t> none;
    NiceMock<MockGetConstants> get_constants;
    NiceMock<MockGetConstantsOfType> get_constants_of_type;
    NiceMock<MockGetFeatures> get_features;
    NiceMock<MockGetFeaturesOfType> get_features_of_type;
    operand_table<
        NiceMock<MockGetConstants>, NiceMock<MockGetConstantsOfType>, NiceMock<MockGetFeatures>,
        NiceMock<MockGetFeaturesOfType>>
        table{get_constants, get_constants_of_type, get_features, get_features_of_type};

    void SetUp() override {
        ON_CALL(get_constants, constants()).WillByDefault(ReturnRef(constants));
        ON_CALL(get_features, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(get_constants_of_type, constants_of_type(_)).WillByDefault(ReturnRef(none));
        ON_CALL(get_constants_of_type, constants_of_type(0)).WillByDefault(ReturnRef(boolean_constants));
        ON_CALL(get_constants_of_type, constants_of_type(1)).WillByDefault(ReturnRef(nat_constants));
        ON_CALL(get_features_of_type, features_of_type(_)).WillByDefault(ReturnRef(none));
        ON_CALL(get_features_of_type, features_of_type(0)).WillByDefault(ReturnRef(boolean_features));
        ON_CALL(get_features_of_type, features_of_type(1)).WillByDefault(ReturnRef(nat_features));
    }
};

OperandTableTest::OperandTableTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , zero{.tag = 2, .args = {}}
    , true_constant{feature::constant{.literal = &truth}}
    , zero_constant{feature::constant{.literal = &zero}}
    , false_constant{feature::constant{.literal = &falsity}}
    , first_variable{feature::var{.index = 0}}
    , second_variable{feature::var{.index = 1}}
    , constants{
          constant_operand{.expression = true_constant, .literal = &truth, .type = 0},
          constant_operand{.expression = zero_constant, .literal = &zero, .type = 1},
          constant_operand{.expression = false_constant, .literal = &falsity, .type = 0}}
    , features{
          candidate{.expression = first_variable, .cost = 1,
                    .observed = typed_column{.type = 0, .cells = {&falsity, &truth}}},
          candidate{.expression = second_variable, .cost = 1,
                    .observed = typed_column{.type = 1, .cells = {&zero, &truth}}}}
    , boolean_constants{0, 2}
    , nat_constants{1}
    , boolean_features{0}
    , nat_features{1}
    , none()
    , get_constants()
    , get_constants_of_type()
    , get_features()
    , get_features_of_type() {}

}

TEST_F(OperandTableTest, OperandsPastTheConstantsAreFeatures) {
    EXPECT_FALSE(table.is_feature(2));
    EXPECT_TRUE(table.is_feature(3));
}

TEST_F(OperandTableTest, ATypesOperandsAreItsConstantsAndItsFeatures) {
    EXPECT_EQ(table.count_of_type(0), 3u);
    EXPECT_EQ(table.constant_count_of_type(0), 2u);
    EXPECT_EQ(table.count_of_type(7), 0u);
}

TEST_F(OperandTableTest, ATypesOperandsAreNumberedConstantsFirst) {
    EXPECT_EQ(table.operand_of_type(0, 0), 0u);
    EXPECT_EQ(table.operand_of_type(0, 1), 2u);
    EXPECT_EQ(table.operand_of_type(0, 2), 3u);
    EXPECT_EQ(table.operand_of_type(1, 1), 4u);
}

TEST_F(OperandTableTest, AnOperandsExpressionComesFromItsStore) {
    EXPECT_EQ(table.expression(1), zero_constant);
    EXPECT_EQ(table.expression(4), second_variable);
}

TEST_F(OperandTableTest, AConstantGivesItsLiteralOnEveryRowAndAFeatureItsCell) {
    EXPECT_EQ(table.cell(0, 1), &truth);
    EXPECT_EQ(table.cell(3, 0), &falsity);
    EXPECT_EQ(table.cell(3, 1), &truth);
}

TEST_F(OperandTableTest, WithNoFeaturesEveryOperandIsAConstant) {
    features.clear();
    nat_features.clear();
    boolean_features.clear();
    EXPECT_EQ(table.count_of_type(0), 2u);
    EXPECT_FALSE(table.is_feature(2));
}

TEST_F(OperandTableTest, AFeaturesOperandIsOffsetByEveryConstantNotJustItsTypes) {
    EXPECT_EQ(table.operand_of_type(1, 1), constants.size() + 1);
}

TEST_F(OperandTableTest, ATypeWithOnlyFeaturesNumbersThemPastTheConstants) {
    nat_constants.clear();
    EXPECT_EQ(table.constant_count_of_type(1), 0u);
    EXPECT_EQ(table.operand_of_type(1, 0), 4u);
}
