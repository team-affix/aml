#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/nat_add.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_ref, const std::vector<const value*>&));
};

// The user naturals are type 4: zero is its tag 0, suc its tag 1.
struct NatAddTest : public ::testing::Test {
    NatAddTest();
    ctor_ref zero_ctor;
    ctor_ref suc_ctor;
    value zero;
    value one;
    value two;
    value three;
    NiceMock<MockMakeValue> make_value;
    nat_add<NiceMock<MockMakeValue>> primitive{make_value, suc_ctor};

    void SetUp() override {
        ON_CALL(make_value, make(suc_ctor, std::vector<const value*>{&zero})).WillByDefault(Return(&one));
        ON_CALL(make_value, make(suc_ctor, std::vector<const value*>{&one})).WillByDefault(Return(&two));
        ON_CALL(make_value, make(suc_ctor, std::vector<const value*>{&two})).WillByDefault(Return(&three));
    }
};

NatAddTest::NatAddTest()
    : zero_ctor{.type = 4, .tag = 0}
    , suc_ctor{.type = 4, .tag = 1}
    , zero{.ctor = zero_ctor, .args = {}}
    , one{.ctor = suc_ctor, .args = {&zero}}
    , two{.ctor = suc_ctor, .args = {&one}}
    , three{.ctor = suc_ctor, .args = {&two}}
    , make_value() {}

}

TEST_F(NatAddTest, AddingZeroGivesTheLeftOperand) {
    EXPECT_EQ(primitive({&two, &zero}), &two);
}

TEST_F(NatAddTest, EachSuccessorOfTheRightOperandWrapsTheLeftOperand) {
    EXPECT_EQ(primitive({&one, &two}), &three);
    EXPECT_EQ(primitive({&zero, &one}), &one);
}

TEST_F(NatAddTest, ZeroPlusZeroIsZero) {
    EXPECT_CALL(make_value, make(_, _)).Times(0);
    EXPECT_EQ(primitive({&zero, &zero}), &zero);
}

TEST_F(NatAddTest, AdditionMakesOneSuccessorPerSuccessorOfTheRightOperand) {
    EXPECT_CALL(make_value, make(suc_ctor, _)).Times(2).WillOnce(Return(&two)).WillOnce(Return(&three));
    EXPECT_EQ(primitive({&one, &two}), &three);
}

TEST_F(NatAddTest, TheRightOperandOnlyCountsSuccessors) {
    EXPECT_EQ(primitive({&zero, &three}), &three);
}
