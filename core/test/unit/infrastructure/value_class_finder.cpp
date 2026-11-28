#include <any>
#include <functional>
#include <optional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/value_class_finder.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

// The column holds 8, 3, 8, 6 with classes 0, 1, 0, 2.
struct ValueClassFinderTest : public ::testing::Test {
    ValueClassFinderTest();
    native_type integer;
    cached_column column;
    NiceMock<MockGetNativeType> get_native_type;
    value_class_finder<NiceMock<MockGetNativeType>> finder{get_native_type};

    void SetUp() override {
        ON_CALL(get_native_type, type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(integer));
    }
};

ValueClassFinderTest::ValueClassFinderTest()
    : integer(native_type::of<int, std::hash<int>>())
    , column{.type = typeid(int), .cells = std::any(std::vector<int>{8, 3, 8, 6}), .classes = {0, 1, 0, 2}, .hash = 0}
    , get_native_type() {}

}

TEST_F(ValueClassFinderTest, AHeldValueTakesItsRowsClass) {
    EXPECT_EQ(finder.find(column, std::any(std::vector<int>{6})), std::optional<size_t>(2));
    EXPECT_EQ(finder.find(column, std::any(std::vector<int>{8})), std::optional<size_t>(0));
}

TEST_F(ValueClassFinderTest, AValueNoRowHoldsHasNoClass) {
    EXPECT_EQ(finder.find(column, std::any(std::vector<int>{4})), std::nullopt);
}

TEST_F(ValueClassFinderTest, AnEmptyColumnHoldsNoValue) {
    const cached_column empty{.type = typeid(int), .cells = std::any(std::vector<int>{}), .classes = {}, .hash = 0};
    EXPECT_EQ(finder.find(empty, std::any(std::vector<int>{4})), std::nullopt);
}
