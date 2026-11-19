#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "support/value_equality.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_ref, const std::vector<const value*>&));
};

// The user boolean type is type 5: false is its tag 0, true its tag 1.
struct ValueEqualityTest : public ::testing::Test {
    ValueEqualityTest();
    ctor_ref false_ctor;
    ctor_ref true_ctor;
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    value_equality<NiceMock<MockMakeValue>> primitive{make_value, false_ctor, true_ctor};

    void SetUp() override {
        ON_CALL(make_value, make(false_ctor, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

ValueEqualityTest::ValueEqualityTest()
    : false_ctor{.type = 5, .tag = 0}
    , true_ctor{.type = 5, .tag = 1}
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , make_value() {}

}

TEST_F(ValueEqualityTest, TheSameValueIsEqual) {
    const value zero{.ctor = ctor_ref{.type = 2, .tag = 0}, .args = {}};
    EXPECT_EQ(primitive({&zero, &zero}), &truth);
}

TEST_F(ValueEqualityTest, DifferentValuesAreNotEqual) {
    const value zero{.ctor = ctor_ref{.type = 2, .tag = 0}, .args = {}};
    const value one{.ctor = ctor_ref{.type = 2, .tag = 1}, .args = {&zero}};
    EXPECT_EQ(primitive({&one, &zero}), &falsity);
}

TEST_F(ValueEqualityTest, EqualityIsByIdentityOfInternedValues) {
    const value zero{.ctor = ctor_ref{.type = 2, .tag = 0}, .args = {}};
    const value copy_of_zero{.ctor = ctor_ref{.type = 2, .tag = 0}, .args = {}};
    EXPECT_EQ(primitive({&zero, &copy_of_zero}), &falsity);
}

TEST_F(ValueEqualityTest, EqualityIsSymmetric) {
    const value zero{.ctor = ctor_ref{.type = 2, .tag = 0}, .args = {}};
    const value one{.ctor = ctor_ref{.type = 2, .tag = 1}, .args = {&zero}};
    EXPECT_EQ(primitive({&zero, &one}), primitive({&one, &zero}));
}

TEST_F(ValueEqualityTest, ResultsAreMadeThroughTheValuePool) {
    const value zero{.ctor = ctor_ref{.type = 2, .tag = 0}, .args = {}};
    EXPECT_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillOnce(Return(&truth));
    primitive({&zero, &zero});
}
