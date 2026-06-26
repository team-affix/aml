// Integration tests for chc/aml.chc — pure lambda calculus with de Bruijn indices.
//
// Strategy: load the CHC database once per suite, then run each query through
// genius_runtime directly (no CLI solve loop). For ground queries we check
// solved vs. refuted; for queries with a result variable we normalise the
// binding and compare it against an expression built from the shared pool.
//
// Run with: ./build/cli_debug --gtest_filter='LambdaCalculusTest.*'
// Working directory must be the aml repo root so that "chc/aml.chc" resolves.
//
// Term constructors in the database (all Peano-indexed):
//   var(N)     -- de Bruijn variable; var(zero) is the innermost binder
//   abs(B)     -- abstraction
//   app(F, A)  -- application
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
        genius_runtime rt(s_db, goals, var_seq.peek(), max_res, kSeed, kExploration);
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
        genius_runtime rt(s_db, goals, frame, max_res, kSeed, kExploration);
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
            s += expr_to_chc(f.args[i]);
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
// not_abs
// ============================================================

TEST_F(LambdaCalculusTest, NotAbsVar) {
    EXPECT_TRUE(solves("not_abs(var(zero))"));
}

TEST_F(LambdaCalculusTest, NotAbsApp) {
    EXPECT_TRUE(solves("not_abs(app(var(zero), var(zero)))"));
}

TEST_F(LambdaCalculusTest, NotAbsRefutedForAbs) {
    // abs is an abstraction — not_abs must not hold for it
    EXPECT_FALSE(solves("not_abs(abs(var(zero)))", 500));
}

// ============================================================
// Normal form detection
// ============================================================

TEST_F(LambdaCalculusTest, NormalVar) {
    EXPECT_TRUE(solves("normal(var(zero))"));
}

TEST_F(LambdaCalculusTest, NormalAbsVar) {
    EXPECT_TRUE(solves("normal(abs(var(zero)))"));
}

TEST_F(LambdaCalculusTest, NormalAbsAbsVar) {
    EXPECT_TRUE(solves("normal(abs(abs(var(suc(zero)))))"));
}

TEST_F(LambdaCalculusTest, NormalAppVarVar) {
    // app(var(0), var(0)) — head is a var, not an abs, so this is in normal form
    EXPECT_TRUE(solves("normal(app(var(zero), var(zero)))"));
}

TEST_F(LambdaCalculusTest, NormalRefutedForBetaRedex) {
    // app(abs(var(zero)), var(zero)) contains a beta-redex — not normal
    EXPECT_FALSE(solves("normal(app(abs(var(zero)), var(zero)))", 500));
}

// ============================================================
// Shift
// ============================================================

TEST_F(LambdaCalculusTest, ShiftFreeVarIncrements) {
    // shift(var(0), 0, R): index 0 >= cutoff 0, so R = var(1)
    const expr* result = solve_for("shift(var(zero), zero, R)", "R");
    EXPECT_EQ(result, dv(1));
}

TEST_F(LambdaCalculusTest, ShiftBoundVarUnchanged) {
    // shift(var(0), suc(zero), R): index 0 < cutoff 1, bound, R = var(0)
    const expr* result = solve_for("shift(var(zero), suc(zero), R)", "R");
    EXPECT_EQ(result, dv(0));
}

TEST_F(LambdaCalculusTest, ShiftVarOneAtCutoffZeroIncrements) {
    // shift(var(1), 0, R): index 1 >= cutoff 0, R = var(2)
    const expr* result = solve_for("shift(var(suc(zero)), zero, R)", "R");
    EXPECT_EQ(result, dv(2));
}

TEST_F(LambdaCalculusTest, ShiftUnderAbsLeavesAbsBoundVar) {
    // shift(abs(var(0)), 0, R): under abs the cutoff becomes 1;
    // var(0) < 1 so it is bound — R = abs(var(0))
    const expr* result = solve_for("shift(abs(var(zero)), zero, R)", "R");
    EXPECT_EQ(result, lm(dv(0)));
}

TEST_F(LambdaCalculusTest, ShiftUnderAbsShiftsFreeVar) {
    // shift(abs(var(1)), 0, R): under abs cutoff = 1; var(1) >= 1 so free — R = abs(var(2))
    const expr* result = solve_for("shift(abs(var(suc(zero))), zero, R)", "R");
    EXPECT_EQ(result, lm(dv(2)));
}

TEST_F(LambdaCalculusTest, ShiftAppShiftsBothSides) {
    // shift(app(var(0), var(1)), 0, R): both free — R = app(var(1), var(2))
    const expr* result = solve_for("shift(app(var(zero), var(suc(zero))), zero, R)", "R");
    EXPECT_EQ(result, ap(dv(1), dv(2)));
}

// ============================================================
// Substitution
// ============================================================

TEST_F(LambdaCalculusTest, SubstVarAtDepthReplacedByArg) {
    // subst(var(0), 0, abs(var(0)), R): depth matches — R = shift_n(abs(var(0)), 0) = abs(var(0))
    const expr* result = solve_for("subst(var(zero), zero, abs(var(zero)), R)", "R");
    EXPECT_EQ(result, lm(dv(0)));
}

TEST_F(LambdaCalculusTest, SubstFreeVarAboveDepthDecremented) {
    // subst(var(1), 0, var(zero), R): var(1) > depth 0 — pred(1)=0, R = var(0)
    const expr* result = solve_for("subst(var(suc(zero)), zero, var(zero), R)", "R");
    EXPECT_EQ(result, dv(0));
}

TEST_F(LambdaCalculusTest, SubstBoundVarBelowDepthUnchanged) {
    // subst(var(0), 1, var(zero), R): var(0) < depth 1 — bound, R = var(0)
    const expr* result = solve_for("subst(var(zero), suc(zero), var(zero), R)", "R");
    EXPECT_EQ(result, dv(0));
}

TEST_F(LambdaCalculusTest, SubstUnderAbsIncrementsDepth) {
    // subst(abs(var(0)), 0, var(zero), R):
    //   descend: subst(var(0), 1, var(zero), R2) — var(0) < 1 — R2 = var(0)
    //   result: R = abs(var(0))
    const expr* result = solve_for("subst(abs(var(zero)), zero, var(zero), R)", "R");
    EXPECT_EQ(result, lm(dv(0)));
}

TEST_F(LambdaCalculusTest, SubstInAppRecursesBothSides) {
    // subst(app(var(0), var(1)), 0, var(zero), R):
    //   left:  subst(var(0), 0, var(zero), L) — depth matches — L = var(0)
    //   right: subst(var(1), 0, var(zero), Rr) — 1 > 0, decrement — Rr = var(0)
    //   R = app(var(0), var(0))
    const expr* result = solve_for("subst(app(var(zero), var(suc(zero))), zero, var(zero), R)", "R");
    EXPECT_EQ(result, ap(dv(0), dv(0)));
}

TEST_F(LambdaCalculusTest, SubstArgShiftedByDepth) {
    // subst(var(1), 1, var(zero), R):
    //   depth matches (D=1) — R = shift_n(var(0), 1) = var(1)
    const expr* result = solve_for("subst(var(suc(zero)), suc(zero), var(zero), R)", "R");
    EXPECT_EQ(result, dv(1));
}

// ============================================================
// Reduce (single beta step)
// ============================================================

TEST_F(LambdaCalculusTest, ReduceIdentityAppliedToVar) {
    // (λ.0) var(0) → var(0)
    const expr* result = solve_for(
        "reduce(app(abs(var(zero)), var(zero)), R)", "R");
    EXPECT_EQ(result, dv(0));
}

TEST_F(LambdaCalculusTest, ReduceIdentityAppliedToIdentity) {
    // (λ.0)(λ.0) → λ.0
    const expr* result = solve_for(
        "reduce(app(abs(var(zero)), abs(var(zero))), R)", "R");
    EXPECT_EQ(result, id_term());
}

TEST_F(LambdaCalculusTest, ReduceInsideRhs) {
    // app(var(0), app(abs(var(0)), var(0))) — rhs is the redex
    // → app(var(0), var(0))
    const expr* result = solve_for(
        "reduce(app(var(zero), app(abs(var(zero)), var(zero))), R)", "R");
    EXPECT_EQ(result, ap(dv(0), dv(0)));
}

TEST_F(LambdaCalculusTest, ReduceKAppliedToTwoArgs_FirstStep) {
    // K = abs(abs(var(1))); first reduction: (K A) consumes outer abs
    // app(abs(abs(var(suc(zero)))), abs(var(zero))) → abs(abs(var(zero)))
    const expr* result = solve_for(
        "reduce(app(abs(abs(var(suc(zero)))), abs(var(zero))), R)", "R");
    EXPECT_EQ(result, lm(lm(dv(0))));
}

TEST_F(LambdaCalculusTest, ReduceUnderAbs) {
    // abs(app(abs(var(zero)), var(zero))) — redex is inside the body
    // → abs(var(zero))
    const expr* result = solve_for(
        "reduce(abs(app(abs(var(zero)), var(zero))), R)", "R");
    EXPECT_EQ(result, lm(dv(0)));
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
    // (λx.xx)(id) = id id = id
    // abs(app(var(0), var(0))) applied to id:
    // step 1: subst(app(var(0),var(0)), 0, id) = app(id, id)
    // step 2: app(id, id) → id
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
//
// Each call exercises: two outer beta steps (succ applied to its
// argument then to f), plus one inner beta step where n is applied
// to f and x.  The subst and shift predicates recurse into the
// body of succ and the body of c_n.
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

// succ(succ(c0)) = c2: the outer succ must reduce before the inner
// result is itself in normal form, so normalize recurses twice
// through the full succ reduction sequence.
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
//
// times c2 c2 reduces as: λf. c2 (c2 f).
// c2 f → λx.f(f x), then c2 applies that function twice, giving
// λx.f(f(f(f x))) = c4.  This is the longest single-term reduction
// in the suite and stresses subst/shift recursion depth.
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

} // namespace
