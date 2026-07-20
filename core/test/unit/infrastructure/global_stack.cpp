#include <gtest/gtest.h>
#include "infrastructure/global_stack.hpp"
#include "value_objects/lc_expr.hpp"

namespace {

struct GlobalStackTest : public ::testing::Test {
    global_stack stack;
    lc_expr g0{};
    lc_expr g1{};
    lc_expr g2{};
};

} // namespace

TEST_F(GlobalStackTest, StartsEmpty) {
    EXPECT_TRUE(stack.empty());
}

TEST_F(GlobalStackTest, PushPopIsLifo) {
    stack.push(&g0);
    stack.push(&g1);
    stack.push(&g2);

    EXPECT_FALSE(stack.empty());
    EXPECT_EQ(stack.pop(), &g2);
    EXPECT_EQ(stack.pop(), &g1);
    EXPECT_EQ(stack.pop(), &g0);
    EXPECT_TRUE(stack.empty());
}
