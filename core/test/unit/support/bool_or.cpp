#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/bool_or.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (size_t, const std::vector<const value*>&));
};

// The user boolean type is type 5: false is its tag 0, true its tag 1.
struct BoolOrTest : public ::testing::Test {
    BoolOrTest();
    size_t false_tag;
    size_t true_tag;
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    bool_or<NiceMock<MockMakeValue>> primitive{make_value, false_tag, true_tag};

    void SetUp() override {
        ON_CALL(make_value, make(false_tag, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_tag, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

BoolOrTest::BoolOrTest()
    : false_tag(0)
    , true_tag(1)
    , falsity{.tag = false_tag, .args = {}}
    , truth{.tag = true_tag, .args = {}}
    , make_value() {}

}

TEST_F(BoolOrTest, OnlyTwoFalsesGiveFalse) {
    EXPECT_EQ(primitive({&truth, &truth}), &truth);
    EXPECT_EQ(primitive({&truth, &falsity}), &truth);
    EXPECT_EQ(primitive({&falsity, &truth}), &truth);
    EXPECT_EQ(primitive({&falsity, &falsity}), &falsity);
}

TEST_F(BoolOrTest, ResultsAreMadeThroughTheValuePool) {
    EXPECT_CALL(make_value, make(false_tag, std::vector<const value*>{})).WillOnce(Return(&falsity));
    primitive({&falsity, &falsity});
}
