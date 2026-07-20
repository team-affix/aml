#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "infrastructure/global_processor.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/global.hpp"
#include "value_objects/lc_expr.hpp"

namespace {

struct MockTranspileExpr {
    MOCK_METHOD(const lc_expr*, transpile, (const aml_expr*), ());
};
struct MockTranspileDecl {
    MOCK_METHOD(const lc_expr*, transpile_decl, (uint32_t, uint32_t, uint32_t), ());
};
struct MockPushGlobal {
    MOCK_METHOD(void, push, (const lc_expr*), ());
};
struct MockPushScope {
    MOCK_METHOD(void, push, (std::string), ());
};

using NiceTx   = testing::NiceMock<MockTranspileExpr>;
using NiceDecl = testing::NiceMock<MockTranspileDecl>;
using NiceGlob = testing::NiceMock<MockPushGlobal>;
using NiceSc   = testing::NiceMock<MockPushScope>;

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

struct GlobalProcessorTest : public ::testing::Test {
    lc_expr  dummy_term{};
    NiceTx   tx;
    NiceDecl decl;
    NiceGlob globals;
    NiceSc   scope;
    global_processor<NiceTx, NiceDecl, NiceGlob, NiceSc>
        gp{tx, decl, globals, scope};
};

} // namespace

TEST_F(GlobalProcessorTest, DefinitionTranspilesThenPushes) {
    using testing::Return;

    aml_expr body{};
    EXPECT_CALL(tx, transpile(&body)).WillOnce(Return(&dummy_term));
    {
        testing::InSequence seq;
        EXPECT_CALL(globals, push(&dummy_term));
        EXPECT_CALL(scope, push(std::string("f")));
    }

    gp.process_global(global{definition{"f", &body}});
}

TEST_F(GlobalProcessorTest, DeclarationGroupTranspilesEachCtor) {
    using testing::Return;

    lc_expr t0{};
    lc_expr t1{};
    EXPECT_CALL(decl, transpile_decl(2u, 0u, 0u)).WillOnce(Return(&t0));
    EXPECT_CALL(decl, transpile_decl(2u, 1u, 0u)).WillOnce(Return(&t1));
    {
        testing::InSequence seq;
        EXPECT_CALL(globals, push(&t0));
        EXPECT_CALL(scope, push(std::string("true")));
        EXPECT_CALL(globals, push(&t1));
        EXPECT_CALL(scope, push(std::string("false")));
    }

    gp.process_global(global{make_group({{"true", 0u}, {"false", 0u}})});
}
