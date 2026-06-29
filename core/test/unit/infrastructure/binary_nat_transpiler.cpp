#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/binary_nat_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockMakeLcVar  { MOCK_METHOD(const lc_expr*, make_var, (uint32_t)); };
struct MockMakeLcApp  { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };
struct MockGetVarIndex { MOCK_METHOD(uint32_t, get_var_index, (const std::string&)); };

// Standard test bindings: nil=0, true=1, false=2, cons=3
struct BinaryNatTranspilerTest : public ::testing::Test {
    lc_expr_pool    lc;
    MockMakeLcVar   mock_var;
    MockMakeLcApp   mock_app;
    MockGetVarIndex mock_idx;
    binary_nat_transpiler<MockMakeLcVar, MockMakeLcApp, MockGetVarIndex>
        bnt{mock_var, mock_app, mock_idx};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_idx, get_var_index("nil"))  .WillByDefault(Return(0u));
        ON_CALL(mock_idx, get_var_index("true")) .WillByDefault(Return(1u));
        ON_CALL(mock_idx, get_var_index("false")).WillByDefault(Return(2u));
        ON_CALL(mock_idx, get_var_index("cons")) .WillByDefault(Return(3u));
    }

    const lc_expr* v(uint32_t i)                           { return lc.make_var(i); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
    const lc_expr* cons_(const lc_expr* hd, const lc_expr* tl) {
        return ap(ap(v(3), hd), tl);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Zero
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, ZeroReturnsNil) {
    EXPECT_EQ(bnt.transpile_nat({0u, nat_format::binary}), v(0));
}

// ---------------------------------------------------------------------------
// One — single bit [true]
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, OneIsConsTrueNil) {
    // 1 = 0b1 → [true]
    EXPECT_EQ(bnt.transpile_nat({1u, nat_format::binary}),
              cons_(v(1), v(0)));
}

// ---------------------------------------------------------------------------
// Two — 0b10: bits in LSB-first order → [bit0=false, bit1=true]
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, TwoIsConsFalseConsTrueNil) {
    // 2 = 0b10 → LSB-first list: [false, true] = cons(false, cons(true, nil))
    EXPECT_EQ(bnt.transpile_nat({2u, nat_format::binary}),
              cons_(v(2), cons_(v(1), v(0))));
}

// ---------------------------------------------------------------------------
// Three — 0b11: all bits set → [true, true]
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, ThreeIsConsTrueConsTrueNil) {
    // 3 = 0b11 → LSB-first: [true, true]
    EXPECT_EQ(bnt.transpile_nat({3u, nat_format::binary}),
              cons_(v(1), cons_(v(1), v(0))));
}

// ---------------------------------------------------------------------------
// Four — 0b100: only bit2 set → [false, false, true]
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, FourIsConsFalseConsFalseConsTrueNil) {
    // 4 = 0b100 → LSB-first: [bit0=false, bit1=false, bit2=true]
    EXPECT_EQ(bnt.transpile_nat({4u, nat_format::binary}),
              cons_(v(2), cons_(v(2), cons_(v(1), v(0)))));
}

// ---------------------------------------------------------------------------
// Five — 0b101: bits 0 and 2 set → [true, false, true]
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, FiveIsConsTrueConsFalseConsTrueNil) {
    // 5 = 0b101 → LSB-first: [bit0=true, bit1=false, bit2=true]
    EXPECT_EQ(bnt.transpile_nat({5u, nat_format::binary}),
              cons_(v(1), cons_(v(2), cons_(v(1), v(0)))));
}

// ---------------------------------------------------------------------------
// make_var is called with nil index for zero (not cons)
// ---------------------------------------------------------------------------

TEST_F(BinaryNatTranspilerTest, ZeroCallsMakeVarOnce) {
    EXPECT_CALL(mock_var, make_var(0u)).Times(1);
    bnt.transpile_nat({0u, nat_format::binary});
}
