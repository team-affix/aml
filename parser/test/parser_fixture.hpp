#ifndef PARSER_FIXTURE_HPP
#define PARSER_FIXTURE_HPP

#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "parser/hpp/aml_visitor.hpp"

struct AmlParserFixture : public ::testing::Test {
    aml_expr_pool pool;
    aml_visitor<aml_expr_pool, aml_expr_pool, aml_expr_pool, aml_expr_pool,
                aml_expr_pool, aml_expr_pool, aml_expr_pool, aml_expr_pool>
        visitor{pool, pool, pool, pool, pool, pool, pool, pool};
};

#endif
