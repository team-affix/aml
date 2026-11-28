#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/cached_column_maker.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

struct CachedColumnMakerTest : public ::testing::Test {
    CachedColumnMakerTest();
    native_type integer;
    NiceMock<MockGetNativeType> get_native_type;
    cached_column_maker<NiceMock<MockGetNativeType>> maker{get_native_type};

    void SetUp() override {
        ON_CALL(get_native_type, type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(integer));
    }
};

CachedColumnMakerTest::CachedColumnMakerTest()
    : integer(native_type::of<int, std::hash<int>>())
    , get_native_type() {}

}

TEST_F(CachedColumnMakerTest, TheColumnKeepsItsTypeAndCells) {
    const std::shared_ptr<const cached_column> made = maker.make(typeid(int), std::any(std::vector<int>{3, 1, 3}));
    EXPECT_EQ(made->type, std::type_index(typeid(int)));
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(made->cells), (std::vector<int>{3, 1, 3}));
}

TEST_F(CachedColumnMakerTest, EachRowIsClassedByItsType) {
    EXPECT_EQ(maker.make(typeid(int), std::any(std::vector<int>{3, 1, 3}))->classes, (std::vector<size_t>{0, 1, 0}));
}

TEST_F(CachedColumnMakerTest, TheColumnIsHashedByItsType) {
    const std::any cells(std::vector<int>{3, 1, 3});
    EXPECT_EQ(maker.make(typeid(int), cells)->hash, integer.hash(cells));
}

TEST_F(CachedColumnMakerTest, TheTypesOperationsAreLookedUpByItsType) {
    EXPECT_CALL(get_native_type, type(std::type_index(typeid(int)))).WillOnce(ReturnRef(integer));
    maker.make(typeid(int), std::any(std::vector<int>{}));
}
