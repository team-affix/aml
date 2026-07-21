#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/symbol_transpiler.hpp"
#include "value_objects/aml_expr.hpp"

namespace {

struct MockMakeLcVar            { MOCK_METHOD(const lc_expr*, make_var, (uint32_t)); };
struct MockContainsName         { MOCK_METHOD(bool, contains, (const std::string&), (const)); };
struct MockGetVarIndex          { MOCK_METHOD(uint32_t, get_var_index, (const std::string&)); };
struct MockTryTranspileBuiltin  {
    MOCK_METHOD(const lc_expr*, try_transpile_name, (const std::string&));
};

struct SymbolTranspilerTest : public ::testing::Test {
    lc_expr_pool lc;
    testing::NiceMock<MockMakeLcVar>           mock_var;
    testing::NiceMock<MockContainsName>        mock_contains;
    testing::NiceMock<MockGetVarIndex>         mock_idx;
    testing::NiceMock<MockTryTranspileBuiltin> mock_builtin;
    symbol_transpiler<testing::NiceMock<MockMakeLcVar>,
                      testing::NiceMock<MockContainsName>,
                      testing::NiceMock<MockGetVarIndex>,
                      testing::NiceMock<MockTryTranspileBuiltin>>
        tt{mock_var, mock_contains, mock_idx, mock_builtin};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_contains, contains(_)).WillByDefault(Return(false));
        ON_CALL(mock_builtin, try_transpile_name(_)).WillByDefault(Return(nullptr));
    }
};

} // namespace

TEST_F(SymbolTranspilerTest, LooksUpNameAndForwardsIndexToMakeVar) {
    using testing::Return;
    ON_CALL(mock_contains, contains("foo")).WillByDefault(Return(true));
    ON_CALL(mock_idx, get_var_index("foo")).WillByDefault(Return(3u));

    EXPECT_CALL(mock_var, make_var(3u)).Times(1);
    tt.transpile_symbol(aml_expr::symbol{"foo"});
}

TEST_F(SymbolTranspilerTest, ReturnsResultOfMakeVar) {
    using testing::Return;
    ON_CALL(mock_contains, contains("foo")).WillByDefault(Return(true));
    ON_CALL(mock_idx, get_var_index("foo")).WillByDefault(Return(3u));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"foo"}), lc.make_var(3));
}

TEST_F(SymbolTranspilerTest, DifferentNamesProduceDifferentIndices) {
    using testing::Return;
    ON_CALL(mock_contains, contains("x")).WillByDefault(Return(true));
    ON_CALL(mock_contains, contains("y")).WillByDefault(Return(true));
    ON_CALL(mock_idx, get_var_index("x")).WillByDefault(Return(0u));
    ON_CALL(mock_idx, get_var_index("y")).WillByDefault(Return(1u));

    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"x"}), lc.make_var(0));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"y"}), lc.make_var(1));
}

TEST_F(SymbolTranspilerTest, IndexZeroProducesVar0) {
    using testing::Return;
    ON_CALL(mock_contains, contains("inner")).WillByDefault(Return(true));
    ON_CALL(mock_idx, get_var_index("inner")).WillByDefault(Return(0u));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"inner"}), lc.make_var(0));
}

TEST_F(SymbolTranspilerTest, FallsBackToBuiltinWhenNotInScope) {
    using testing::Return;
    const lc_expr* builtin = lc.make_var(42);
    ON_CALL(mock_contains, contains("true")).WillByDefault(Return(false));
    ON_CALL(mock_builtin, try_transpile_name("true")).WillByDefault(Return(builtin));
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"true"}), builtin);
}

TEST_F(SymbolTranspilerTest, ScopeWinsOverBuiltin) {
    using testing::Return;
    ON_CALL(mock_contains, contains("true")).WillByDefault(Return(true));
    ON_CALL(mock_idx, get_var_index("true")).WillByDefault(Return(7u));
    EXPECT_CALL(mock_builtin, try_transpile_name).Times(0);
    EXPECT_EQ(tt.transpile_symbol(aml_expr::symbol{"true"}), lc.make_var(7));
}

TEST_F(SymbolTranspilerTest, UnboundNonBuiltinThrows) {
    EXPECT_THROW(tt.transpile_symbol(aml_expr::symbol{"missing"}), std::out_of_range);
}
