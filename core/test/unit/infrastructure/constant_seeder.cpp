#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_seeder.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

namespace {

struct MockGetTypeCount {
    MOCK_METHOD(size_t, type_count, (), (const));
};

struct MockEnumerateValues {
    MOCK_METHOD(std::vector<const value*>, enumerate, (type_id, size_t));
};

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
    NiceMock<MockGetTypeCount> get_type_count;
    NiceMock<MockEnumerateValues> enumerate_values;
    NiceMock<MockMakeConstantFeature> make_constant_feature;
    NiceMock<MockInsertConstant> insert_constant;
    constant_seeder<
        NiceMock<MockGetTypeCount>, NiceMock<MockEnumerateValues>, NiceMock<MockMakeConstantFeature>,
        NiceMock<MockInsertConstant>>
        seeder{get_type_count, enumerate_values, make_constant_feature, insert_constant};

    void SetUp() override {
        ON_CALL(get_type_count, type_count()).WillByDefault(Return(2));
        ON_CALL(enumerate_values, enumerate(0, 4)).WillByDefault(Return(std::vector<const value*>{&falsity, &truth}));
        ON_CALL(enumerate_values, enumerate(1, 4)).WillByDefault(Return(std::vector<const value*>{&zero}));
        ON_CALL(make_constant_feature, make_constant(&falsity)).WillByDefault(Return(&false_feature));
        ON_CALL(make_constant_feature, make_constant(&truth)).WillByDefault(Return(&true_feature));
        ON_CALL(make_constant_feature, make_constant(&zero)).WillByDefault(Return(&zero_feature));
    }
};

ConstantSeederTest::ConstantSeederTest()
    : falsity{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , truth{.ctor = {.type = 0, .tag = 1}, .args = {}}
    , zero{.ctor = {.type = 0, .tag = 2}, .args = {}}
    , false_feature{feature::constant{.literal = &falsity}}
    , true_feature{feature::constant{.literal = &truth}}
    , zero_feature{feature::constant{.literal = &zero}}
    , get_type_count()
    , enumerate_values()
    , make_constant_feature()
    , insert_constant() {}

}

TEST_F(ConstantSeederTest, EveryEnumeratedValueOfEveryTypeIsInsertedAsAConstant) {
    InSequence order;
    EXPECT_CALL(insert_constant,
                insert(constant_operand{.expression = &false_feature, .literal = &falsity, .type = 0}));
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = &true_feature, .literal = &truth, .type = 0}));
    EXPECT_CALL(insert_constant, insert(constant_operand{.expression = &zero_feature, .literal = &zero, .type = 1}));
    seeder.seed(4);
}

TEST_F(ConstantSeederTest, EachTypeIsEnumeratedUpToTheGivenSize) {
    EXPECT_CALL(enumerate_values, enumerate(0, 4)).WillOnce(Return(std::vector<const value*>{}));
    EXPECT_CALL(enumerate_values, enumerate(1, 4)).WillOnce(Return(std::vector<const value*>{}));
    seeder.seed(4);
}

TEST_F(ConstantSeederTest, ATypeWithNoValuesInsertsNothing) {
    ON_CALL(get_type_count, type_count()).WillByDefault(Return(1));
    ON_CALL(enumerate_values, enumerate(0, 4)).WillByDefault(Return(std::vector<const value*>{}));
    EXPECT_CALL(insert_constant, insert(_)).Times(0);
    seeder.seed(4);
}

TEST_F(ConstantSeederTest, NoTypesSeedNothing) {
    ON_CALL(get_type_count, type_count()).WillByDefault(Return(0));
    EXPECT_CALL(enumerate_values, enumerate(_, _)).Times(0);
    EXPECT_CALL(insert_constant, insert(_)).Times(0);
    seeder.seed(4);
}

TEST_F(ConstantSeederTest, EachConstantsFeatureIsMadeFromItsLiteral) {
    EXPECT_CALL(make_constant_feature, make_constant(&falsity)).WillOnce(Return(&false_feature));
    EXPECT_CALL(make_constant_feature, make_constant(&truth)).WillOnce(Return(&true_feature));
    EXPECT_CALL(make_constant_feature, make_constant(&zero)).WillOnce(Return(&zero_feature));
    seeder.seed(4);
}
