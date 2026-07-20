#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "value_objects/list_format.hpp"
#include "value_objects/nat_format.hpp"

struct AmlExprPoolTest : public ::testing::Test {
    aml_expr_pool pool;
};

// ---------------------------------------------------------------------------
// make_symbol
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, TokenInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_symbol("foo"), pool.make_symbol("foo"));
}

TEST_F(AmlExprPoolTest, DifferentTokenNamesReturnDifferentPointers) {
    EXPECT_NE(pool.make_symbol("foo"), pool.make_symbol("bar"));
}

TEST_F(AmlExprPoolTest, EmptyTokenName) {
    EXPECT_EQ(pool.make_symbol(""), pool.make_symbol(""));
}

// ---------------------------------------------------------------------------
// make_nat
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, NatInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_nat(42u, nat_format::binary), pool.make_nat(42u, nat_format::binary));
}

TEST_F(AmlExprPoolTest, DifferentNatValuesReturnDifferentPointers) {
    EXPECT_NE(pool.make_nat(1u, nat_format::binary), pool.make_nat(2u, nat_format::binary));
}

TEST_F(AmlExprPoolTest, SameNatValueDifferentFormatReturnsDifferentPointers) {
    EXPECT_NE(pool.make_nat(5u, nat_format::binary), pool.make_nat(5u, nat_format::church));
}

TEST_F(AmlExprPoolTest, NatZeroInterned) {
    EXPECT_EQ(pool.make_nat(0u, nat_format::binary), pool.make_nat(0u, nat_format::binary));
}

// ---------------------------------------------------------------------------
// make_integer
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, IntegerInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_integer(-5, nat_format::binary), pool.make_integer(-5, nat_format::binary));
}

TEST_F(AmlExprPoolTest, IntegerDifferentValuesReturnDifferentPointers) {
    EXPECT_NE(pool.make_integer(5, nat_format::binary), pool.make_integer(6, nat_format::binary));
}

TEST_F(AmlExprPoolTest, IntegerOppositeSignsReturnDifferentPointers) {
    EXPECT_NE(pool.make_integer(5, nat_format::binary), pool.make_integer(-5, nat_format::binary));
}

TEST_F(AmlExprPoolTest, IntegerZeroInterned) {
    EXPECT_EQ(pool.make_integer(0, nat_format::binary), pool.make_integer(0, nat_format::binary));
}

// ---------------------------------------------------------------------------
// make_character
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, CharacterInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_character('a'), pool.make_character('a'));
}

TEST_F(AmlExprPoolTest, DifferentCharactersReturnDifferentPointers) {
    EXPECT_NE(pool.make_character('a'), pool.make_character('b'));
}

TEST_F(AmlExprPoolTest, NullCharacterInterned) {
    EXPECT_EQ(pool.make_character('\0'), pool.make_character('\0'));
}

// ---------------------------------------------------------------------------
// make_string
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, StringInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_string("hello"), pool.make_string("hello"));
}

TEST_F(AmlExprPoolTest, DifferentStringsReturnDifferentPointers) {
    EXPECT_NE(pool.make_string("hello"), pool.make_string("world"));
}

TEST_F(AmlExprPoolTest, EmptyStringInterned) {
    EXPECT_EQ(pool.make_string(""), pool.make_string(""));
}

TEST_F(AmlExprPoolTest, EmptyStringDiffersFromNonempty) {
    EXPECT_NE(pool.make_string(""), pool.make_string("x"));
}

// ---------------------------------------------------------------------------
// make_abs
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, AbsInternedTwiceReturnsSamePointer) {
    const aml_expr* body = pool.make_symbol("x");
    EXPECT_EQ(pool.make_abs("x", body), pool.make_abs("x", body));
}

TEST_F(AmlExprPoolTest, AbsDifferentParamReturnsDifferentPointers) {
    const aml_expr* body = pool.make_symbol("x");
    EXPECT_NE(pool.make_abs("x", body), pool.make_abs("y", body));
}

TEST_F(AmlExprPoolTest, AbsDifferentBodyReturnsDifferentPointers) {
    const aml_expr* bx = pool.make_symbol("x");
    const aml_expr* by = pool.make_symbol("y");
    EXPECT_NE(pool.make_abs("p", bx), pool.make_abs("p", by));
}

// ---------------------------------------------------------------------------
// make_app
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, AppInternedTwiceReturnsSamePointer) {
    const aml_expr* f = pool.make_symbol("f");
    const aml_expr* x = pool.make_symbol("x");
    EXPECT_EQ(pool.make_app(f, x), pool.make_app(f, x));
}

TEST_F(AmlExprPoolTest, AppDifferentFuncReturnsDifferentPointers) {
    const aml_expr* f = pool.make_symbol("f");
    const aml_expr* g = pool.make_symbol("g");
    const aml_expr* x = pool.make_symbol("x");
    EXPECT_NE(pool.make_app(f, x), pool.make_app(g, x));
}

TEST_F(AmlExprPoolTest, AppDifferentArgReturnsDifferentPointers) {
    const aml_expr* f = pool.make_symbol("f");
    const aml_expr* x = pool.make_symbol("x");
    const aml_expr* y = pool.make_symbol("y");
    EXPECT_NE(pool.make_app(f, x), pool.make_app(f, y));
}

TEST_F(AmlExprPoolTest, AppSwappedFuncArgReturnsDifferentPointers) {
    const aml_expr* a = pool.make_symbol("a");
    const aml_expr* b = pool.make_symbol("b");
    EXPECT_NE(pool.make_app(a, b), pool.make_app(b, a));
}

// ---------------------------------------------------------------------------
// make_list
// ---------------------------------------------------------------------------

TEST_F(AmlExprPoolTest, ListInternedTwiceReturnsSamePointer) {
    const aml_expr* a = pool.make_symbol("a");
    EXPECT_EQ(pool.make_list({a}, list_format::scott),
              pool.make_list({a}, list_format::scott));
}

TEST_F(AmlExprPoolTest, SameListElementsDifferentFormatReturnsDifferentPointers) {
    const aml_expr* a = pool.make_symbol("a");
    EXPECT_NE(pool.make_list({a}, list_format::scott),
              pool.make_list({a}, list_format::church));
}

TEST_F(AmlExprPoolTest, ListDifferentLengthsReturnDifferentPointers) {
    const aml_expr* a = pool.make_symbol("a");
    EXPECT_NE(pool.make_list({a}, list_format::scott),
              pool.make_list({a, a}, list_format::scott));
}

TEST_F(AmlExprPoolTest, EmptyListInterned) {
    EXPECT_EQ(pool.make_list({}, list_format::scott),
              pool.make_list({}, list_format::scott));
}

TEST_F(AmlExprPoolTest, ListDifferentElementOrderReturnsDifferentPointers) {
    const aml_expr* a = pool.make_symbol("a");
    const aml_expr* b = pool.make_symbol("b");
    EXPECT_NE(pool.make_list({a, b}, list_format::scott),
              pool.make_list({b, a}, list_format::scott));
}
