#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/integer_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockMakeLcVar  { MOCK_METHOD(const lc_expr*, make_var,       (uint32_t)); };
struct MockMakeLcApp  { MOCK_METHOD(const lc_expr*, make_app,       (const lc_expr*, const lc_expr*)); };
struct MockTranspileNat { MOCK_METHOD(const lc_expr*, transpile_nat, (const aml_expr::nat&)); };
struct MockGetVarIndex  { MOCK_METHOD(uint32_t,       get_var_index, (const std::string&)); };

// Standard bindings: pos=0, negsuc=1
struct IntegerTranspilerTest : public ::testing::Test {
    lc_expr_pool    lc;
    MockMakeLcVar   mock_var;
    MockMakeLcApp   mock_app;
    MockTranspileNat mock_nat;
    MockGetVarIndex  mock_idx;
    integer_transpiler<MockMakeLcVar, MockMakeLcApp, MockTranspileNat, MockGetVarIndex>
        it{mock_var, mock_app, mock_nat, mock_idx};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_idx, get_var_index("pos"))   .WillByDefault(Return(0u));
        ON_CALL(mock_idx, get_var_index("negsuc")).WillByDefault(Return(1u));
        // default: transpile_nat returns var(99) as a sentinel
        ON_CALL(mock_nat, transpile_nat(_)).WillByDefault([this](const aml_expr::nat&) {
            return lc.make_var(99);
        });
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Positive integers: app(pos, transpile_nat(value))
// ---------------------------------------------------------------------------

TEST_F(IntegerTranspilerTest, ZeroUsesPos) {
    // 0 → app(var(0/*pos*/), transpile_nat({0, format}))
    using testing::_;
    const lc_expr* nat_result = lc.make_var(0);
    EXPECT_CALL(mock_nat, transpile_nat(_)).WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({0, nat_format::binary}),
              lc.make_app(lc.make_var(0), nat_result));
}

TEST_F(IntegerTranspilerTest, PositiveUsesPos) {
    using testing::_;
    const lc_expr* nat_result = lc.make_var(5);
    EXPECT_CALL(mock_nat, transpile_nat(_)).WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({12, nat_format::binary}),
              lc.make_app(lc.make_var(0), nat_result));
}

// ---------------------------------------------------------------------------
// Negative integers: app(negsuc, transpile_nat(-value - 1))
// ---------------------------------------------------------------------------

TEST_F(IntegerTranspilerTest, NegativeOneUsesNegsucOfZero) {
    // -1 → negsuc(0)  i.e. app(var(1/*negsuc*/), transpile_nat({0, format}))
    using testing::_;
    const lc_expr* nat_result = lc.make_var(0); // represents transpile_nat({0,...})

    EXPECT_CALL(mock_nat, transpile_nat(testing::Field(&aml_expr::nat::value, 0u)))
        .WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({-1, nat_format::binary}),
              lc.make_app(lc.make_var(1), nat_result));
}

TEST_F(IntegerTranspilerTest, NegativeTwoUsesNegsucOfOne) {
    // -2 → negsuc(1)  i.e. app(var(1/*negsuc*/), transpile_nat({1, format}))
    using testing::_;
    const lc_expr* nat_result = lc.make_var(1);

    EXPECT_CALL(mock_nat, transpile_nat(testing::Field(&aml_expr::nat::value, 1u)))
        .WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({-2, nat_format::binary}),
              lc.make_app(lc.make_var(1), nat_result));
}

// The format field must be forwarded to transpile_nat unchanged.
TEST_F(IntegerTranspilerTest, ForwardsBinaryFormatToTranspileNat) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::AllOf(
            testing::Field(&aml_expr::nat::value, 3u),
            testing::Field(&aml_expr::nat::format, nat_format::binary))))
        .Times(1);
    it.transpile_integer({3, nat_format::binary});
}
