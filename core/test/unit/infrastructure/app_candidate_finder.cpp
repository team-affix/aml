#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/app_candidate_finder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockApplyPrimitiveToOperands {
    MOCK_METHOD(column, apply, (prim_id, const std::vector<size_t>&, size_t), (const));
};

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column_view&), (const));
};

struct MockGetOperandExpression {
    MOCK_METHOD(const feature&, expression, (size_t), (const));
};

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature&), (const));
};

using finder_t = app_candidate_finder<
    NiceMock<MockApplyPrimitiveToOperands>,
    NiceMock<MockFindUniformValue>,
    NiceMock<MockGetOperandExpression>,
    NiceMock<MockSizeFeature>>;

// Operand 0 is the constant true and operand 1 the variable x0. Primitive 2 applied to them over two rows gives a
// heterogeneous result of type 5.
struct AppCandidateFinderTest : public ::testing::Test {
    AppCandidateFinderTest();
    value falsity;
    value truth;
    feature true_constant;
    feature variable;
    feature application;
    column result;
    NiceMock<MockApplyPrimitiveToOperands> apply_primitive_to_operands;
    NiceMock<MockFindUniformValue> find_uniform_value;
    NiceMock<MockGetOperandExpression> get_operand_expression;
    NiceMock<MockSizeFeature> size_feature;
    finder_t finder{apply_primitive_to_operands, find_uniform_value, get_operand_expression, size_feature};

    void SetUp() override {
        ON_CALL(apply_primitive_to_operands, apply(2, _, _)).WillByDefault(Return(result));
        ON_CALL(find_uniform_value, find(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(get_operand_expression, expression(0)).WillByDefault(ReturnRef(true_constant));
        ON_CALL(get_operand_expression, expression(1)).WillByDefault(ReturnRef(variable));
        ON_CALL(size_feature, size(application)).WillByDefault(Return(3));
    }
};

AppCandidateFinderTest::AppCandidateFinderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , true_constant{feature::constant{.literal = truth}}
    , variable{feature::var{.index = 0}}
    , application{feature::app{.prim = 2, .args = {true_constant, variable}}}
    , result{falsity, truth}
    , apply_primitive_to_operands()
    , find_uniform_value()
    , get_operand_expression()
    , size_feature() {}

}

TEST_F(AppCandidateFinderTest, ThePrimitiveIsAppliedToTheOperandsOverTheRows) {
    EXPECT_CALL(apply_primitive_to_operands, apply(2, std::vector<size_t>{0, 1}, 2)).WillOnce(Return(result));
    finder.find(2, 5, {0, 1}, 2);
}

TEST_F(AppCandidateFinderTest, AHeterogeneousApplicationIsACandidateOverTheOperandsExpressions) {
    EXPECT_EQ(finder.find(2, 5, {0, 1}, 2),
              (candidate{.expression = application,
                         .cost = 3,
                         .observed = column_view::of(typed_column{.type = 5, .cells = result})}));
}

TEST_F(AppCandidateFinderTest, AHomogeneousApplicationGivesNoCandidateAndReadsNoExpression) {
    ON_CALL(find_uniform_value, find(column_view::of(typed_column{.type = 5, .cells = result})))
        .WillByDefault(Return(&truth));
    EXPECT_CALL(get_operand_expression, expression(_)).Times(0);
    EXPECT_CALL(size_feature, size(_)).Times(0);
    EXPECT_FALSE(finder.find(2, 5, {0, 1}, 2).has_value());
}

TEST_F(AppCandidateFinderTest, TheResultsCostIsItsFeaturesSize) {
    EXPECT_CALL(size_feature, size(application)).WillOnce(Return(11));
    EXPECT_EQ(finder.find(2, 5, {0, 1}, 2).value().cost, 11u);
}

TEST_F(AppCandidateFinderTest, TheFeatureIsMadeFromTheOperandsExpressionsInOrder) {
    ON_CALL(get_operand_expression, expression(0)).WillByDefault(ReturnRef(variable));
    ON_CALL(get_operand_expression, expression(1)).WillByDefault(ReturnRef(true_constant));
    EXPECT_EQ(finder.find(2, 5, {1, 0}, 2).value().expression, application);
}

TEST_F(AppCandidateFinderTest, HomogeneityIsJudgedOnTheAppliedColumn) {
    EXPECT_CALL(find_uniform_value, find(column_view::of(typed_column{.type = 5, .cells = result})))
        .WillOnce(Return(std::nullopt));
    finder.find(2, 5, {0, 1}, 2);
}
