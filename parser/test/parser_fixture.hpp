#ifndef PARSER_FIXTURE_HPP
#define PARSER_FIXTURE_HPP

#include <gtest/gtest.h>
#include "parser/hpp/schema_visitor.hpp"

struct AmlParserFixture : public ::testing::Test {
    schema_visitor visitor;
};

#endif
