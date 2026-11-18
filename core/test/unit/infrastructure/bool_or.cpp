#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/bool_or.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_id, const std::vector<const value*>&));
};

struct BoolOrTest : public ::testing::Test {
    BoolOrTest();
    value falsity;
    value truth;
    NiceMock<MockMakeValue> make_value;
    bool_or<NiceMock<MockMakeValue>> primitive{
        make_value, boolean_encoding{.type = 5, .false_ctor = 0, .true_ctor = 1}};

    void SetUp() override {
        ON_CALL(make_value, make(0, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(1, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

BoolOrTest::BoolOrTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , make_value() {}

}

TEST_F(BoolOrTest, TheSignatureIsOverBooleans) {
    EXPECT_EQ(primitive.signature(), (primitive_signature{.arg_types = {5, 5}, .result_type = 5}));
}

TEST_F(BoolOrTest, OnlyTwoFalsesGiveFalse) {
    EXPECT_EQ(primitive.apply({&truth, &truth}), &truth);
    EXPECT_EQ(primitive.apply({&truth, &falsity}), &truth);
    EXPECT_EQ(primitive.apply({&falsity, &truth}), &truth);
    EXPECT_EQ(primitive.apply({&falsity, &falsity}), &falsity);
}
