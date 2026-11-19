#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/primitive_caller.hpp"
#include "value_objects/primitive.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetPrimitiveFunction {
    MOCK_METHOD(const primitive&, function, (prim_id), (const));
};

const value* first_argument(const std::vector<const value*>& args) {
    return args.front();
}

const value* last_argument(const std::vector<const value*>& args) {
    return args.back();
}

const value* no_argument(const std::vector<const value*>& args) {
    return args.empty() ? nullptr : args.front();
}

struct PrimitiveCallerTest : public ::testing::Test {
    PrimitiveCallerTest();
    value zero;
    value one;
    primitive first;
    primitive last;
    NiceMock<MockGetPrimitiveFunction> get_primitive_function;
    primitive_caller<NiceMock<MockGetPrimitiveFunction>> caller{get_primitive_function};

    void SetUp() override {
        ON_CALL(get_primitive_function, function(0)).WillByDefault(ReturnRef(first));
        ON_CALL(get_primitive_function, function(1)).WillByDefault(ReturnRef(last));
    }
};

PrimitiveCallerTest::PrimitiveCallerTest()
    : zero{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , one{.ctor = {.type = 0, .tag = 1}, .args = {&zero}}
    , first(&first_argument)
    , last(&last_argument)
    , get_primitive_function() {}

}

TEST_F(PrimitiveCallerTest, TheArgumentsGoToThePrimitiveAtThatId) {
    EXPECT_EQ(caller.call(0, {&one, &zero}), &one);
    EXPECT_EQ(caller.call(1, {&one, &zero}), &zero);
}

TEST_F(PrimitiveCallerTest, ThePrimitiveIsLookedUpByItsId) {
    EXPECT_CALL(get_primitive_function, function(1)).WillOnce(ReturnRef(last));
    caller.call(1, {&zero});
}

TEST_F(PrimitiveCallerTest, ANullaryCallPassesNoArguments) {
    const primitive nullary(&no_argument);
    ON_CALL(get_primitive_function, function(2)).WillByDefault(ReturnRef(nullary));
    EXPECT_EQ(caller.call(2, {}), nullptr);
}
