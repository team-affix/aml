#include <gtest/gtest.h>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/statement_iterator.hpp"
#include "value_objects/statement.hpp"
#include "value_objects/statement_file.hpp"

namespace {

struct StatementIteratorTest : public ::testing::Test {
    aml_expr_pool aml;
};

} // namespace

TEST_F(StatementIteratorTest, EmptyFilesYieldsNullopt) {
    std::vector<statement_file> files;
    statement_iterator it{files};
    EXPECT_EQ(it.get_next_statement(), std::nullopt);
}

TEST_F(StatementIteratorTest, SingleFileMultipleStatements) {
    statement_file file;
    file.statements.push_back({aml.make_symbol("a"), aml.make_symbol("b")});
    file.statements.push_back({aml.make_symbol("c"), aml.make_symbol("d")});

    std::vector<statement_file> files{file};
    statement_iterator it{files};

    auto first = it.get_next_statement();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->lhs, file.statements.at(0).lhs);

    auto second = it.get_next_statement();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->rhs, file.statements.at(1).rhs);

    EXPECT_EQ(it.get_next_statement(), std::nullopt);
}

TEST_F(StatementIteratorTest, SkipsEmptyFileBetweenStatements) {
    statement_file first;
    first.statements.push_back({aml.make_symbol("a"), aml.make_symbol("b")});
    statement_file empty;
    statement_file third;
    third.statements.push_back({aml.make_symbol("c"), aml.make_symbol("d")});

    std::vector<statement_file> files{first, empty, third};
    statement_iterator it{files};

    ASSERT_TRUE(it.get_next_statement().has_value());
    ASSERT_TRUE(it.get_next_statement().has_value());
    EXPECT_EQ(it.get_next_statement(), std::nullopt);
}
