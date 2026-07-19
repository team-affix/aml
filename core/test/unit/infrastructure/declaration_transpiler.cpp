#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/declaration_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/declaration_decl.hpp"
#include "value_objects/declaration_group.hpp"

namespace {

struct MockMakeLcVar { MOCK_METHOD(const lc_expr*, make_var, (uint32_t)); };
struct MockMakeLcAbs { MOCK_METHOD(const lc_expr*, make_abs, (const lc_expr*)); };
struct MockMakeLcApp { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };

struct DeclarationTranspilerTest : public ::testing::Test {
    lc_expr_pool  lc;
    testing::NiceMock<MockMakeLcVar> mock_var;
    testing::NiceMock<MockMakeLcAbs> mock_abs;
    testing::NiceMock<MockMakeLcApp> mock_app;
    declaration_transpiler<testing::NiceMock<MockMakeLcVar>,
                           testing::NiceMock<MockMakeLcAbs>,
                           testing::NiceMock<MockMakeLcApp>> dt{mock_var, mock_abs, mock_app};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_var, make_var(_)).WillByDefault([this](uint32_t i) {
            return lc.make_var(i);
        });
        ON_CALL(mock_abs, make_abs(_)).WillByDefault([this](const lc_expr* b) {
            return lc.make_abs(b);
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
    }

    const lc_expr* v(uint32_t i)                           { return lc.make_var(i); }
    const lc_expr* ab(const lc_expr* b)                    { return lc.make_abs(b); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a)  { return lc.make_app(f, a); }

    static declaration_group make_group(std::initializer_list<declaration_decl> decls) {
        declaration_group g;
        g.declarations = decls;
        return g;
    }
};

} // namespace

TEST_F(DeclarationTranspilerTest, SingleVariantArity0) {
    // n=1, k=0, a=0: abs(var(0))
    auto terms = dt.transpile_group(make_group({{"only", 0u}}));
    ASSERT_EQ(terms.size(), 1u);
    EXPECT_EQ(terms[0], ab(v(0)));
}

TEST_F(DeclarationTranspilerTest, BooleanEncoding) {
    // true/0 | false/0  (n=2)
    // true  (k=0, a=0): abs(abs(var(1)))
    // false (k=1, a=0): abs(abs(var(0)))
    auto terms = dt.transpile_group(make_group({{"true", 0u}, {"false", 0u}}));
    ASSERT_EQ(terms.size(), 2u);
    EXPECT_EQ(terms[0], ab(ab(v(1))));
    EXPECT_EQ(terms[1], ab(ab(v(0))));
}

TEST_F(DeclarationTranspilerTest, ThreeVariantGroupFromPlanExample) {
    // abc/0 | def/0 | ghi/1  (n=3)
    // abc (k=0, a=0): abs(abs(abs(var(2))))
    // def (k=1, a=0): abs(abs(abs(var(1))))
    // ghi (k=2, a=1): abs(abs(abs(abs(app(var(1),var(0))))))
    auto terms = dt.transpile_group(make_group({{"abc", 0u}, {"def", 0u}, {"ghi", 1u}}));
    ASSERT_EQ(terms.size(), 3u);
    EXPECT_EQ(terms[0], ab(ab(ab(v(2)))));
    EXPECT_EQ(terms[1], ab(ab(ab(v(1)))));
    EXPECT_EQ(terms[2], ab(ab(ab(ab(ap(v(1), v(0)))))));
}

TEST_F(DeclarationTranspilerTest, ListEncoding) {
    // nil/0 | cons/2  (n=2)
    // nil  (k=0, a=0): abs(abs(var(1)))
    // cons (k=1, a=2): abs(abs(abs(abs(app(app(var(2),var(1)),var(0))))))
    auto terms = dt.transpile_group(make_group({{"nil", 0u}, {"cons", 2u}}));
    ASSERT_EQ(terms.size(), 2u);
    EXPECT_EQ(terms[0], ab(ab(v(1))));
    EXPECT_EQ(terms[1], ab(ab(ab(ab(ap(ap(v(2), v(1)), v(0)))))));
}

TEST_F(DeclarationTranspilerTest, HighAritySingleVariant) {
    // only/3  (n=1, k=0, a=3)
    // elim_idx = 3+(1-1-0) = 3; body = app(app(app(var(3),var(2)),var(1)),var(0))
    // wrapped in 4 abs
    auto terms = dt.transpile_group(make_group({{"only", 3u}}));
    ASSERT_EQ(terms.size(), 1u);
    EXPECT_EQ(terms[0], ab(ab(ab(ab(ap(ap(ap(v(3), v(2)), v(1)), v(0)))))));
}
