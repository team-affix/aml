// Integration test: parse declaration group + definitions, build scope with
// globals pre-pushed, wire transpiler bundle, transpile bodies, wrap with
// global_wrapper, and assert closed-term structure.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/global_wrapper.hpp"
#include "infrastructure/lc_transpile_bundle.hpp"
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

declaration_file parse_decls(const std::string& src) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    return make_visitor().parse_declaration_file(parser.declarationFile());
}

definition_file parse_defs(const std::string& src) {
    antlr4::ANTLRInputStream stream(src);
    AMLLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    AMLParser parser(&tokens);
    parser.removeErrorListeners();
    lexer.removeErrorListeners();
    return make_visitor().parse_definition_file(parser.definitionFile());
}

} // namespace

TEST(ProgramAssemblyTest, DeclarationGroupConstructorTerms) {
    declaration_file decls = parse_decls("abc/0 | def/0 | ghi/1.");
    ASSERT_EQ(decls.groups.size(), 1u);
    ASSERT_EQ(decls.groups[0].declarations.size(), 3u);

    lc_expr_pool lc;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{lc, lc, lc};
    auto pairs = dt.transpile_group(decls.groups[0]);

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
    // Declaration group: abc/0 | def/0 | ghi/1
    // Main body: "main = ghi abc."  (uses first and last globals)
    declaration_file decls = parse_decls("abc/0 | def/0 | ghi/1.");
    definition_file  defs  = parse_defs("main = ghi abc.");
    ASSERT_EQ(decls.groups.size(), 1u);
    ASSERT_EQ(defs.definitions.size(), 1u);

    lc_transpile_bundle bundle;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{bundle.lc, bundle.lc, bundle.lc};

    // Push globals into scope + collect lc terms
    auto ctor_pairs = dt.transpile_group(decls.groups[0]);
    std::vector<const lc_expr*> global_lc_terms;
    for (const auto& [name, term] : ctor_pairs) {
        bundle.sc.push(name);
        global_lc_terms.push_back(term);
    }

    // Transpile main's body with globals in scope
    const lc_expr* main_lc = bundle.tx.transpile(defs.definitions[0].body);

    // Verify token resolution: abc=var(2), ghi=var(0) with 3 globals (depth=3)
    // "main = ghi abc" → app(var(0), var(2))
    EXPECT_EQ(main_lc, bundle.lc.make_app(bundle.lc.make_var(0), bundle.lc.make_var(2)));

    // Pop globals back out
    for (size_t i = 0; i < global_lc_terms.size(); ++i)
        bundle.sc.pop();

    // Wrap main with globals
    global_wrapper<lc_expr_pool, lc_expr_pool> gw{bundle.lc, bundle.lc};
    const lc_expr* program = gw.wrap(main_lc, global_lc_terms);

    // Expected: app(abs(app(abs(app(abs(main_lc), ghi_lc)), def_lc)), abc_lc)
    const lc_expr* step1 = bundle.lc.make_app(bundle.lc.make_abs(main_lc), ctor_pairs[2].second);
    const lc_expr* step2 = bundle.lc.make_app(bundle.lc.make_abs(step1), ctor_pairs[1].second);
    const lc_expr* step3 = bundle.lc.make_app(bundle.lc.make_abs(step2), ctor_pairs[0].second);
    EXPECT_EQ(program, step3);
}

TEST(ProgramAssemblyTest, GlobalWrapperProducesClosedTerm) {
    // Wrap a simple main expression with global_wrapper and verify structure.
    // Globals: just "true/0 | false/0"  (2 constructors)
    declaration_file decls = parse_decls("true/0 | false/0.");
    ASSERT_EQ(decls.groups.size(), 1u);

    lc_transpile_bundle bundle;
    declaration_transpiler<lc_expr_pool, lc_expr_pool, lc_expr_pool> dt{bundle.lc, bundle.lc, bundle.lc};
    auto pairs = dt.transpile_group(decls.groups[0]);

    // true = abs(abs(var(1))), false = abs(abs(var(0)))
    EXPECT_EQ(pairs[0].second, bundle.lc.make_abs(bundle.lc.make_abs(bundle.lc.make_var(1))));
    EXPECT_EQ(pairs[1].second, bundle.lc.make_abs(bundle.lc.make_abs(bundle.lc.make_var(0))));

    for (const auto& [name, term] : pairs) bundle.sc.push(name);
    // main = "not = b => b false true"
    definition_file defs = parse_defs("not = b => b false true.");
    const lc_expr* not_lc = bundle.tx.transpile(defs.definitions[0].body);
    for (size_t i = 0; i < pairs.size(); ++i) bundle.sc.pop();

    // Wrap: app(abs(app(abs(not_lc), false_lc)), true_lc)
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
