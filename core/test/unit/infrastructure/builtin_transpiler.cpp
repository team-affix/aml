#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/builtin_transpiler.hpp"
#include "value_objects/bool_decl_group.hpp"
#include "value_objects/int_decl_group.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/list_decl_group.hpp"
#include "value_objects/nat_decl_group.hpp"

namespace {

struct MockTranspileDecl {
    MOCK_METHOD(const lc_expr*, transpile_decl, (uint32_t, uint32_t, uint32_t), ());
};

using NiceDecl = testing::NiceMock<MockTranspileDecl>;

struct BuiltinTranspilerTest : public ::testing::Test {
    NiceDecl mock_decl;
    builtin_transpiler<NiceDecl> bt{mock_decl};
    lc_expr sentinel{};
};

} // namespace

TEST_F(BuiltinTranspilerTest, NamedHelpersMatchTranspileDeclEncodings) {
    using testing::Return;

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_true(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_false(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 2u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_cons(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_nil(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 1u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_suc(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_zero(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 1u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_pos(), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 1u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.transpile_negsuc(), &sentinel);
}

TEST_F(BuiltinTranspilerTest, TryTranspileNameDispatchesKnownCtors) {
    using testing::Return;

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_true_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_false_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 2u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_cons_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_nil_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 1u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_suc_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_zero_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 0u, 1u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_pos_name), &sentinel);

    EXPECT_CALL(mock_decl, transpile_decl(2u, 1u, 1u)).WillOnce(Return(&sentinel));
    EXPECT_EQ(bt.try_transpile_name(k_negsuc_name), &sentinel);
}

TEST_F(BuiltinTranspilerTest, TryTranspileNameUnknownReturnsNull) {
    EXPECT_CALL(mock_decl, transpile_decl).Times(0);
    EXPECT_EQ(bt.try_transpile_name("unknown"), nullptr);
}
