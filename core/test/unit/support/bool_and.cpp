#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/bool_and.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_ref, const std::vector<const value*>&));
};

// The user boolean type is type 5: false is its tag 0, true its tag 1.
struct BoolAndTest : public ::testing::Test {
    BoolAndTest();
    ctor_ref false_ctor;
    ctor_ref true_ctor;
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    bool_and<NiceMock<MockMakeValue>> primitive{make_value, false_ctor, true_ctor};

    void SetUp() override {
        ON_CALL(make_value, make(false_ctor, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

BoolAndTest::BoolAndTest()
    : false_ctor{.type = 5, .tag = 0}
    , true_ctor{.type = 5, .tag = 1}
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , make_value() {}

}

TEST_F(BoolAndTest, OnlyTwoTruesGiveTrue) {
    EXPECT_EQ(primitive({&truth, &truth}), &truth);
    EXPECT_EQ(primitive({&truth, &falsity}), &falsity);
    EXPECT_EQ(primitive({&falsity, &truth}), &falsity);
    EXPECT_EQ(primitive({&falsity, &falsity}), &falsity);
}

TEST_F(BoolAndTest, ResultsAreMadeThroughTheValuePool) {
    EXPECT_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillOnce(Return(&truth));
    primitive({&truth, &truth});
}
