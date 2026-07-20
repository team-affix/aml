#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/lc_var_transpiler.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

namespace {

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor,
                (uint32_t, (const std::vector<const expr*>&)), ());
};

using NiceFun = testing::NiceMock<MockMakeFunctor>;

struct LcVarTranspilerTest : public ::testing::Test {
    NiceFun mock_fun;
    lc_var_transpiler<NiceFun> tx{mock_fun};
    expr zero{};
    expr suc1{};
    expr suc2{};
    expr var_term{};
};

} // namespace

TEST_F(LcVarTranspilerTest, IndexZeroIsVarOfZero) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(mock_fun, make_functor(k_zero_functor_id,
            std::vector<const expr*>{}))
            .WillOnce(Return(&zero));
        EXPECT_CALL(mock_fun, make_functor(k_var_functor_id,
            std::vector<const expr*>{&zero}))
            .WillOnce(Return(&var_term));
    }
    EXPECT_EQ(tx.transpile_var(lc_expr::var{0}), &var_term);
}

TEST_F(LcVarTranspilerTest, IndexTwoIsVarOfSucSucZero) {
    using testing::Return;
    using testing::InSequence;

    {
        InSequence seq;
        EXPECT_CALL(mock_fun, make_functor(k_zero_functor_id,
            std::vector<const expr*>{}))
            .WillOnce(Return(&zero));
        EXPECT_CALL(mock_fun, make_functor(k_suc_functor_id,
            std::vector<const expr*>{&zero}))
            .WillOnce(Return(&suc1));
        EXPECT_CALL(mock_fun, make_functor(k_suc_functor_id,
            std::vector<const expr*>{&suc1}))
            .WillOnce(Return(&suc2));
        EXPECT_CALL(mock_fun, make_functor(k_var_functor_id,
            std::vector<const expr*>{&suc2}))
            .WillOnce(Return(&var_term));
    }
    EXPECT_EQ(tx.transpile_var(lc_expr::var{2}), &var_term);
}
