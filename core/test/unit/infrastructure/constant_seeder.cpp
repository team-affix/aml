#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_seeder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace {

struct MockInsertConstant {
    MOCK_METHOD(void, insert, (const constant_operand&));
};

struct ConstantSeederTest : public ::testing::Test {
    ConstantSeederTest();
    value falsity;
    value truth;
    value zero;
    feature false_feature;
    feature true_feature;
    feature zero_feature;
    std::vector<typed_constant> offered;
    NiceMock<MockInsertConstant> insert_constant;
    constant_seeder<NiceMock<MockInsertConstant>> seeder{insert_constant};
};

ConstantSeederTest::ConstantSeederTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , zero{.ctor = 0, .args = {}}
    , false_feature{feature::constant{.literal = falsity}}
    , true_feature{feature::constant{.literal = truth}}
    , zero_feature{feature::constant{.literal = zero}}
    , offered{
          typed_constant{.type = 0, .literal = falsity}, typed_constant{.type = 0, .literal = truth},
          typed_constant{.type = 1, .literal = zero}}
    , insert_constant() {}

}

TEST_F(ConstantSeederTest, EveryOfferedConstantIsInsertedInOrderWithItsTypeAndFeature) {
    InSequence order;
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = false_feature, .literal = falsity, .type = 0}));
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = true_feature, .literal = truth, .type = 0}));
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = zero_feature, .literal = zero, .type = 1}));
    seeder.seed(offered);
}

TEST_F(ConstantSeederTest, NoOfferedConstantsInsertNothing) {
    EXPECT_CALL(insert_constant, insert(_)).Times(0);
    seeder.seed({});
}

TEST_F(ConstantSeederTest, TheSameLiteralUnderTwoTypesIsTwoConstants) {
    const std::vector<typed_constant> shared{
        typed_constant{.type = 0, .literal = zero}, typed_constant{.type = 1, .literal = zero}};
    EXPECT_CALL(insert_constant, insert(_)).Times(2);
    seeder.seed(shared);
}
