#include <functional>
#include <typeindex>
#include <gtest/gtest.h>
#include "infrastructure/native_primitive_store.hpp"

using namespace AML;

TEST(NativePrimitiveStoreTest, PrimitivesAreNumberedByPosition) {
    const native_primitive_store primitives({
        native_primitive::of(std::function<int(int, int)>(std::plus<int>())),
        native_primitive::of(std::function<bool(bool)>(std::logical_not<bool>()))});
    EXPECT_EQ(primitives.count(), 2u);
    EXPECT_EQ(primitives.primitive(1).result_type, std::type_index(typeid(bool)));
    EXPECT_EQ(primitives.primitive(0).arg_types.size(), 2u);
}

TEST(NativePrimitiveStoreTest, NoPrimitivesCountNone) {
    const native_primitive_store primitives({});
    EXPECT_EQ(primitives.count(), 0u);
}
