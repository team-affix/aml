#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/tiered_argument_chooser.hpp"
#include "value_objects/feature.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockFindArgumentOptions {
    MOCK_METHOD(
        std::vector<size_t>, find, (prim_id, const std::vector<type_id>&, const std::vector<size_t>&), (const));
};

struct MockGetOperandExpression {
    MOCK_METHOD(const feature&, expression, (size_t), (const));
};

struct MockMeasureOrder {
    MOCK_METHOD(size_t, measure, (const feature&), (const));
};

struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

struct TieredArgumentChooserTest : public ::testing::Test {
    TieredArgumentChooserTest();
    feature x0;
    feature x1;
    feature derived;
    NiceMock<MockFindArgumentOptions> find_argument_options;
    NiceMock<MockGetOperandExpression> get_operand_expression;
    NiceMock<MockMeasureOrder> measure_order;
    NiceMock<MockChooseOption> choose_option;
    tiered_argument_chooser<
        NiceMock<MockFindArgumentOptions>, NiceMock<MockGetOperandExpression>, NiceMock<MockMeasureOrder>,
        NiceMock<MockChooseOption>>
        chooser{find_argument_options, get_operand_expression, measure_order, choose_option};

    void SetUp() override {
        ON_CALL(get_operand_expression, expression(4)).WillByDefault(ReturnRef(x0));
        ON_CALL(get_operand_expression, expression(5)).WillByDefault(ReturnRef(x1));
        ON_CALL(get_operand_expression, expression(6)).WillByDefault(ReturnRef(derived));
        ON_CALL(measure_order, measure(x0)).WillByDefault(Return(0));
        ON_CALL(measure_order, measure(x1)).WillByDefault(Return(0));
        ON_CALL(measure_order, measure(derived)).WillByDefault(Return(1));
    }
};

TieredArgumentChooserTest::TieredArgumentChooserTest()
    : x0{feature::var{.index = 0}}
    , x1{feature::var{.index = 1}}
    , derived{feature::app{.prim = 0, .args = {x0, x1}}} {}

}

TEST_F(TieredArgumentChooserTest, EachArgumentIsChosenByTheOrdersOfTheOptionsLeftByTheArgumentsBeforeIt) {
    const std::vector<type_id> arg_types{0, 0};
    InSequence order;
    EXPECT_CALL(find_argument_options, find(3, arg_types, std::vector<size_t>{}))
        .WillOnce(Return(std::vector<size_t>{4, 6}));
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0, 1})).WillOnce(Return(1));
    EXPECT_CALL(find_argument_options, find(3, arg_types, std::vector<size_t>{6}))
        .WillOnce(Return(std::vector<size_t>{4, 5, 6}));
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0, 0, 1})).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(3, arg_types), (std::vector<size_t>{6, 4}));
}

TEST_F(TieredArgumentChooserTest, EachOptionsOrderIsMeasuredFromItsOperandsExpression) {
    ON_CALL(find_argument_options, find(_, _, _)).WillByDefault(Return(std::vector<size_t>{6, 5}));
    EXPECT_CALL(get_operand_expression, expression(_)).Times(AnyNumber());
    EXPECT_CALL(get_operand_expression, expression(6));
    EXPECT_CALL(get_operand_expression, expression(5));
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{1, 0})).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(0, {0}), (std::vector<size_t>{6}));
}

TEST_F(TieredArgumentChooserTest, ANullaryPrimitiveHasNoArguments) {
    EXPECT_CALL(choose_option, choose(_)).Times(0);
    EXPECT_TRUE(chooser.choose(0, {}).empty());
}

TEST_F(TieredArgumentChooserTest, AForcedArgumentIsStillChosenThrough) {
    ON_CALL(find_argument_options, find(_, _, _)).WillByDefault(Return(std::vector<size_t>{5}));
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0})).Times(3).WillRepeatedly(Return(0));
    EXPECT_EQ(chooser.choose(0, {0, 0, 0}), (std::vector<size_t>{5, 5, 5}));
}
