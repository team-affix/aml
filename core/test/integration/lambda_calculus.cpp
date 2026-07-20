// Integration tests for chc/aml.chc — pure lambda calculus with de Bruijn indices.
//
// Strategy: load the CHC database once per suite, then run each query through
// genius_runtime directly (no CLI solve loop). For ground queries we check
// solved vs. refuted; for queries with a result variable we normalise the
// binding and compare it against an expression built from the shared pool.
//
// Run with: ./build/core_debug_fast --gtest_filter='LambdaCalculusTest.*'
// Working directory must be the aml repo root so that "chc/aml.chc" resolves.
//
// Term constructors in the database (all Peano-indexed):
//   var(N)     -- de Bruijn variable; var(zero) is the innermost binder
//   abs(B)     -- abstraction
//   app(F, A)  -- application
//
// The CHC file uses Normalization by Evaluation (NBE): eval/3 drives terms to
// weak head normal form via a Krivine machine (no substitution), and reify/3
// converts the resulting value back to a beta-normal term.
//
// Notable combinators used in tests:
//   id  = abs(var(zero))                          -- λx.x
//   K   = abs(abs(var(suc(zero))))                -- λxy.x
//   S   = abs(abs(abs(app(app(var(2),var(0)),app(var(1),var(0))))))

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/genius_runtime.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/functor_names.hpp"
#include "import_database_from_file.hpp"
#include "import_goals_from_string.hpp"

namespace {

constexpr size_t   kDefaultMaxResolutions = 100'000;
constexpr size_t   kMediumMaxResolutions  = 1'000'000;
constexpr size_t   kHeavyMaxResolutions   = 5'000'000;
constexpr uint32_t kSeed                  = 42;
constexpr double   kExploration           = 1.414;
constexpr double   kHorizonExploration    = 2.0;

// ============================================================
// Test fixture
// ============================================================

struct LambdaCalculusTest : public ::testing::Test {
    // The database and pool are loaded once for the whole suite.
    static db              s_db;
    static expr_pool       s_pool;
    static std::map<std::string, uint32_t> s_fmap;
    static std::map<uint32_t, std::string> s_rev_fmap;
    static uint32_t        s_next_id;

    static void SetUpTestSuite() {
        s_next_id = k_first_user_functor_id;
        import_database_from_file(
            "chc/aml.chc", s_pool, s_pool, s_db, s_fmap, s_next_id);
        for (auto& [name, id] : s_fmap)
            s_rev_fmap[id] = name;
    }

    // --------------------------------------------------------
    // Query helpers
    // --------------------------------------------------------

    // Run a ground (no variables) or existential query.
    // Returns true if the solver finds at least one solution.
    static bool solves(const std::string& goal_str,
                       size_t max_res = kDefaultMaxResolutions) {
        initial_goal_exprs goals;
        non_backtracking_var_sequencer var_seq(0);
        auto fmap    = s_fmap;
        auto next_id = s_next_id;
        import_goals_from_string(
            goal_str, s_pool, s_pool, var_seq, goals, fmap, next_id);
        genius_runtime rt(s_db, goals, var_seq.peek(), max_res, kSeed, kExploration, kHorizonExploration);
        while (rt.next())
            if (rt.solved()) return true;
        return false;
    }

    // Run a query with one result variable and return its first normalised binding.
    // Returns nullptr if no solution is found within max_res.
    static const expr* solve_for(const std::string& goal_str,
                                 const std::string& var_name,
                                 size_t max_res = kDefaultMaxResolutions) {
        initial_goal_exprs goals;
        non_backtracking_var_sequencer var_seq(0);
        auto fmap    = s_fmap;
        auto next_id = s_next_id;
        auto var_idx_map = import_goals_from_string(
            goal_str, s_pool, s_pool, var_seq, goals, fmap, next_id);
        const uint32_t frame = var_seq.peek();
        genius_runtime rt(s_db, goals, frame, max_res, kSeed, kExploration, kHorizonExploration);
        while (rt.next()) {
            if (rt.solved()) {
                auto it = var_idx_map.find(var_name);
                if (it == var_idx_map.end()) return nullptr;
                return s_pool.import(rt.normalize({s_pool.make_var(it->second), 0}));
            }
        }
        return nullptr;
    }

    // --------------------------------------------------------
    // Expression builders (use the shared pool so pointer
    // equality works for EXPECT_EQ comparisons)
    // --------------------------------------------------------

    static uint32_t fid(const std::string& name) {
        return s_fmap.at(name);
    }

    // Build Peano natural: suc^n(zero)
    static const expr* peano(uint32_t n) {
        const expr* r = s_pool.make_functor(fid("zero"), {});
        for (uint32_t i = 0; i < n; ++i)
            r = s_pool.make_functor(fid("suc"), {r});
        return r;
    }

    // Build de Bruijn variable at index n
    static const expr* dv(uint32_t n) {
        return s_pool.make_functor(fid("var"), {peano(n)});
    }

    // Build abs(body)
    static const expr* lm(const expr* body) {
        return s_pool.make_functor(fid("abs"), {body});
    }

    // Build app(f, a)
    static const expr* ap(const expr* f, const expr* a) {
        return s_pool.make_functor(fid("app"), {f, a});
    }

    // Common combinators
    static const expr* id_term()  { return lm(dv(0)); }                            // λ. 0
    static const expr* k_term()   { return lm(lm(dv(1))); }                        // λ. λ. 1
    static const expr* s_term()   {                                                 // λ. λ. λ. (2 0)(1 0)
        return lm(lm(lm(ap(ap(dv(2), dv(0)), ap(dv(1), dv(0))))));
    }

    // --------------------------------------------------------
    // Church numerals
    // --------------------------------------------------------

    // c_n = λf.λx. f^n(x)
    static const expr* church(uint32_t n) {
        const expr* body = dv(0);                   // x at index 0
        for (uint32_t i = 0; i < n; ++i)
            body = ap(dv(1), body);                 // f at index 1
        return lm(lm(body));
    }

    // succ = λn.λf.λx. f(n f x)
    static const expr* succ_comb() {
        return lm(lm(lm(ap(dv(1), ap(ap(dv(2), dv(1)), dv(0))))));
    }

    // plus = λm.λn.λf.λx. m f (n f x)
    static const expr* plus_comb() {
        return lm(lm(lm(lm(ap(ap(dv(3), dv(1)), ap(ap(dv(2), dv(1)), dv(0)))))));
    }

    // times = λm.λn.λf. m (n f)
    static const expr* times_comb() {
        return lm(lm(lm(ap(dv(2), ap(dv(1), dv(0))))));
    }

    // --------------------------------------------------------
    // Boolean combinators (Church encoding)
    // --------------------------------------------------------

    static const expr* true_comb()  { return lm(lm(dv(1))); }    // K  = λxy.x
    static const expr* false_comb() { return lm(lm(dv(0))); }    // KI = λxy.y
    // and = λp.λq.p q p
    static const expr* and_comb()   { return lm(lm(ap(ap(dv(1), dv(0)), dv(1)))); }
    // or  = λp.λq.p p q
    static const expr* or_comb()    { return lm(lm(ap(ap(dv(1), dv(1)), dv(0)))); }
    // not = λp. p false true
    static const expr* not_comb()   {
        return lm(ap(ap(dv(0), false_comb()), true_comb()));
    }

    // --------------------------------------------------------
    // NBE value / environment constructors
    // (build the runtime values produced by eval/3 and reify/3)
    // --------------------------------------------------------

    // nil  — the empty environment
    static const expr* nil_env() {
        return s_pool.make_functor(fid("nil"), {});
    }

    // env(Arg, ArgEnv, Outer)
    static const expr* env_node(const expr* arg,
                                const expr* arg_env,
                                const expr* outer) {
        return s_pool.make_functor(fid("env"), {arg, arg_env, outer});
    }

    // clo(Body, Env)
    static const expr* clo_val(const expr* body, const expr* env) {
        return s_pool.make_functor(fid("clo"), {body, env});
    }

    // fvar(Level) — reification sentinel at Peano level
    static const expr* fvar_val(uint32_t level) {
        return s_pool.make_functor(fid("fvar"), {peano(level)});
    }

    // global_free(N) — free variable from input at de Bruijn index N
    static const expr* gfree(uint32_t n) {
        return s_pool.make_functor(fid("global_free"), {peano(n)});
    }

    // napp(Head, Arg, ArgEnv) — neutral application
    static const expr* napp_val(const expr* head,
                                const expr* arg,
                                const expr* arg_env) {
        return s_pool.make_functor(fid("napp"), {head, arg, arg_env});
    }

    // --------------------------------------------------------
    // Expr → CHC string converter + normalize_expr helper
    // --------------------------------------------------------

    // Convert a ground lambda term to its CHC functor string so it can be
    // embedded in a query passed to import_goals_from_string.
    static std::string expr_to_chc(const expr* e) {
        if (std::holds_alternative<expr::var>(e->content))
            return "_";   // solver variable — shouldn't appear in ground terms
        const auto& f = std::get<expr::functor>(e->content);
        auto it = s_rev_fmap.find(f.id);
        const std::string name = (it != s_rev_fmap.end()) ? it->second : "?";
        if (f.args.empty()) return name;
        std::string s = name + "(";
        for (size_t i = 0; i < f.args.size(); ++i) {
            if (i > 0) s += ", ";
            s += expr_to_chc(f.args.at(i));
        }
        return s + ")";
    }

    // Normalise a lambda term (given as an expr*) by round-tripping it through
    // the CHC solver.  Returns nullptr if max_res is exhausted.
    static const expr* normalize_expr(const expr* term,
                                      size_t max_res = kDefaultMaxResolutions) {
        return solve_for("normalize(" + expr_to_chc(term) + ", R)", "R", max_res);
    }
};

db              LambdaCalculusTest::s_db;
expr_pool       LambdaCalculusTest::s_pool;
std::map<std::string, uint32_t> LambdaCalculusTest::s_fmap;
std::map<uint32_t, std::string> LambdaCalculusTest::s_rev_fmap;
uint32_t        LambdaCalculusTest::s_next_id = k_first_user_functor_id;

// ============================================================
// Peano naturals
// ============================================================

TEST_F(LambdaCalculusTest, NatZeroHolds) {
    EXPECT_TRUE(solves("nat(zero)"));
}

TEST_F(LambdaCalculusTest, NatSuccZeroHolds) {
    EXPECT_TRUE(solves("nat(suc(zero))"));
}

TEST_F(LambdaCalculusTest, NatSuccSuccZeroHolds) {
    EXPECT_TRUE(solves("nat(suc(suc(zero)))"));
}

// ============================================================
// Comparison predicates
// ============================================================

TEST_F(LambdaCalculusTest, LtZeroSuccZero) {
    EXPECT_TRUE(solves("lt(zero, suc(zero))"));
}

TEST_F(LambdaCalculusTest, LtOneTwo) {
    EXPECT_TRUE(solves("lt(suc(zero), suc(suc(zero)))"));
}

TEST_F(LambdaCalculusTest, GtSuccZeroZero) {
    EXPECT_TRUE(solves("gt(suc(zero), zero)"));
}

TEST_F(LambdaCalculusTest, LeZeroZeroReflexive) {
    EXPECT_TRUE(solves("le(zero, zero)"));
}

TEST_F(LambdaCalculusTest, LeZeroOne) {
    EXPECT_TRUE(solves("le(zero, suc(zero))"));
}

TEST_F(LambdaCalculusTest, GeOneZero) {
    EXPECT_TRUE(solves("ge(suc(zero), zero)"));
}

TEST_F(LambdaCalculusTest, PredSuccZeroIsZero) {
    EXPECT_TRUE(solves("pred(suc(zero), zero)"));
}

TEST_F(LambdaCalculusTest, PredTwoIsOne) {
    EXPECT_TRUE(solves("pred(suc(suc(zero)), suc(zero))"));
}

// ============================================================
// Peano addition (plus/3)
// ============================================================

TEST_F(LambdaCalculusTest, PlusZeroZeroIsZero) {
    EXPECT_EQ(solve_for("plus(zero, zero, R)", "R"), peano(0));
}

TEST_F(LambdaCalculusTest, PlusZeroOneIsOne) {
    EXPECT_EQ(solve_for("plus(zero, suc(zero), R)", "R"), peano(1));
}

TEST_F(LambdaCalculusTest, PlusOneZeroIsOne) {
    EXPECT_EQ(solve_for("plus(suc(zero), zero, R)", "R"), peano(1));
}

TEST_F(LambdaCalculusTest, PlusTwoOneIsThree) {
    EXPECT_EQ(solve_for("plus(suc(suc(zero)), suc(zero), R)", "R"), peano(3));
}

TEST_F(LambdaCalculusTest, PlusOneTwoIsThree) {
    EXPECT_EQ(solve_for("plus(suc(zero), suc(suc(zero)), R)", "R"), peano(3));
}

// ============================================================
// Level to de Bruijn index (level_to_idx/3)
//
// level_to_idx(FvarLevel, CurrentLevel, Index)
//   Index = CurrentLevel - FvarLevel - 1
//   i.e. variable introduced at reification level L,
//        seen from level Level, has de Bruijn index Level-L-1.
// ============================================================

TEST_F(LambdaCalculusTest, LevelToIdxBase) {
    // level 0, current 1 → index 0
    EXPECT_EQ(solve_for("level_to_idx(zero, suc(zero), R)", "R"), peano(0));
}

TEST_F(LambdaCalculusTest, LevelToIdxOneStep) {
    // level 0, current 2 → index 1
    EXPECT_EQ(solve_for("level_to_idx(zero, suc(suc(zero)), R)", "R"), peano(1));
}

TEST_F(LambdaCalculusTest, LevelToIdxTwoSteps) {
    // level 0, current 3 → index 2
    EXPECT_EQ(solve_for("level_to_idx(zero, suc(suc(suc(zero))), R)", "R"), peano(2));
}

TEST_F(LambdaCalculusTest, LevelToIdxInnerLevel) {
    // level 1, current 2 → index 0
    EXPECT_EQ(solve_for("level_to_idx(suc(zero), suc(suc(zero)), R)", "R"), peano(0));
}

TEST_F(LambdaCalculusTest, LevelToIdxInnerLevelDeeper) {
    // level 1, current 3 → index 1
    EXPECT_EQ(solve_for("level_to_idx(suc(zero), suc(suc(suc(zero))), R)", "R"), peano(1));
}

// ============================================================
// eval/3 — Krivine machine (WHNF evaluation)
// ============================================================

TEST_F(LambdaCalculusTest, EvalAbsInNilGivesClosure) {
    // eval(abs(var(zero)), nil, R) → clo(var(zero), nil)
    EXPECT_EQ(solve_for("eval(abs(var(zero)), nil, R)", "R"),
              clo_val(dv(0), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalVarZeroInNilGivesGlobalFree) {
    // eval(var(zero), nil, R) → global_free(zero)
    EXPECT_EQ(solve_for("eval(var(zero), nil, R)", "R"), gfree(0));
}

TEST_F(LambdaCalculusTest, EvalVarOneInNilGivesGlobalFreeOne) {
    // eval(var(suc(zero)), nil, R) → global_free(suc(zero))
    EXPECT_EQ(solve_for("eval(var(suc(zero)), nil, R)", "R"), gfree(1));
}

TEST_F(LambdaCalculusTest, EvalVarZeroInEnvLookupsBinding) {
    // eval(var(zero), env(abs(var(zero)), nil, nil), R)
    //   → eval abs(var(zero)) in nil → clo(var(zero), nil)
    EXPECT_EQ(solve_for("eval(var(zero), env(abs(var(zero)), nil, nil), R)", "R"),
              clo_val(dv(0), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalVarOneInEnvSkipsToOuterBinding) {
    // eval(var(suc(zero)), env(abs(var(zero)), nil,
    //                         env(abs(abs(var(suc(zero)))), nil, nil)), R)
    //   → skip one slot → eval var(zero) in outer
    //   → eval K in nil → clo(abs(var(suc(zero))), nil)
    EXPECT_EQ(solve_for(
        "eval(var(suc(zero)),"
        "     env(abs(var(zero)), nil,"
        "         env(abs(abs(var(suc(zero)))), nil, nil)), R)", "R"),
        clo_val(lm(dv(1)), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalAppBetaStep) {
    // eval(app(abs(var(zero)), abs(var(zero))), nil, R)
    //   → drive function to clo(var(zero), nil)
    //   → eval var(zero) in env(abs(var(zero)), nil, nil) → clo(var(zero), nil)
    EXPECT_EQ(solve_for("eval(app(abs(var(zero)), abs(var(zero))), nil, R)", "R"),
              clo_val(dv(0), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalAppNeutralFreeHead) {
    // eval(app(var(zero), abs(var(zero))), nil, R)
    //   → var(zero) in nil → global_free(zero) (neutral)
    //   → napp(global_free(zero), abs(var(zero)), nil)
    EXPECT_EQ(solve_for("eval(app(var(zero), abs(var(zero))), nil, R)", "R"),
              napp_val(gfree(0), lm(dv(0)), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalFvarReturnsSelf) {
    // eval(fvar(zero), nil, R) → fvar(zero)   (sentinel passes through unchanged)
    EXPECT_EQ(solve_for("eval(fvar(zero), nil, R)", "R"), fvar_val(0));
}

// ============================================================
// eval_app/4 — dispatch on function value kind
// ============================================================

TEST_F(LambdaCalculusTest, EvalAppCloFiresBeta) {
    // eval_app(clo(var(zero), nil), abs(var(zero)), nil, R)
    //   → eval var(zero) in env(abs(var(zero)), nil, nil) → clo(var(zero), nil)
    EXPECT_EQ(solve_for(
        "eval_app(clo(var(zero), nil), abs(var(zero)), nil, R)", "R"),
        clo_val(dv(0), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalAppFvarBuildsNapp) {
    // eval_app(fvar(zero), abs(var(zero)), nil, R)
    //   → napp(fvar(zero), abs(var(zero)), nil)
    EXPECT_EQ(solve_for(
        "eval_app(fvar(zero), abs(var(zero)), nil, R)", "R"),
        napp_val(fvar_val(0), lm(dv(0)), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalAppGlobalFreeBuildsNapp) {
    // eval_app(global_free(zero), abs(var(zero)), nil, R)
    //   → napp(global_free(zero), abs(var(zero)), nil)
    EXPECT_EQ(solve_for(
        "eval_app(global_free(zero), abs(var(zero)), nil, R)", "R"),
        napp_val(gfree(0), lm(dv(0)), nil_env()));
}

TEST_F(LambdaCalculusTest, EvalAppNappBuildsNestedNapp) {
    // eval_app(napp(global_free(zero), var(zero), nil), abs(var(zero)), nil, R)
    //   → napp(napp(global_free(zero), var(zero), nil), abs(var(zero)), nil)
    const expr* inner = napp_val(gfree(0), dv(0), nil_env());
    EXPECT_EQ(solve_for(
        "eval_app(napp(global_free(zero), var(zero), nil), abs(var(zero)), nil, R)", "R"),
        napp_val(inner, lm(dv(0)), nil_env()));
}

// ============================================================
// reify/3 — value → beta-normal term
// ============================================================

TEST_F(LambdaCalculusTest, ReifyClosureAtLevelZeroGivesId) {
    // reify(clo(var(zero), nil), zero, R)
    //   → body eval under env(fvar(0), nil, nil) → fvar(0)
    //   → level_to_idx(0, 1, 0) → var(zero)
    //   → abs(var(zero)) = id
    EXPECT_EQ(solve_for("reify(clo(var(zero), nil), zero, R)", "R"), id_term());
}

TEST_F(LambdaCalculusTest, ReifyFvarAtImmediateLevel) {
    // reify(fvar(zero), suc(zero), R): level_to_idx(0,1) → index 0
    EXPECT_EQ(solve_for("reify(fvar(zero), suc(zero), R)", "R"), dv(0));
}

TEST_F(LambdaCalculusTest, ReifyFvarAtHigherLevel) {
    // reify(fvar(zero), suc(suc(zero)), R): level_to_idx(0,2) → index 1
    EXPECT_EQ(solve_for("reify(fvar(zero), suc(suc(zero)), R)", "R"), dv(1));
}

TEST_F(LambdaCalculusTest, ReifyInnerFvarAtHigherLevel) {
    // reify(fvar(suc(zero)), suc(suc(zero)), R): level_to_idx(1,2) → index 0
    EXPECT_EQ(solve_for("reify(fvar(suc(zero)), suc(suc(zero)), R)", "R"), dv(0));
}

TEST_F(LambdaCalculusTest, ReifyGlobalFreeAtLevelZero) {
    // reify(global_free(zero), zero, R): plus(0,0) → index 0
    EXPECT_EQ(solve_for("reify(global_free(zero), zero, R)", "R"), dv(0));
}

TEST_F(LambdaCalculusTest, ReifyGlobalFreeAtLevelOne) {
    // reify(global_free(zero), suc(zero), R): plus(0,1) → index 1
    EXPECT_EQ(solve_for("reify(global_free(zero), suc(zero), R)", "R"), dv(1));
}

TEST_F(LambdaCalculusTest, ReifyGlobalFreeOneAtLevelOne) {
    // reify(global_free(suc(zero)), suc(zero), R): plus(1,1) → index 2
    EXPECT_EQ(solve_for("reify(global_free(suc(zero)), suc(zero), R)", "R"), dv(2));
}

TEST_F(LambdaCalculusTest, ReifyNappGivesApp) {
    // reify(napp(global_free(zero), abs(var(zero)), nil), zero, R)
    //   head: global_free(0) at level 0 → var(zero)
    //   arg:  eval abs(var(zero)) in nil → clo → reify → abs(var(zero)) = id
    //   → app(var(zero), abs(var(zero)))
    EXPECT_EQ(solve_for(
        "reify(napp(global_free(zero), abs(var(zero)), nil), zero, R)", "R"),
        ap(dv(0), id_term()));
}

TEST_F(LambdaCalculusTest, ReifyNestedNappGivesNestedApp) {
    // reify(napp(napp(global_free(zero), var(zero), nil), abs(var(zero)), nil), zero, R)
    //   → app(app(var(zero), var(zero)), abs(var(zero)))
    const expr* inner_napp = napp_val(gfree(0), dv(0), nil_env());
    EXPECT_EQ(solve_for(
        "reify(napp(napp(global_free(zero), var(zero), nil),"
        "          abs(var(zero)), nil), zero, R)", "R"),
        ap(ap(dv(0), dv(0)), id_term()));
}

// ============================================================
// Well-formed terms
// ============================================================

TEST_F(LambdaCalculusTest, TermVar) {
    EXPECT_TRUE(solves("term(var(zero))"));
}

TEST_F(LambdaCalculusTest, TermAbs) {
    EXPECT_TRUE(solves("term(abs(var(zero)))"));
}

TEST_F(LambdaCalculusTest, TermApp) {
    EXPECT_TRUE(solves("term(app(abs(var(zero)), var(zero)))"));
}

TEST_F(LambdaCalculusTest, TermNestedApp) {
    EXPECT_TRUE(solves("term(app(app(abs(abs(var(suc(zero)))), var(zero)), abs(var(zero))))"));
}

// ============================================================
// Normalize — already-normal terms
// ============================================================

TEST_F(LambdaCalculusTest, NormalizeVar) {
    const expr* result = solve_for("normalize(var(zero), R)", "R");
    EXPECT_EQ(result, dv(0));
}

TEST_F(LambdaCalculusTest, NormalizeAbsVar) {
    // λ.0 is already normal
    const expr* result = solve_for("normalize(abs(var(zero)), R)", "R");
    EXPECT_EQ(result, id_term());
}

TEST_F(LambdaCalculusTest, NormalizeAppVarVar) {
    // app(var(0), var(0)) — no redex, already normal
    const expr* result = solve_for("normalize(app(var(zero), var(zero)), R)", "R");
    EXPECT_EQ(result, ap(dv(0), dv(0)));
}

// ============================================================
// Normalize — single beta step needed
// ============================================================

TEST_F(LambdaCalculusTest, NormalizeIdentityOnVar) {
    // (λ.0) var(0) → var(0)
    const expr* result = solve_for(
        "normalize(app(abs(var(zero)), var(zero)), R)", "R");
    EXPECT_EQ(result, dv(0));
}

TEST_F(LambdaCalculusTest, NormalizeIdentityOnIdentity) {
    // (λ.0)(λ.0) → λ.0
    const expr* result = solve_for(
        "normalize(app(abs(var(zero)), abs(var(zero))), R)", "R");
    EXPECT_EQ(result, id_term());
}

// ============================================================
// Normalize — multi-step reductions
// ============================================================

TEST_F(LambdaCalculusTest, NormalizeSelfApplicationOfIdentity) {
    // (λx.xx)(id) → id id → id
    const expr* result = solve_for(
        "normalize(app(abs(app(var(zero), var(zero))), abs(var(zero))), R)", "R");
    EXPECT_EQ(result, id_term());
}

TEST_F(LambdaCalculusTest, NormalizeKAppliedToTwoIdentities) {
    // K id id = id   (K returns its first arg)
    // K = abs(abs(var(1))), id = abs(var(0))
    const expr* result = solve_for(
        "normalize("
        "  app(app(abs(abs(var(suc(zero)))), abs(var(zero))), abs(var(zero))"
        "), R)", "R");
    EXPECT_EQ(result, id_term());
}

TEST_F(LambdaCalculusTest, NormalizeKAppliedToIdAndK) {
    // K id K = id
    const expr* result = solve_for(
        "normalize("
        "  app(app(abs(abs(var(suc(zero)))), abs(var(zero))),"
        "      abs(abs(var(suc(zero)))))"
        ", R)", "R");
    EXPECT_EQ(result, id_term());
}

TEST_F(LambdaCalculusTest, NormalizeKFirstArgReturnedAsAbs) {
    // K (K) id = K  (the first arg, K itself, is returned)
    // K K id → K
    const expr* result = solve_for(
        "normalize("
        "  app(app(abs(abs(var(suc(zero)))), abs(abs(var(suc(zero))))),"
        "      abs(var(zero)))"
        ", R)", "R");
    EXPECT_EQ(result, k_term());
}

TEST_F(LambdaCalculusTest, NormalizeSKKIsIdentity) {
    // S K K = I  (classical combinatory logic identity)
    // S = abs(abs(abs(app(app(var(2),var(0)),app(var(1),var(0))))))
    // S K K x → K x (K x) → x
    // Test: S K K applied to id yields id
    const expr* result = solve_for(
        "normalize("
        "  app(app(app("
        "    abs(abs(abs(app(app(var(suc(suc(zero))), var(zero)),"
        "                    app(var(suc(zero)), var(zero)))))),"
        "    abs(abs(var(suc(zero))))),"
        "    abs(abs(var(suc(zero))))),"
        "  abs(var(zero)))"
        ", R)", "R", 500'000);
    EXPECT_EQ(result, id_term());
}

// ============================================================
// Church numeral — successor
// succ = λn.λf.λx. f(n f x)     c_n = λf.λx. f^n(x)
// ============================================================

TEST_F(LambdaCalculusTest, ChurchSuccZeroEqualsOne) {
    EXPECT_EQ(normalize_expr(ap(succ_comb(), church(0))), church(1));
}

TEST_F(LambdaCalculusTest, ChurchSuccOneEqualsTwo) {
    EXPECT_EQ(normalize_expr(ap(succ_comb(), church(1))), church(2));
}

TEST_F(LambdaCalculusTest, ChurchSuccTwoEqualsThree) {
    EXPECT_EQ(normalize_expr(ap(succ_comb(), church(2)), kMediumMaxResolutions),
              church(3));
}

// succ(succ(c0)) = c2
TEST_F(LambdaCalculusTest, ChurchSuccAppliedTwiceToZero) {
    EXPECT_EQ(normalize_expr(ap(succ_comb(), ap(succ_comb(), church(0))),
                             kMediumMaxResolutions),
              church(2));
}

// ============================================================
// Church numeral — addition
// plus = λm.λn.λf.λx. m f (n f x)
//
// Two outer betas consume plus's two arguments, then the body
// reduces as m and n apply f repeatedly.  Exercises the solver's
// ability to chain multiple distinct sequences of reductions.
// ============================================================

TEST_F(LambdaCalculusTest, ChurchPlusOneOneEqualsTwo) {
    EXPECT_EQ(normalize_expr(ap(ap(plus_comb(), church(1)), church(1)),
                             kMediumMaxResolutions),
              church(2));
}

TEST_F(LambdaCalculusTest, ChurchPlusTwoOneEqualsThree) {
    EXPECT_EQ(normalize_expr(ap(ap(plus_comb(), church(2)), church(1)),
                             kMediumMaxResolutions),
              church(3));
}

// ============================================================
// Church numeral — multiplication
// times = λm.λn.λf. m (n f)
// ============================================================

TEST_F(LambdaCalculusTest, ChurchTimesOneTwoEqualsTwo) {
    EXPECT_EQ(normalize_expr(ap(ap(times_comb(), church(1)), church(2)),
                             kMediumMaxResolutions),
              church(2));
}

TEST_F(LambdaCalculusTest, ChurchTimesTwoOneEqualsTwo) {
    EXPECT_EQ(normalize_expr(ap(ap(times_comb(), church(2)), church(1)),
                             kMediumMaxResolutions),
              church(2));
}

TEST_F(LambdaCalculusTest, ChurchTimesTwoTwoEqualsFour) {
    EXPECT_EQ(normalize_expr(ap(ap(times_comb(), church(2)), church(2)),
                             kHeavyMaxResolutions),
              church(4));
}

// ============================================================
// Boolean combinators (Church encoding)
// true  = K  = λxy.x       false = KI = λxy.y
// and   = λpq. p q p        or   = λpq. p p q
// not   = λp.  p false true
//
// These require only two or three beta steps each; the stress
// comes from the solver having to search through multiple
// candidate unifications before committing to the correct branch.
// ============================================================

TEST_F(LambdaCalculusTest, BoolAndTrueTrue) {
    EXPECT_EQ(normalize_expr(ap(ap(and_comb(), true_comb()), true_comb())),
              true_comb());
}

TEST_F(LambdaCalculusTest, BoolAndTrueFalse) {
    EXPECT_EQ(normalize_expr(ap(ap(and_comb(), true_comb()), false_comb())),
              false_comb());
}

TEST_F(LambdaCalculusTest, BoolAndFalseTrue) {
    EXPECT_EQ(normalize_expr(ap(ap(and_comb(), false_comb()), true_comb())),
              false_comb());
}

TEST_F(LambdaCalculusTest, BoolAndFalseFalse) {
    EXPECT_EQ(normalize_expr(ap(ap(and_comb(), false_comb()), false_comb())),
              false_comb());
}

TEST_F(LambdaCalculusTest, BoolOrFalseFalse) {
    EXPECT_EQ(normalize_expr(ap(ap(or_comb(), false_comb()), false_comb())),
              false_comb());
}

TEST_F(LambdaCalculusTest, BoolOrFalseTrue) {
    EXPECT_EQ(normalize_expr(ap(ap(or_comb(), false_comb()), true_comb())),
              true_comb());
}

TEST_F(LambdaCalculusTest, BoolOrTrueFalse) {
    EXPECT_EQ(normalize_expr(ap(ap(or_comb(), true_comb()), false_comb())),
              true_comb());
}

TEST_F(LambdaCalculusTest, BoolOrTrueTrue) {
    EXPECT_EQ(normalize_expr(ap(ap(or_comb(), true_comb()), true_comb())),
              true_comb());
}

TEST_F(LambdaCalculusTest, BoolNotTrue) {
    EXPECT_EQ(normalize_expr(ap(not_comb(), true_comb())), false_comb());
}

TEST_F(LambdaCalculusTest, BoolNotFalse) {
    EXPECT_EQ(normalize_expr(ap(not_comb(), false_comb())), true_comb());
}

// Double negation requires the solver to chain two not-reductions;
// each not reduction unfolds into a three-step beta sequence.
TEST_F(LambdaCalculusTest, BoolDoubleNegationTrue) {
    EXPECT_EQ(normalize_expr(ap(not_comb(), ap(not_comb(), true_comb())),
                             kMediumMaxResolutions),
              true_comb());
}

TEST_F(LambdaCalculusTest, BoolDoubleNegationFalse) {
    EXPECT_EQ(normalize_expr(ap(not_comb(), ap(not_comb(), false_comb())),
                             kMediumMaxResolutions),
              false_comb());
}

// ============================================================
// Divergence guard
// Ω = (λx.xx)(λx.xx) has no normal form.
// The solver must exhaust its budget and return nullptr rather
// than looping forever — this verifies the resolution cap works.
// ============================================================

TEST_F(LambdaCalculusTest, OmegaDivergesWithinBudget) {
    const expr* omega_small = lm(ap(dv(0), dv(0)));   // λx.x x
    const expr* omega       = ap(omega_small, omega_small);   // Ω
    EXPECT_EQ(normalize_expr(omega, 500), nullptr);
}

// ============================================================
// Normalize — open terms (free variables in the input)
//
// With NBE, free variables evaluate to global_free(N) and are
// reified back to var(N + reification_level), so they shift
// correctly when passed under binders.
// ============================================================

TEST_F(LambdaCalculusTest, NormalizeFreeVarPassesThroughUnchanged) {
    // var(zero) at top level is free — no binders, already normal.
    EXPECT_EQ(normalize_expr(dv(0)), dv(0));
}

TEST_F(LambdaCalculusTest, NormalizeAbsWithFreeVarUnchanged) {
    // abs(var(suc(zero))): var(1) refers to the free variable
    // outside the lambda — already in normal form.
    EXPECT_EQ(normalize_expr(lm(dv(1))), lm(dv(1)));
}

TEST_F(LambdaCalculusTest, BetaDropsArgReturnsOuterFreeVar) {
    // (λx. outer_free) arg  =  outer_free
    // abs(var(suc(zero))) ignores its argument and returns the
    // outer free variable.  After reduction the free variable is
    // at index 0 from the top level.
    EXPECT_EQ(normalize_expr(ap(lm(dv(1)), id_term())), dv(0));
}

TEST_F(LambdaCalculusTest, BetaUnderAbsWithFreeVar) {
    // abs(app(abs(var(suc(zero))), var(zero)))
    //   = λx. (λ_. x) x
    //   Under the abs, var(zero)=x and var(suc(zero))=x (the outer bound var).
    //   Inner beta: (λ_. x) x → x = var(zero) from inside the outer abs.
    //   Result: abs(var(zero)) = id
    EXPECT_EQ(normalize_expr(lm(ap(lm(dv(1)), dv(0)))), id_term());
}

// ============================================================
// Normalize — neutral terms
//
// When the head of an application is a free variable, no beta
// reduction is possible.  reify must recursively normalise the
// arguments and produce a structurally identical term.
// ============================================================

TEST_F(LambdaCalculusTest, NormalizeNeutralAppFreeVarTimesAbs) {
    // app(var(zero), abs(var(zero))): head is free → neutral, unchanged
    EXPECT_EQ(normalize_expr(ap(dv(0), id_term())), ap(dv(0), id_term()));
}

TEST_F(LambdaCalculusTest, NormalizeNeutralNestedApp) {
    // app(app(var(zero), var(zero)), abs(var(zero))): double neutral, unchanged
    EXPECT_EQ(normalize_expr(ap(ap(dv(0), dv(0)), id_term())),
              ap(ap(dv(0), dv(0)), id_term()));
}

TEST_F(LambdaCalculusTest, NeutralArgGetsNormalisedBeforeResult) {
    // app(var(zero), app(abs(var(zero)), abs(var(zero))))
    //   head is free, but the argument is a redex that normalises to id.
    //   Result: app(var(zero), abs(var(zero)))
    EXPECT_EQ(normalize_expr(ap(dv(0), ap(id_term(), id_term()))),
              ap(dv(0), id_term()));
}

TEST_F(LambdaCalculusTest, NormalizeAbsNeutralBodyUnchanged) {
    // abs(app(var(suc(zero)), var(zero))): body is neutral —
    // outer free var applied to the bound var. Already normal.
    EXPECT_EQ(normalize_expr(lm(ap(dv(1), dv(0)))), lm(ap(dv(1), dv(0))));
}

TEST_F(LambdaCalculusTest, NeutralBodyArgGetsNormalisedUnderAbs) {
    // abs(app(var(suc(zero)), app(abs(var(zero)), var(zero))))
    //   Under the abs: outer free var applied to a redex.
    //   Inner redex: (λx.x) var(zero) → var(zero)
    //   Result: abs(app(var(suc(zero)), var(zero)))
    EXPECT_EQ(normalize_expr(lm(ap(dv(1), ap(id_term(), dv(0))))),
              lm(ap(dv(1), dv(0))));
}

} // namespace
