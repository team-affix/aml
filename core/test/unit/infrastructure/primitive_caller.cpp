#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/primitive_caller.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockPrimitive {
    MOCK_METHOD(const value*, apply, (const std::vector<const value*>&), (const));
};

struct OtherMockPrimitive {
    MOCK_METHOD(const value*, apply, (const std::vector<const value*>&), (const));
};

using test_primitive = std::variant<const NiceMock<MockPrimitive>*, const NiceMock<OtherMockPrimitive>*>;

struct MockGetPrimitive {
    MOCK_METHOD(const test_primitive&, primitive_at, (prim_id), (const));
};

struct PrimitiveCallerTest : public ::testing::Test {
    PrimitiveCallerTest();
    value zero;
    value one;
    NiceMock<MockPrimitive> first;
    NiceMock<OtherMockPrimitive> second;
    test_primitive first_entry;
    test_primitive second_entry;
    NiceMock<MockGetPrimitive> get_primitive;
    primitive_caller<NiceMock<MockGetPrimitive>> caller{get_primitive};

    void SetUp() override {
        ON_CALL(get_primitive, primitive_at(0)).WillByDefault(ReturnRef(first_entry));
        ON_CALL(get_primitive, primitive_at(1)).WillByDefault(ReturnRef(second_entry));
    }
};

PrimitiveCallerTest::PrimitiveCallerTest()
    : zero{.ctor = 0, .args = {}}
    , one{.ctor = 1, .args = {&zero}}
    , first()
    , second()
    , first_entry(&first)
    , second_entry(&second)
    , get_primitive() {}

}

TEST_F(PrimitiveCallerTest, TheArgumentsGoToThePrimitiveAtThatId) {
    EXPECT_CALL(first, apply(std::vector<const value*>{&zero})).WillOnce(Return(&one));
    EXPECT_EQ(caller.call(0, {&zero}), &one);
}

TEST_F(PrimitiveCallerTest, OnlyThePrimitiveAtThatIdIsCalled) {
    EXPECT_CALL(first, apply(_)).Times(0);
    EXPECT_CALL(second, apply(std::vector<const value*>{&one, &zero})).WillOnce(Return(&zero));
    EXPECT_EQ(caller.call(1, {&one, &zero}), &zero);
}

TEST_F(PrimitiveCallerTest, ANullaryCallPassesNoArguments) {
    EXPECT_CALL(first, apply(std::vector<const value*>{})).WillOnce(Return(&zero));
    EXPECT_EQ(caller.call(0, {}), &zero);
}
