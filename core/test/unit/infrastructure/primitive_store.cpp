#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/primitive_store.hpp"

namespace {

// The store is generic over what a primitive is; plain ints stand in for primitives here.
struct PrimitiveStoreTest : public ::testing::Test {
    PrimitiveStoreTest();
    primitive_store<int> store;
};

PrimitiveStoreTest::PrimitiveStoreTest()
    : store(std::vector<int>{40, 41, 42}) {}

}

TEST_F(PrimitiveStoreTest, CountsThePrimitives) {
    EXPECT_EQ(store.count(), 3u);
}

TEST_F(PrimitiveStoreTest, APrimitivesIdIsItsPosition) {
    EXPECT_EQ(store.primitive_at(0), 40);
    EXPECT_EQ(store.primitive_at(2), 42);
}

TEST_F(PrimitiveStoreTest, NoPrimitivesCountZero) {
    const primitive_store<int> empty(std::vector<int>{});
    EXPECT_EQ(empty.count(), 0u);
}

TEST_F(PrimitiveStoreTest, ThePrimitivesAreHeldNotReferredTo) {
    std::vector<int> given{7};
    const primitive_store<int> held(given);
    given[0] = 8;
    EXPECT_EQ(held.primitive_at(0), 7);
}
