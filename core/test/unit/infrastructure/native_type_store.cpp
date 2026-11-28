#include <functional>
#include <typeindex>
#include <gtest/gtest.h>
#include "infrastructure/native_type_store.hpp"

using namespace AML;

TEST(NativeTypeStoreTest, EachTypeIsFoundByItsCppType) {
    const native_type_store types({native_type::of<int, std::hash<int>>(), native_type::of<bool, std::hash<bool>>()});
    EXPECT_EQ(types.type(typeid(int)).type, std::type_index(typeid(int)));
    EXPECT_EQ(types.type(typeid(bool)).type, std::type_index(typeid(bool)));
    EXPECT_EQ(types.type(typeid(int)).cell_bytes, sizeof(int));
}

TEST(NativeTypeStoreTest, ALaterTypeReplacesAnEarlierOneOfTheSameCppType) {
    native_type replacement = native_type::of<int, std::hash<int>>();
    replacement.cell_bytes = 99;
    const native_type_store types({native_type::of<int, std::hash<int>>(), replacement});
    EXPECT_EQ(types.type(typeid(int)).cell_bytes, 99u);
}
