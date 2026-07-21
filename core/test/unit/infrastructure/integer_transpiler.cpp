#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/integer_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockMakeLcApp         { MOCK_METHOD(const lc_expr*, make_app, (const lc_expr*, const lc_expr*)); };
struct MockTranspileNat      { MOCK_METHOD(const lc_expr*, transpile_nat, (const aml_expr::nat&)); };
struct MockTranspilePos      { MOCK_METHOD(const lc_expr*, transpile_pos, ()); };
struct MockTranspileNegsuc   { MOCK_METHOD(const lc_expr*, transpile_negsuc, ()); };

// Standard bindings: pos=0, negsuc=1
struct IntegerTranspilerTest : public ::testing::Test {
    lc_expr_pool    lc;
    testing::NiceMock<MockMakeLcApp>       mock_app;
    testing::NiceMock<MockTranspileNat>    mock_nat;
    testing::NiceMock<MockTranspilePos>    mock_pos;
    testing::NiceMock<MockTranspileNegsuc> mock_negsuc;
    integer_transpiler<testing::NiceMock<MockMakeLcApp>,
                       testing::NiceMock<MockTranspileNat>,
                       testing::NiceMock<MockTranspilePos>,
                       testing::NiceMock<MockTranspileNegsuc>>
        it{mock_app, mock_nat, mock_pos, mock_negsuc};

    void SetUp() override {
        using testing::_;
        using testing::Return;
        ON_CALL(mock_app, make_app(_, _)).WillByDefault([this](const lc_expr* f, const lc_expr* a) {
            return lc.make_app(f, a);
        });
        ON_CALL(mock_pos, transpile_pos()).WillByDefault(Return(lc.make_var(0)));
        ON_CALL(mock_negsuc, transpile_negsuc()).WillByDefault(Return(lc.make_var(1)));
        ON_CALL(mock_nat, transpile_nat(_)).WillByDefault([this](const aml_expr::nat&) {
            return lc.make_var(99);
        });
    }
};

} // namespace

TEST_F(IntegerTranspilerTest, ZeroUsesPos) {
    using testing::_;
    const lc_expr* nat_result = lc.make_var(0);
    EXPECT_CALL(mock_nat, transpile_nat(_)).WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({0, nat_format::binary}),
              lc.make_app(lc.make_var(0), nat_result));
}

TEST_F(IntegerTranspilerTest, PositiveUsesPos) {
    using testing::_;
    const lc_expr* nat_result = lc.make_var(5);
    EXPECT_CALL(mock_nat, transpile_nat(_)).WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({12, nat_format::binary}),
              lc.make_app(lc.make_var(0), nat_result));
}

TEST_F(IntegerTranspilerTest, NegativeOneUsesNegsucOfZero) {
    using testing::_;
    const lc_expr* nat_result = lc.make_var(0);

    EXPECT_CALL(mock_nat, transpile_nat(testing::Field(&aml_expr::nat::value, 0u)))
        .WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({-1, nat_format::binary}),
              lc.make_app(lc.make_var(1), nat_result));
}

TEST_F(IntegerTranspilerTest, NegativeTwoUsesNegsucOfOne) {
    using testing::_;
    const lc_expr* nat_result = lc.make_var(1);

    EXPECT_CALL(mock_nat, transpile_nat(testing::Field(&aml_expr::nat::value, 1u)))
        .WillOnce(testing::Return(nat_result));
    EXPECT_EQ(it.transpile_integer({-2, nat_format::binary}),
              lc.make_app(lc.make_var(1), nat_result));
}

TEST_F(IntegerTranspilerTest, ForwardsBinaryFormatToTranspileNat) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::AllOf(
            testing::Field(&aml_expr::nat::value, 3u),
            testing::Field(&aml_expr::nat::format, nat_format::binary))))
        .Times(1);
    it.transpile_integer({3, nat_format::binary});
}

TEST_F(IntegerTranspilerTest, ForwardsChurchFormatToTranspileNat) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::AllOf(
            testing::Field(&aml_expr::nat::value, 5u),
            testing::Field(&aml_expr::nat::format, nat_format::church))))
        .Times(1);
    it.transpile_integer({5, nat_format::church});
}

TEST_F(IntegerTranspilerTest, ForwardsChurchFormatToTranspileNatForNegative) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::AllOf(
            testing::Field(&aml_expr::nat::value, 2u),
            testing::Field(&aml_expr::nat::format, nat_format::church))))
        .Times(1);
    it.transpile_integer({-3, nat_format::church});
}
