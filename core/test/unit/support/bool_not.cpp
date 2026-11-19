#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/bool_not.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_ref, const std::vector<const value*>&));
};

// The user boolean type is type 5: false is its tag 0, true its tag 1.
struct BoolNotTest : public ::testing::Test {
    BoolNotTest();
    ctor_ref false_ctor;
    ctor_ref true_ctor;
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    bool_not<NiceMock<MockMakeValue>> primitive{make_value, false_ctor, true_ctor};

    void SetUp() override {
        ON_CALL(make_value, make(false_ctor, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

BoolNotTest::BoolNotTest()
    : false_ctor{.type = 5, .tag = 0}
    , true_ctor{.type = 5, .tag = 1}
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , make_value() {}

}

TEST_F(BoolNotTest, TrueBecomesFalseAndFalseBecomesTrue) {
    EXPECT_EQ(primitive({&truth}), &falsity);
    EXPECT_EQ(primitive({&falsity}), &truth);
}

TEST_F(BoolNotTest, NegatingTwiceGivesTheOriginal) {
    EXPECT_EQ(primitive({primitive({&truth})}), &truth);
}

TEST_F(BoolNotTest, ResultsAreMadeThroughTheValuePool) {
    EXPECT_CALL(make_value, make(false_ctor, std::vector<const value*>{})).WillOnce(Return(&falsity));
    primitive({&truth});
}

TEST_F(BoolNotTest, OnlyTheTrueConstructorCountsAsTrue) {
    const value other{.ctor = ctor_ref{.type = 6, .tag = 1}, .args = {}};
    EXPECT_EQ(primitive({&other}), &truth);
}
