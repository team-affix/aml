#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/bool_not.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_id, const std::vector<const value*>&));
};

struct BoolNotTest : public ::testing::Test {
    BoolNotTest();
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    bool_not<NiceMock<MockMakeValue>> primitive{make_value, boolean_encoding{.type = 5, .false_ctor = 0, .true_ctor = 1}};

    void SetUp() override {
        ON_CALL(make_value, make(0, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(1, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

BoolNotTest::BoolNotTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , make_value() {}

}

TEST_F(BoolNotTest, TheSignatureIsOverBooleans) {
    EXPECT_EQ(primitive.signature(), (primitive_signature{.arg_types = {5}, .result_type = 5}));
}

TEST_F(BoolNotTest, TrueBecomesFalseAndFalseBecomesTrue) {
    EXPECT_EQ(primitive.apply({&truth}), &falsity);
    EXPECT_EQ(primitive.apply({&falsity}), &truth);
}
