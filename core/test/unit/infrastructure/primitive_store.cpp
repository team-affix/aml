#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/primitive_store.hpp"

using namespace AML;

namespace {

value first_argument(const std::vector<const value*>& args) {
    return *args.front();
}

value last_argument(const std::vector<const value*>& args) {
    return *args.back();
}

struct PrimitiveStoreTest : public ::testing::Test {
    PrimitiveStoreTest();
    value zero;
    value one;
    primitive_type unary;
    primitive_type binary;
    primitive_store store;
};

PrimitiveStoreTest::PrimitiveStoreTest()
    : zero{.ctor = 0, .args = {}}
    , one{.ctor = 1, .args = {zero}}
    , unary{.arg_types = {0}, .result_type = 0}
    , binary{.arg_types = {0, 1}, .result_type = 2}
    , store(std::vector<typed_primitive>{
          typed_primitive{.type = unary, .apply = &first_argument},
          typed_primitive{.type = binary, .apply = &last_argument}}) {}

}

TEST_F(PrimitiveStoreTest, CountsThePrimitives) {
    EXPECT_EQ(store.count(), 2u);
}

TEST_F(PrimitiveStoreTest, APrimitivesIdIsItsPosition) {
    EXPECT_EQ(store.type(0), unary);
    EXPECT_EQ(store.type(1), binary);
}

TEST_F(PrimitiveStoreTest, AStoredFunctionIsTheOneGivenAtThatId) {
    EXPECT_EQ(store.function(0)({&zero, &one}), zero);
    EXPECT_EQ(store.function(1)({&zero, &one}), one);
}

TEST_F(PrimitiveStoreTest, NoPrimitivesCountZero) {
    const primitive_store empty(std::vector<typed_primitive>{});
    EXPECT_EQ(empty.count(), 0u);
}

TEST_F(PrimitiveStoreTest, ThePrimitivesAreHeldNotReferredTo) {
    std::vector<typed_primitive> given{typed_primitive{.type = unary, .apply = &first_argument}};
    const primitive_store held(given);
    given[0].type = binary;
    EXPECT_EQ(held.type(0), unary);
}
