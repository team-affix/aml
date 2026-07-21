#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <variant>
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/nat_transpiler.hpp"
#include "value_objects/bool_decl_group.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockMakeLcVar  { MOCK_METHOD(const lc_expr*, make_var, (uint32_t)); };
struct MockMakeLcAbs  { MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*)); };
struct MockMakeLcApp  { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };
struct MockGetVarIndex { MOCK_METHOD(uint32_t, get_var_index, (const std::string&)); };

using NiceVar = testing::NiceMock<MockMakeLcVar>;
using NiceAbs = testing::NiceMock<MockMakeLcAbs>;
using NiceApp = testing::NiceMock<MockMakeLcApp>;
using NiceIdx = testing::NiceMock<MockGetVarIndex>;

// Standard test bindings: nil=0, true=1, false=2, cons=3
struct NatTranspilerTest : public ::testing::Test {
    lc_expr_pool lc;
    NiceVar mock_var;
    NiceAbs mock_abs;
    NiceApp mock_app;
    NiceIdx mock_idx;
    nat_transpiler<NiceVar, NiceAbs, NiceApp, NiceIdx>
        nt{mock_var, mock_abs, mock_app, mock_idx};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc.make_abs(b);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_idx, get_var_index(k_nil_name))  .WillByDefault(Return(0u));
        ON_CALL(mock_idx, get_var_index(k_true_name)) .WillByDefault(Return(1u));
        ON_CALL(mock_idx, get_var_index(k_false_name)).WillByDefault(Return(2u));
        ON_CALL(mock_idx, get_var_index(k_cons_name)) .WillByDefault(Return(3u));
    }

    const lc_expr* v(uint32_t i)                          { return lc.make_var(i); }
    const lc_expr* ab(const lc_expr* b)                   { return lc.make_abs(b); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
    const lc_expr* cons_(const lc_expr* hd, const lc_expr* tl) {
        return ap(ap(v(3), hd), tl);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Binary
// ---------------------------------------------------------------------------

TEST_F(NatTranspilerTest, BinaryZeroReturnsNil) {
    EXPECT_EQ(nt.transpile_nat({0u, nat_format::binary}), v(0));
}

TEST_F(NatTranspilerTest, BinaryOneIsConsTrueNil) {
    EXPECT_EQ(nt.transpile_nat({1u, nat_format::binary}),
              cons_(v(1), v(0)));
}

TEST_F(NatTranspilerTest, BinaryTwoIsConsFalseConsTrueNil) {
    EXPECT_EQ(nt.transpile_nat({2u, nat_format::binary}),
              cons_(v(2), cons_(v(1), v(0))));
}

TEST_F(NatTranspilerTest, BinaryThreeIsConsTrueConsTrueNil) {
    EXPECT_EQ(nt.transpile_nat({3u, nat_format::binary}),
              cons_(v(1), cons_(v(1), v(0))));
}

TEST_F(NatTranspilerTest, BinaryFourIsConsFalseConsFalseConsTrueNil) {
    EXPECT_EQ(nt.transpile_nat({4u, nat_format::binary}),
              cons_(v(2), cons_(v(2), cons_(v(1), v(0)))));
}

TEST_F(NatTranspilerTest, BinaryFiveIsConsTrueConsFalseConsTrueNil) {
    EXPECT_EQ(nt.transpile_nat({5u, nat_format::binary}),
              cons_(v(1), cons_(v(2), cons_(v(1), v(0)))));
}

TEST_F(NatTranspilerTest, BinaryZeroCallsMakeVarOnce) {
    EXPECT_CALL(mock_var, make_var(0u)).Times(1);
    nt.transpile_nat({0u, nat_format::binary});
}

TEST_F(NatTranspilerTest, BinaryEightHasFourBitsLsbFirst) {
    EXPECT_EQ(nt.transpile_nat({8u, nat_format::binary}),
              cons_(v(2), cons_(v(2), cons_(v(2), cons_(v(1), v(0))))));
}

// ---------------------------------------------------------------------------
// Church
// ---------------------------------------------------------------------------

TEST_F(NatTranspilerTest, ChurchZeroIsLambdaFLambdaXX) {
    EXPECT_EQ(nt.transpile_nat({0u, nat_format::church}), ab(ab(v(0))));
}

TEST_F(NatTranspilerTest, ChurchOneIsLambdaFLambdaXFX) {
    EXPECT_EQ(nt.transpile_nat({1u, nat_format::church}),
              ab(ab(ap(v(1), v(0)))));
}

TEST_F(NatTranspilerTest, ChurchTwoIsLambdaFLambdaXFFX) {
    EXPECT_EQ(nt.transpile_nat({2u, nat_format::church}),
              ab(ab(ap(v(1), ap(v(1), v(0))))));
}

TEST_F(NatTranspilerTest, ChurchThreeAppliesFThreeTimes) {
    EXPECT_EQ(nt.transpile_nat({3u, nat_format::church}),
              ab(ab(ap(v(1), ap(v(1), ap(v(1), v(0)))))));
}

TEST_F(NatTranspilerTest, ChurchAlwaysWrapsWithTwoAbstractions) {
    const lc_expr* c4 = nt.transpile_nat({4u, nat_format::church});
    const auto* outer = std::get_if<lc_expr::abs>(&c4->content);
    ASSERT_NE(outer, nullptr);
    const auto* inner = std::get_if<lc_expr::abs>(&outer->body->content);
    EXPECT_NE(inner, nullptr);
}
