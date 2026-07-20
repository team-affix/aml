#include <gtest/gtest.h>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/global_iterator.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/global.hpp"
#include "value_objects/module_file.hpp"

namespace {

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

struct GlobalIteratorTest : public ::testing::Test {
    aml_expr_pool aml;
};

} // namespace

TEST_F(GlobalIteratorTest, EmptyFilesYieldsNullopt) {
    std::vector<module_file> files;
    global_iterator it{files};
    EXPECT_EQ(it.get_next_global(), std::nullopt);
}

TEST_F(GlobalIteratorTest, SingleFileMultipleItems) {
    const aml_expr* body = aml.make_token("x");
    module_file file;
    file.items.push_back(global{make_group({{"a", 0u}})});
    file.items.push_back(global{definition{"f", body}});

    std::vector<module_file> files{file};
    global_iterator it{files};

    auto first = it.get_next_global();
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(std::holds_alternative<declaration_group>(*first));

    auto second = it.get_next_global();
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(std::holds_alternative<definition>(*second));
    EXPECT_EQ(std::get<definition>(*second).name, "f");

    EXPECT_EQ(it.get_next_global(), std::nullopt);
}

TEST_F(GlobalIteratorTest, SkipsEmptyFileBetweenItems) {
    module_file first;
    first.items.push_back(global{definition{"a", aml.make_token("x")}});
    module_file empty;
    module_file third;
    third.items.push_back(global{definition{"b", aml.make_token("y")}});

    std::vector<module_file> files{first, empty, third};
    global_iterator it{files};

    auto g0 = it.get_next_global();
    ASSERT_TRUE(g0.has_value());
    EXPECT_EQ(std::get<definition>(*g0).name, "a");

    auto g1 = it.get_next_global();
    ASSERT_TRUE(g1.has_value());
    EXPECT_EQ(std::get<definition>(*g1).name, "b");

    EXPECT_EQ(it.get_next_global(), std::nullopt);
}
