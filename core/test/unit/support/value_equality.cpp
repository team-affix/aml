#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/value_equality.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (size_t, const std::vector<const value*>&));
};

// The user boolean type is type 5: false is its tag 0, true its tag 1.
struct ValueEqualityTest : public ::testing::Test {
    ValueEqualityTest();
    size_t false_tag;
    size_t true_tag;
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    value_equality<NiceMock<MockMakeValue>> primitive{make_value, false_tag, true_tag};

    void SetUp() override {
        ON_CALL(make_value, make(false_tag, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_tag, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

ValueEqualityTest::ValueEqualityTest()
    : false_tag(0)
    , true_tag(1)
    , falsity{.tag = false_tag, .args = {}}
    , truth{.tag = true_tag, .args = {}}
    , make_value() {}

}

TEST_F(ValueEqualityTest, TheSameValueIsEqual) {
    const value zero{.tag = 0, .args = {}};
    EXPECT_EQ(primitive({&zero, &zero}), &truth);
}

TEST_F(ValueEqualityTest, DifferentValuesAreNotEqual) {
    const value zero{.tag = 0, .args = {}};
    const value one{.tag = 1, .args = {&zero}};
    EXPECT_EQ(primitive({&one, &zero}), &falsity);
}

TEST_F(ValueEqualityTest, EqualityIsByIdentityOfInternedValues) {
    const value zero{.tag = 0, .args = {}};
    const value copy_of_zero{.tag = 0, .args = {}};
    EXPECT_EQ(primitive({&zero, &copy_of_zero}), &falsity);
}

TEST_F(ValueEqualityTest, EqualityIsSymmetric) {
    const value zero{.tag = 0, .args = {}};
    const value one{.tag = 1, .args = {&zero}};
    EXPECT_EQ(primitive({&zero, &one}), primitive({&one, &zero}));
}

TEST_F(ValueEqualityTest, ResultsAreMadeThroughTheValuePool) {
    const value zero{.tag = 0, .args = {}};
    EXPECT_CALL(make_value, make(true_tag, std::vector<const value*>{})).WillOnce(Return(&truth));
    primitive({&zero, &zero});
}
