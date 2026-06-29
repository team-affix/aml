#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/string_transpiler.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockTranspileNat { MOCK_METHOD(const lc_expr*, transpile_nat, (const aml_expr::nat&)); };
struct MockMakeLcVar    { MOCK_METHOD(const lc_expr*, make_var,       (uint32_t)); };
struct MockMakeLcApp    { MOCK_METHOD(const lc_expr*, make_app,       (const lc_expr*, const lc_expr*)); };
struct MockGetVarIndex  { MOCK_METHOD(uint32_t,       get_var_index,  (const std::string&)); };

// Standard bindings: nil=0, cons=1
struct StringTranspilerTest : public ::testing::Test {
    lc_expr_pool     lc;
    MockTranspileNat mock_nat;
    MockMakeLcVar    mock_var;
    MockMakeLcApp    mock_app;
    MockGetVarIndex  mock_idx;
    string_transpiler<MockTranspileNat, MockMakeLcVar, MockMakeLcApp, MockGetVarIndex>
        st{mock_nat, mock_var, mock_app, mock_idx};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_nat, transpile_nat(_)).WillByDefault([this](const aml_expr::nat& n) {
            return lc.make_var(static_cast<uint32_t>(n.value));
        });
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_idx, get_var_index("nil")) .WillByDefault(Return(0u));
        ON_CALL(mock_idx, get_var_index("cons")).WillByDefault(Return(1u));
    }

    const lc_expr* v(uint32_t i)                           { return lc.make_var(i); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
    const lc_expr* cons_(const lc_expr* hd, const lc_expr* tl) {
        return ap(ap(v(1), hd), tl);
    }
};

} // namespace

TEST_F(StringTranspilerTest, EmptyStringReturnsNil) {
    EXPECT_EQ(st.transpile_string(aml_expr::string{""}), v(0));
}

TEST_F(StringTranspilerTest, SingleCharIsConsByteNil) {
    // "A" → cons(65, nil)
    EXPECT_EQ(st.transpile_string(aml_expr::string{"A"}),
              cons_(v(65), v(0)));
}

TEST_F(StringTranspilerTest, TwoCharactersInOrder) {
    // "ab" → cons('a', cons('b', nil))  ('a'=97, 'b'=98)
    EXPECT_EQ(st.transpile_string(aml_expr::string{"ab"}),
              cons_(v(97), cons_(v(98), v(0))));
}

TEST_F(StringTranspilerTest, OrderIsPreservedNotReversed) {
    // "hi" must produce h-head, i-tail (not reversed)
    // 'h'=104, 'i'=105
    const lc_expr* result = st.transpile_string(aml_expr::string{"hi"});
    const auto* outer_app = std::get_if<lc_expr::app>(&result->content);
    ASSERT_NE(outer_app, nullptr);
    // outer_app is app(app(cons, head_char), tail)
    const auto* inner_app = std::get_if<lc_expr::app>(&outer_app->func->content);
    ASSERT_NE(inner_app, nullptr);
    // inner_app->arg is the head character: should be var(104) = 'h', not 'i'
    EXPECT_EQ(inner_app->arg, v(104));
}

TEST_F(StringTranspilerTest, EachCharTranspilesThroughTranspileNatWithBinaryFormat) {
    // Every character must reach transpile_nat with nat_format::binary
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::Field(&aml_expr::nat::format, nat_format::binary)))
        .Times(3);
    st.transpile_string(aml_expr::string{"xyz"});
}
