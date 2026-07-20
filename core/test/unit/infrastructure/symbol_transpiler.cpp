#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/symbol_transpiler.hpp"

namespace {

struct MockMakeLcVar  { MOCK_METHOD(const lc_expr*, make_var,       (uint32_t)); };
struct MockGetVarIndex { MOCK_METHOD(uint32_t,       get_var_index, (const std::string&)); };

struct SymbolTranspilerTest : public ::testing::Test {
    lc_expr_pool                                                     lc;
    testing::NiceMock<MockMakeLcVar>                                 mock_var;
    testing::NiceMock<MockGetVarIndex>                               mock_idx;
    symbol_transpiler<testing::NiceMock<MockMakeLcVar>,
                     testing::NiceMock<MockGetVarIndex>>             tt{mock_var, mock_idx};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
    }
};

} // namespace

// The observable contract: transpile_symbol looks up the name to obtain an
// index, then forwards that index to make_var and returns the result.

TEST_F(SymbolTranspilerTest, LooksUpNameAndForwardsIndexToMakeVar) {
    using testing::Return;
    ON_CALL(mock_idx, get_var_index("foo")).WillByDefault(Return(3u));

    EXPECT_CALL(mock_var, make_var(3u)).Times(1);
    tt.transpile_symbol(aml_expr::symbol{"foo"});
}

TEST_F(SymbolTranspilerTest, ReturnsResultOfMakeVar) {
    using testing::Return;
    ON_CALL(mock_idx, get_var_index("foo")).WillByDefault(Return(3u));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"foo"}), lc.make_var(3));
}

TEST_F(SymbolTranspilerTest, DifferentNamesProduceDifferentIndices) {
    using testing::Return;
    ON_CALL(mock_idx, get_var_index("x")).WillByDefault(Return(0u));
    ON_CALL(mock_idx, get_var_index("y")).WillByDefault(Return(1u));

    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"x"}), lc.make_var(0));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"y"}), lc.make_var(1));
}

TEST_F(SymbolTranspilerTest, IndexZeroProducesVar0) {
    using testing::Return;
    ON_CALL(mock_idx, get_var_index("inner")).WillByDefault(Return(0u));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"inner"}), lc.make_var(0));
}
