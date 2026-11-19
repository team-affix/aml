#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_seeder.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeConstantFeature {
    MOCK_METHOD(const feature*, make_constant, (const value*));
};

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
    NiceMock<MockMakeConstantFeature> make_constant_feature;
    NiceMock<MockInsertConstant> insert_constant;
    constant_seeder<NiceMock<MockMakeConstantFeature>, NiceMock<MockInsertConstant>> seeder{
        make_constant_feature, insert_constant};

    void SetUp() override {
        ON_CALL(make_constant_feature, make_constant(&falsity)).WillByDefault(Return(&false_feature));
        ON_CALL(make_constant_feature, make_constant(&truth)).WillByDefault(Return(&true_feature));
        ON_CALL(make_constant_feature, make_constant(&zero)).WillByDefault(Return(&zero_feature));
    }
};

ConstantSeederTest::ConstantSeederTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , zero{.tag = 0, .args = {}}
    , false_feature{feature::constant{.literal = &falsity}}
    , true_feature{feature::constant{.literal = &truth}}
    , zero_feature{feature::constant{.literal = &zero}}
    , offered{
          typed_constant{.type = 0, .literal = &falsity}, typed_constant{.type = 0, .literal = &truth},
          typed_constant{.type = 1, .literal = &zero}}
    , make_constant_feature()
    , insert_constant() {}

}

TEST_F(ConstantSeederTest, EveryOfferedConstantIsInsertedInOrderWithItsTypeAndFeature) {
    InSequence order;
    EXPECT_CALL(insert_constant,
                insert(constant_operand{.expression = &false_feature, .literal = &falsity, .type = 0}));
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = &true_feature, .literal = &truth, .type = 0}));
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = &zero_feature, .literal = &zero, .type = 1}));
    seeder.seed(offered);
}

TEST_F(ConstantSeederTest, EachConstantsFeatureIsMadeFromItsLiteral) {
    EXPECT_CALL(make_constant_feature, make_constant(&falsity)).WillOnce(Return(&false_feature));
    EXPECT_CALL(make_constant_feature, make_constant(&truth)).WillOnce(Return(&true_feature));
    EXPECT_CALL(make_constant_feature, make_constant(&zero)).WillOnce(Return(&zero_feature));
    seeder.seed(offered);
}

TEST_F(ConstantSeederTest, NoOfferedConstantsInsertNothing) {
    EXPECT_CALL(insert_constant, insert(_)).Times(0);
    seeder.seed({});
}

TEST_F(ConstantSeederTest, TheSameLiteralUnderTwoTypesIsTwoConstants) {
    const std::vector<typed_constant> shared{
        typed_constant{.type = 0, .literal = &zero}, typed_constant{.type = 1, .literal = &zero}};
    EXPECT_CALL(insert_constant, insert(_)).Times(2);
    seeder.seed(shared);
}
