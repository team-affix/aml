#include <functional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/argument_type_index.hpp"

using namespace AML;

namespace {

// 0 takes (int, int); 1 takes bool; 2 takes (bool, int).
struct ArgumentTypeIndexTest : public ::testing::Test {
    ArgumentTypeIndexTest();
    argument_type_index index;
};

ArgumentTypeIndexTest::ArgumentTypeIndexTest()
    : index({native_primitive::of(std::function<int(int, int)>(std::plus<int>())),
             native_primitive::of(std::function<bool(bool)>(std::logical_not<bool>())),
             native_primitive::of(std::function<bool(bool, int)>(std::equal_to<int>()))}) {}

}

TEST_F(ArgumentTypeIndexTest, ATypeListsEveryPrimitiveTakingItInIncreasingId) {
    EXPECT_EQ(index.of_type(typeid(int)), (std::vector<prim_id>{0, 2}));
    EXPECT_EQ(index.of_type(typeid(bool)), (std::vector<prim_id>{1, 2}));
}

TEST_F(ArgumentTypeIndexTest, APrimitiveTakingATypeTwiceIsListedOnce) {
    EXPECT_EQ(index.of_type(typeid(int)).front(), 0u);
    EXPECT_EQ(index.of_type(typeid(int)).size(), 2u);
}

TEST_F(ArgumentTypeIndexTest, ATypeNoPrimitiveTakesListsNone) {
    EXPECT_TRUE(index.of_type(typeid(double)).empty());
}
