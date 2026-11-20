#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/nat_add.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (size_t, const std::vector<const value*>&));
};

// The user naturals are type 4: zero is its tag 0, suc its tag 1.
struct NatAddTest : public ::testing::Test {
    NatAddTest();
    size_t zero_tag;
    size_t suc_tag;
    value zero;
    value one;
    value two;
    value three;
    NiceMock<MockMakeValue> make_value;
    nat_add<NiceMock<MockMakeValue>> primitive{make_value, suc_tag};

    void SetUp() override {
        ON_CALL(make_value, make(suc_tag, std::vector<const value*>{&zero})).WillByDefault(Return(&one));
        ON_CALL(make_value, make(suc_tag, std::vector<const value*>{&one})).WillByDefault(Return(&two));
        ON_CALL(make_value, make(suc_tag, std::vector<const value*>{&two})).WillByDefault(Return(&three));
    }
};

NatAddTest::NatAddTest()
    : zero_tag(0)
    , suc_tag(1)
    , zero{.tag = zero_tag, .args = {}}
    , one{.tag = suc_tag, .args = {&zero}}
    , two{.tag = suc_tag, .args = {&one}}
    , three{.tag = suc_tag, .args = {&two}}
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
    EXPECT_CALL(make_value, make(suc_tag, _)).Times(2).WillOnce(Return(&two)).WillOnce(Return(&three));
    EXPECT_EQ(primitive({&one, &two}), &three);
}

TEST_F(NatAddTest, TheRightOperandOnlyCountsSuccessors) {
    EXPECT_EQ(primitive({&zero, &three}), &three);
}
