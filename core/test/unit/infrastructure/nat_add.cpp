#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/nat_add.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_id, const std::vector<const value*>&));
};

struct NatAddTest : public ::testing::Test {
    NatAddTest();
    value zero;
    value one;
    value two;
    value three;
    NiceMock<MockMakeValue> make_value;
    nat_add<NiceMock<MockMakeValue>> primitive{make_value, nat_encoding{.type = 4, .zero_ctor = 0, .suc_ctor = 1}};

    void SetUp() override {
        ON_CALL(make_value, make(1, std::vector<const value*>{&zero})).WillByDefault(Return(&one));
        ON_CALL(make_value, make(1, std::vector<const value*>{&one})).WillByDefault(Return(&two));
        ON_CALL(make_value, make(1, std::vector<const value*>{&two})).WillByDefault(Return(&three));
    }
};

NatAddTest::NatAddTest()
    : zero{.ctor = 0, .fields = {}}
    , one{.ctor = 1, .fields = {&zero}}
    , two{.ctor = 1, .fields = {&one}}
    , three{.ctor = 1, .fields = {&two}}
    , make_value() {}

}

TEST_F(NatAddTest, TheSignatureAddsTwoNaturals) {
    EXPECT_EQ(primitive.signature(), (primitive_signature{.arg_types = {4, 4}, .result_type = 4}));
}

TEST_F(NatAddTest, AddingZeroGivesTheLeftOperand) {
    EXPECT_EQ(primitive.apply({&two, &zero}), &two);
}

TEST_F(NatAddTest, EachSuccessorOfTheRightOperandWrapsTheLeftOperand) {
    EXPECT_EQ(primitive.apply({&one, &two}), &three);
    EXPECT_EQ(primitive.apply({&zero, &one}), &one);
}

TEST_F(NatAddTest, ZeroPlusZeroIsZero) {
    EXPECT_CALL(make_value, make(_, _)).Times(0);
    EXPECT_EQ(primitive.apply({&zero, &zero}), &zero);
}

TEST_F(NatAddTest, AdditionMakesOneSuccessorPerSuccessorOfTheRightOperand) {
    EXPECT_CALL(make_value, make(1, _)).Times(2).WillOnce(Return(&two)).WillOnce(Return(&three));
    EXPECT_EQ(primitive.apply({&one, &two}), &three);
}

TEST_F(NatAddTest, TheRightOperandOnlyCountsSuccessors) {
    EXPECT_EQ(primitive.apply({&zero, &three}), &three);
}
