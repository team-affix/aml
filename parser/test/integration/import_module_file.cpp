// import_module_file: end-to-end path → module_file.

#include <stdexcept>
#include <variant>
#include <gtest/gtest.h>
#include "infrastructure/aml_expr_pool.hpp"
#include "parser/hpp/import_module_file.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/global.hpp"

namespace {

struct ImportModuleFileTest : public ::testing::Test {
    aml_expr_pool pool;
};

} // namespace

TEST_F(ImportModuleFileTest, LogicDefFixture) {
    module_file mod =
        import_module_file("parser/fixtures/logic.def",
                           pool, pool, pool, pool, pool, pool, pool, pool);

    ASSERT_EQ(mod.items.size(), 3u);
    ASSERT_TRUE(std::holds_alternative<definition>(mod.items.at(0)));
    EXPECT_EQ(std::get<definition>(mod.items.at(0)).name, "Y");
    ASSERT_TRUE(std::holds_alternative<definition>(mod.items.at(1)));
    EXPECT_EQ(std::get<definition>(mod.items.at(1)).name, "not");
    ASSERT_TRUE(std::holds_alternative<definition>(mod.items.at(2)));
    EXPECT_EQ(std::get<definition>(mod.items.at(2)).name, "if_then_else");
    EXPECT_NE(std::get<definition>(mod.items.at(0)).body, nullptr);
}

TEST_F(ImportModuleFileTest, BoolNatDeclFixture) {
    module_file mod =
        import_module_file("parser/fixtures/bool_nat.decl",
                           pool, pool, pool, pool, pool, pool, pool, pool);

    ASSERT_EQ(mod.items.size(), 4u);
    for (const global& item : mod.items)
        ASSERT_TRUE(std::holds_alternative<declaration_group>(item));

    const auto& bools = std::get<declaration_group>(mod.items.at(0));
    ASSERT_EQ(bools.declarations.size(), 2u);
    EXPECT_EQ(bools.declarations.at(0).name, "true");
    EXPECT_EQ(bools.declarations.at(0).arity, 0u);
    EXPECT_EQ(bools.declarations.at(1).name, "false");
    EXPECT_EQ(bools.declarations.at(1).arity, 0u);

    const auto& cons_nil = std::get<declaration_group>(mod.items.at(2));
    ASSERT_EQ(cons_nil.declarations.size(), 2u);
    EXPECT_EQ(cons_nil.declarations.at(0).name, "cons");
    EXPECT_EQ(cons_nil.declarations.at(0).arity, 2u);
    EXPECT_EQ(cons_nil.declarations.at(1).name, "nil");
    EXPECT_EQ(cons_nil.declarations.at(1).arity, 0u);
}

TEST_F(ImportModuleFileTest, BadPathThrows) {
    EXPECT_THROW(
        import_module_file("parser/fixtures/nonexistent.def",
                           pool, pool, pool, pool, pool, pool, pool, pool),
        std::runtime_error);
}
