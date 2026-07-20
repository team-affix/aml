// Integration: real iterators + pumps, mocked processors.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/global_iterator.hpp"
#include "infrastructure/global_pump.hpp"
#include "infrastructure/statement_iterator.hpp"
#include "infrastructure/statement_pump.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/global.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement.hpp"
#include "value_objects/statement_file.hpp"

namespace {

struct MockProcessGlobal {
    MOCK_METHOD(void, process_global, (const global&), ());
};
struct MockProcessStatement {
    MOCK_METHOD(void, process_statement, (const statement&), ());
};

using NiceGlobProc = testing::NiceMock<MockProcessGlobal>;
using NiceStmtProc = testing::NiceMock<MockProcessStatement>;

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

struct PumpIntegrationTest : public ::testing::Test {
    aml_expr_pool aml;
    NiceGlobProc  glob_proc;
    NiceStmtProc  stmt_proc;
};

} // namespace

TEST_F(PumpIntegrationTest, GlobalPumpDrainsIteratorAcrossFiles) {
    using testing::InSequence;

    const aml_expr* x = aml.make_symbol("x");
    global def_item{definition{"a", x}};
    global decl_item{make_group({{"true", 0u}})};

    module_file first;
    first.items.push_back(def_item);
    module_file empty;
    module_file third;
    third.items.push_back(decl_item);

    std::vector<module_file> files{first, empty, third};
    global_iterator it{files};
    global_pump<global_iterator, NiceGlobProc> pump{it, glob_proc};

    {
        InSequence seq;
        EXPECT_CALL(glob_proc, process_global(def_item));
        EXPECT_CALL(glob_proc, process_global(decl_item));
    }
    pump.pump();

    EXPECT_EQ(it.get_next_global(), std::nullopt);
}

TEST_F(PumpIntegrationTest, StatementPumpDrainsIteratorAcrossFiles) {
    using testing::InSequence;

    const aml_expr* a = aml.make_symbol("a");
    const aml_expr* b = aml.make_symbol("b");
    const aml_expr* c = aml.make_symbol("c");
    const aml_expr* d = aml.make_symbol("d");

    statement_file first;
    first.statements.push_back({a, b});
    statement_file empty;
    statement_file third;
    third.statements.push_back({c, d});

    std::vector<statement_file> files{first, empty, third};
    statement_iterator it{files};
    statement_pump<statement_iterator, NiceStmtProc> pump{it, stmt_proc};

    {
        InSequence seq;
        EXPECT_CALL(stmt_proc, process_statement(statement{a, b}));
        EXPECT_CALL(stmt_proc, process_statement(statement{c, d}));
    }
    pump.pump();

    EXPECT_EQ(it.get_next_statement(), std::nullopt);
}
