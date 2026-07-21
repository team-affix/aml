// import_statement_file: end-to-end path → statement_file.

#include <stdexcept>
#include <variant>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "parser/hpp/import_statement_file.hpp"
#include "value_objects/aml_expr.hpp"

namespace {

struct ImportStatementFileTest : public ::testing::Test {
    aml_expr_pool pool;
};

} // namespace

TEST_F(ImportStatementFileTest, MultiplyStmtsFixture) {
    statement_file sf =
        import_statement_file("parser/fixtures/multiply.stmts",
                              pool, pool, pool, pool, pool, pool, pool, pool);

    ASSERT_EQ(sf.statements.size(), 4u);
    for (const statement& s : sf.statements) {
        EXPECT_NE(s.lhs, nullptr);
        EXPECT_NE(s.rhs, nullptr);
    }

    // First stmt: multiply 3 4 12 : true  — rhs is symbol true
    ASSERT_TRUE(std::holds_alternative<aml_expr::symbol>(sf.statements.at(0).rhs->content));
    EXPECT_EQ(std::get<aml_expr::symbol>(sf.statements.at(0).rhs->content).name,
              "true");
    ASSERT_TRUE(std::holds_alternative<aml_expr::symbol>(sf.statements.at(2).rhs->content));
    EXPECT_EQ(std::get<aml_expr::symbol>(sf.statements.at(2).rhs->content).name,
              "false");
}

TEST_F(ImportStatementFileTest, BadPathThrows) {
    EXPECT_THROW(
        import_statement_file("parser/fixtures/nonexistent.stmts",
                              pool, pool, pool, pool, pool, pool, pool, pool),
        std::runtime_error);
}
