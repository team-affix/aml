#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/character_transpiler.hpp"
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/nat_format.hpp"

namespace {

struct MockTranspileNat {
    MOCK_METHOD(const lc_expr*, transpile_nat, (const aml_expr::nat&));
};

struct CharacterTranspilerTest : public ::testing::Test {
    lc_expr_pool    lc;
    testing::NiceMock<MockTranspileNat> mock_nat;
    character_transpiler<testing::NiceMock<MockTranspileNat>> ct{mock_nat};

    void SetUp() override {
        using testing::_;
        ON_CALL(mock_nat, transpile_nat(_)).WillByDefault([this](const aml_expr::nat& n) {
            return lc.make_var(static_cast<uint32_t>(n.value));
        });
    }
};

} // namespace

// The observable contract: character_transpiler converts a char to its
// unsigned byte value and delegates to transpile_nat with nat_format::binary.

TEST_F(CharacterTranspilerTest, NulCharacterYieldsNatValue0WithBinaryFormat) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::AllOf(
            testing::Field(&aml_expr::nat::value, 0u),
            testing::Field(&aml_expr::nat::format, nat_format::binary))))
        .Times(1);
    ct.transpile_character(aml_expr::character{'\0'});
}

TEST_F(CharacterTranspilerTest, UppercaseAYieldsNatValue65) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::Field(&aml_expr::nat::value, 65u)))
        .Times(1);
    ct.transpile_character(aml_expr::character{'A'});
}

TEST_F(CharacterTranspilerTest, AlwaysUsesBinaryFormat) {
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::Field(&aml_expr::nat::format, nat_format::binary)))
        .Times(1);
    ct.transpile_character(aml_expr::character{'z'});
}

TEST_F(CharacterTranspilerTest, HighByteCharTreatedAsUnsigned) {
    // '\x80' must become 128, not -128 (signed char overflow guard)
    EXPECT_CALL(mock_nat, transpile_nat(
        testing::Field(&aml_expr::nat::value, 128u)))
        .Times(1);
    ct.transpile_character(aml_expr::character{'\x80'});
}

TEST_F(CharacterTranspilerTest, ReturnsTranspileNatResult) {
    using testing::Return;
    using testing::_;
    const lc_expr* sentinel = lc.make_var(42);
    ON_CALL(mock_nat, transpile_nat(_)).WillByDefault(Return(sentinel));
    EXPECT_EQ(ct.transpile_character(aml_expr::character{'x'}), sentinel);
}
