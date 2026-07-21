#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/lc_expr_pool.hpp"
#include "infrastructure/string_transpiler.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockTranspileNat  { MOCK_METHOD(const lc_expr*, transpile_nat, (const aml_expr::nat&)); };
struct MockMakeLcApp     { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };
struct MockTranspileNil  { MOCK_METHOD(const lc_expr*, transpile_nil, ()); };
struct MockTranspileCons { MOCK_METHOD(const lc_expr*, transpile_cons, ()); };

// Standard bindings: nil=0, cons=1
struct StringTranspilerTest : public ::testing::Test {
    lc_expr_pool     lc;
    testing::NiceMock<MockTranspileNat>  mock_nat;
    testing::NiceMock<MockMakeLcApp>     mock_app;
    testing::NiceMock<MockTranspileNil>  mock_nil;
    testing::NiceMock<MockTranspileCons> mock_cons;
    string_transpiler<testing::NiceMock<MockTranspileNat>,
                      testing::NiceMock<MockMakeLcApp>,
                      testing::NiceMock<MockTranspileNil>,
                      testing::NiceMock<MockTranspileCons>>
        st{mock_nat, mock_app, mock_nil, mock_cons};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_nat, transpile_nat(_)).WillByDefault([this](const aml_expr::nat& n) {
            return lc.make_var(static_cast<uint32_t>(n.value));
        });
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_nil, transpile_nil()).WillByDefault(Return(lc.make_var(0)));
        ON_CALL(mock_cons, transpile_cons()).WillByDefault(Return(lc.make_var(1)));
    }

    const lc_expr* v(uint32_t i)                           { return lc.make_var(i); }
    const lc_expr* ap(const lc_expr* f, const lc_expr* a) { return lc.make_app(f, a); }
    const lc_expr* cons_(const lc_expr* hd, const lc_expr* tl) {
        return ap(ap(v(1), hd), tl);
    }
};

} // namespace

TEST_F(StringTranspilerTest, EmptyStringReturnsNil) {
    EXPECT_EQ(st.transpile_string(aml_expr::string{""}), v(0));
}

TEST_F(StringTranspilerTest, SingleCharIsConsByteNil) {
    EXPECT_EQ(st.transpile_string(aml_expr::string{"A"}),
              cons_(v(65), v(0)));
}

TEST_F(StringTranspilerTest, TwoCharactersInOrder) {
    EXPECT_EQ(st.transpile_string(aml_expr::string{"ab"}),
              cons_(v(97), cons_(v(98), v(0))));
}

TEST_F(StringTranspilerTest, OrderIsPreservedNotReversed) {
    const lc_expr* result = st.transpile_string(aml_expr::string{"hi"});
    const auto* outer_app = std::get_if<lc_expr::app>(&result->content);
    ASSERT_NE(outer_app, nullptr);
    const auto* inner_app = std::get_if<lc_expr::app>(&outer_app->func->content);
    ASSERT_NE(inner_app, nullptr);
    EXPECT_EQ(inner_app->arg, v(104));
}

TEST_F(StringTranspilerTest, EachCharTranspilesThroughTranspileNatWithBinaryFormat) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::Field(&aml_expr::nat::format, nat_format::binary)))
        .Times(3);
    st.transpile_string(aml_expr::string{"xyz"});
}
