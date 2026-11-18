#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/value_equality.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_id, const std::vector<const value*>&));
};

struct ValueEqualityTest : public ::testing::Test {
    ValueEqualityTest();
    value falsity;
    value truth;
    value zero;
    value one;
    NiceMock<MockMakeValue> make_value;
    value_equality<NiceMock<MockMakeValue>> primitive{
        make_value, 7, boolean_encoding{.type = 5, .false_ctor = 0, .true_ctor = 1}};

    void SetUp() override {
        ON_CALL(make_value, make(0, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(1, std::vector<const value*>{})).WillByDefault(Return(&truth));
    }
};

ValueEqualityTest::ValueEqualityTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , zero{.ctor = 2, .fields = {}}
    , one{.ctor = 3, .fields = {&zero}}
    , make_value() {}

}

TEST_F(ValueEqualityTest, TheSignatureComparesTwoOperandsIntoABoolean) {
    EXPECT_EQ(primitive.signature(), (primitive_signature{.arg_types = {7, 7}, .result_type = 5}));
}

TEST_F(ValueEqualityTest, TheSameValueIsEqual) {
    EXPECT_EQ(primitive.apply({&one, &one}), &truth);
}

TEST_F(ValueEqualityTest, DifferentValuesAreNotEqual) {
    EXPECT_EQ(primitive.apply({&one, &zero}), &falsity);
}

TEST_F(ValueEqualityTest, EqualityIsByIdentityOfInternedValues) {
    const value copy_of_one{.ctor = 3, .fields = {&zero}};
    EXPECT_EQ(primitive.apply({&one, &copy_of_one}), &falsity);
}

TEST_F(ValueEqualityTest, EqualityIsSymmetric) {
    EXPECT_EQ(primitive.apply({&zero, &one}), primitive.apply({&one, &zero}));
}

TEST_F(ValueEqualityTest, ResultsAreMadeThroughTheValuePool) {
    EXPECT_CALL(make_value, make(1, std::vector<const value*>{})).WillOnce(Return(&truth));
    primitive.apply({&zero, &zero});
}
