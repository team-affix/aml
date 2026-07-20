#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/lc_nullptr_transpiler.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/expr.hpp"

namespace {

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};

using NiceVar = testing::NiceMock<MockMakeVar>;

struct LcNullptrTranspilerTest : public ::testing::Test {
    NiceVar mock_var;
    lc_nullptr_transpiler<NiceVar> tx{mock_var};
    expr hole{};
};

} // namespace

TEST_F(LcNullptrTranspilerTest, MakesMainFunctionVar) {
    using testing::Return;

    EXPECT_CALL(mock_var, make_var(k_main_function_var_id)).WillOnce(Return(&hole));
    EXPECT_EQ(tx.transpile_nullptr(), &hole);
}
