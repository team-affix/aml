// Integration test: parse a module, build scope with globals pre-pushed,
// wire transpiler bundle, transpile bodies, wrap with global_wrapper,
// and assert closed-term structure.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/global_wrapper.hpp"
#include "value_objects/transpiler_manifest.hpp"
#include "parser/generated/AMLLexer.h"
#include "parser/generated/AMLParser.h"
#include "parser/hpp/aml_visitor.hpp"

namespace {

aml_expr_pool& shared_pool() {
    static aml_expr_pool pool;
    return pool;
}

aml_visitor<aml_expr_pool> make_visitor() {
    return aml_visitor<aml_expr_pool>{shared_pool()};
}

module_file parse_module(const std::string& src) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    return make_visitor().parse_module_file(parser.moduleFile());
}

std::vector<declaration_group> groups_from(const module_file& mf) {
    std::vector<declaration_group> result;
    for (const auto& item : mf.items)
        if (const auto* g = std::get_if<declaration_group>(&item.content))
            result.push_back(*g);
    return result;
}

std::vector<definition> definitions_from(const module_file& mf) {
    std::vector<definition> result;
    for (const auto& item : mf.items)
        if (const auto* d = std::get_if<definition>(&item.content))
            result.push_back(*d);
    return result;
}

} // namespace

TEST(ProgramAssemblyTest, DeclarationGroupConstructorTerms) {
    auto gs = groups_from(parse_module("abc/0 | def/0 | ghi/1."));
    ASSERT_EQ(gs.size(), 1u);
    ASSERT_EQ(gs[0].declarations.size(), 3u);

    lc_expr_pool lc;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};
    auto pairs = dt.transpile_group(gs[0]);

    ASSERT_EQ(pairs.size(), 3u);
    EXPECT_EQ(pairs[0].first, "abc");
    EXPECT_EQ(pairs[1].first, "def");
    EXPECT_EQ(pairs[2].first, "ghi");

    // abc (k=0, a=0): abs(abs(abs(var(2))))
    EXPECT_EQ(pairs[0].second, lc.make_abs(lc.make_abs(lc.make_abs(lc.make_var(2)))));
    // def (k=1, a=0): abs(abs(abs(var(1))))
    EXPECT_EQ(pairs[1].second, lc.make_abs(lc.make_abs(lc.make_abs(lc.make_var(1)))));
    // ghi (k=2, a=1): abs(abs(abs(abs(app(var(1), var(0))))))
    EXPECT_EQ(pairs[2].second, lc.make_abs(lc.make_abs(lc.make_abs(lc.make_abs(
        lc.make_app(lc.make_var(1), lc.make_var(0)))))));
}

TEST(ProgramAssemblyTest, MainReferencingDeclarationGlobals) {
    module_file mod = parse_module("abc/0 | def/0 | ghi/1.\nmain = ghi abc.");
    auto gs = groups_from(mod);
    auto ds = definitions_from(mod);
    ASSERT_EQ(gs.size(), 1u);
    ASSERT_EQ(ds.size(), 1u);

    transpiler_manifest bundle;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{bundle.lc, bundle.lc, bundle.lc};

    auto ctor_pairs = dt.transpile_group(gs[0]);
    std::vector<const lc_expr*> global_lc_terms;
    for (const auto& [name, term] : ctor_pairs) {
        bundle.sc.push(name);
        global_lc_terms.push_back(term);
    }

    const lc_expr* main_lc = bundle.tx.transpile(ds[0].body);

    // abc=var(2), ghi=var(0) with 3 globals (depth=3)
    EXPECT_EQ(main_lc, bundle.lc.make_app(bundle.lc.make_var(0), bundle.lc.make_var(2)));

    for (size_t i = 0; i < global_lc_terms.size(); ++i)
        bundle.sc.pop();

    global_wrapper<lc_expr_pool, lc_expr_pool> gw{bundle.lc, bundle.lc};
    const lc_expr* program = gw.wrap(main_lc, global_lc_terms);

    const lc_expr* step1 = bundle.lc.make_app(bundle.lc.make_abs(main_lc), ctor_pairs[2].second);
    const lc_expr* step2 = bundle.lc.make_app(bundle.lc.make_abs(step1), ctor_pairs[1].second);
    const lc_expr* step3 = bundle.lc.make_app(bundle.lc.make_abs(step2), ctor_pairs[0].second);
    EXPECT_EQ(program, step3);
}

TEST(ProgramAssemblyTest, GlobalWrapperProducesClosedTerm) {
    auto gs = groups_from(parse_module("true/0 | false/0."));
    ASSERT_EQ(gs.size(), 1u);

    transpiler_manifest bundle;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{bundle.lc, bundle.lc, bundle.lc};
    auto pairs = dt.transpile_group(gs[0]);

    EXPECT_EQ(pairs[0].second, bundle.lc.make_abs(bundle.lc.make_abs(bundle.lc.make_var(1))));
    EXPECT_EQ(pairs[1].second, bundle.lc.make_abs(bundle.lc.make_abs(bundle.lc.make_var(0))));

    for (const auto& [name, term] : pairs) bundle.sc.push(name);
    auto ds = definitions_from(parse_module("not = b => b false true."));
    const lc_expr* not_lc = bundle.tx.transpile(ds[0].body);
    for (size_t i = 0; i < pairs.size(); ++i) bundle.sc.pop();

    global_wrapper<lc_expr_pool, lc_expr_pool> gw{bundle.lc, bundle.lc};
    std::vector<const lc_expr*> globals = {pairs[0].second, pairs[1].second};
    const lc_expr* program = gw.wrap(not_lc, globals);

    const lc_expr* expected =
        bundle.lc.make_app(
            bundle.lc.make_abs(
                bundle.lc.make_app(bundle.lc.make_abs(not_lc), pairs[1].second)),
            pairs[0].second);
    EXPECT_EQ(program, expected);
}
