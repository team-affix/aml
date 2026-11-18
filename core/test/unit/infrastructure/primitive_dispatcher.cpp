#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/primitive_dispatcher.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

namespace {

struct MockPrimitive {
    MOCK_METHOD(primitive_signature, signature, (), (const));
    MOCK_METHOD(const value*, apply, (const std::vector<const value*>&), (const));
};

using test_primitive = std::variant<const NiceMock<MockPrimitive>*>;

struct PrimitiveDispatcherTest : public ::testing::Test {
    PrimitiveDispatcherTest();
    value zero;
    value one;
    NiceMock<MockPrimitive> first;
    NiceMock<MockPrimitive> second;
    primitive_dispatcher<test_primitive> dispatcher;
};

PrimitiveDispatcherTest::PrimitiveDispatcherTest()
    : zero{.ctor = 0, .fields = {}}
    , one{.ctor = 1, .fields = {&zero}}
    , first()
    , second()
    , dispatcher(std::vector<test_primitive>{&first, &second}) {}

}

TEST_F(PrimitiveDispatcherTest, CountsThePrimitives) {
    EXPECT_EQ(dispatcher.count(), 2u);
}

TEST_F(PrimitiveDispatcherTest, ASignatureComesFromThePrimitiveAtThatId) {
    const primitive_signature signature{.arg_types = {3}, .result_type = 4};
    EXPECT_CALL(second, signature()).WillOnce(Return(signature));
    EXPECT_EQ(dispatcher.signature(1), signature);
}

TEST_F(PrimitiveDispatcherTest, AnApplicationIsForwardedToThePrimitiveAtThatId) {
    EXPECT_CALL(first, apply(std::vector<const value*>{&zero})).WillOnce(Return(&one));
    EXPECT_EQ(dispatcher.apply(0, {&zero}), &one);
}

TEST_F(PrimitiveDispatcherTest, NoPrimitivesCountZero) {
    const primitive_dispatcher<test_primitive> empty(std::vector<test_primitive>{});
    EXPECT_EQ(empty.count(), 0u);
}

TEST_F(PrimitiveDispatcherTest, OnlyThePrimitiveAtTheIdIsApplied) {
    EXPECT_CALL(first, apply(_)).Times(0);
    EXPECT_CALL(second, apply(std::vector<const value*>{&one})).WillOnce(Return(&zero));
    EXPECT_EQ(dispatcher.apply(1, {&one}), &zero);
}

TEST_F(PrimitiveDispatcherTest, ThePrimitivesAreReferredToNotCopied) {
    const primitive_signature signature{.arg_types = {}, .result_type = 9};
    ON_CALL(first, signature()).WillByDefault(Return(signature));
    EXPECT_EQ(dispatcher.signature(0), signature);
}
