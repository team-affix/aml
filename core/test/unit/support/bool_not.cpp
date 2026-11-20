#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/bool_not.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (size_t, const std::vector<const value*>&));
};

// The user boolean type is type 5: false is its tag 0, true its tag 1.
struct BoolNotTest : public ::testing::Test {
    BoolNotTest();
    size_t false_tag;
    size_t true_tag;
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    bool_not<NiceMock<MockMakeValue>> primitive{make_value, false_tag, true_tag};

    void SetUp() override {
        ON_CALL(make_value, make(false_tag, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_tag, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

BoolNotTest::BoolNotTest()
    : false_tag(0)
    , true_tag(1)
    , falsity{.tag = false_tag, .args = {}}
    , truth{.tag = true_tag, .args = {}}
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
    EXPECT_CALL(make_value, make(false_tag, std::vector<const value*>{})).WillOnce(Return(&falsity));
    primitive({&truth});
}

TEST_F(BoolNotTest, AnyTagButTrueCountsAsFalse) {
    const value other{.tag = 7, .args = {}};
    EXPECT_EQ(primitive({&other}), &truth);
}
