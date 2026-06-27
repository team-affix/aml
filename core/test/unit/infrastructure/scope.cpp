#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/scope.hpp"

struct ScopeTest : public ::testing::Test {
    scope sc;
};

TEST_F(ScopeTest, PushAndLookupSingle) {
    sc.push("x");
    EXPECT_EQ(sc.get_var_index("x"), 0u);
}

TEST_F(ScopeTest, TwoVarsInnerIsZero) {
    sc.push("x");
    sc.push("y");
    EXPECT_EQ(sc.get_var_index("y"), 0u);
    EXPECT_EQ(sc.get_var_index("x"), 1u);
}

TEST_F(ScopeTest, ThreeVarsIndices) {
    sc.push("a");
    sc.push("b");
    sc.push("c");
    EXPECT_EQ(sc.get_var_index("c"), 0u);
    EXPECT_EQ(sc.get_var_index("b"), 1u);
    EXPECT_EQ(sc.get_var_index("a"), 2u);
}

TEST_F(ScopeTest, PopRestoresPrevious) {
    sc.push("x");
    sc.push("y");
    sc.pop();
    EXPECT_EQ(sc.get_var_index("x"), 0u);
}

TEST_F(ScopeTest, PopRemovesTopName) {
    sc.push("x");
    sc.push("y");
    sc.pop();
    EXPECT_THROW(sc.get_var_index("y"), std::out_of_range);
}

TEST_F(ScopeTest, ShadowingInnerSeesZero) {
    sc.push("x");
    sc.push("x");
    EXPECT_EQ(sc.get_var_index("x"), 0u);
}

TEST_F(ScopeTest, ShadowingOuterSeenFromInsideIsOne) {
    sc.push("x");
    sc.push("y");
    sc.push("x");
    EXPECT_EQ(sc.get_var_index("x"), 0u);
    EXPECT_EQ(sc.get_var_index("y"), 1u);
}

TEST_F(ScopeTest, ShadowingPopRestoresOuter) {
    sc.push("x");
    sc.push("x");
    sc.pop();
    EXPECT_EQ(sc.get_var_index("x"), 0u);
}

TEST_F(ScopeTest, ShadowingFullyPopped) {
    sc.push("x");
    sc.push("x");
    sc.pop();
    sc.pop();
    EXPECT_THROW(sc.get_var_index("x"), std::out_of_range);
}

TEST_F(ScopeTest, UnboundNameThrows) {
    EXPECT_THROW(sc.get_var_index("missing"), std::out_of_range);
}

TEST_F(ScopeTest, PoppedNameThrows) {
    sc.push("x");
    sc.pop();
    EXPECT_THROW(sc.get_var_index("x"), std::out_of_range);
}

TEST_F(ScopeTest, GlobalsThenLocalsShiftIndices) {
    sc.push("g0");
    sc.push("g1");
    EXPECT_EQ(sc.get_var_index("g0"), 1u);
    EXPECT_EQ(sc.get_var_index("g1"), 0u);
    sc.push("x");
    EXPECT_EQ(sc.get_var_index("x"),  0u);
    EXPECT_EQ(sc.get_var_index("g1"), 1u);
    EXPECT_EQ(sc.get_var_index("g0"), 2u);
    sc.pop();
    sc.pop();
    sc.pop();
}
