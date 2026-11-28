#include <gtest/gtest.h>
#include "value_objects/derivation_hash.hpp"

using namespace AML;

namespace {

struct DerivationHashTest : public ::testing::Test {
    derivation application(prim_id prim, std::vector<feature_id> args);
    derivation_hash hasher;
};

derivation DerivationHashTest::application(prim_id prim, std::vector<feature_id> args) {
    std::vector<operand> operands;
    for (const feature_id id : args)
        operands.push_back(operand{operand::from_feature{.id = id}});
    return derivation{derivation::application{.prim = prim, .args = operands}};
}

}

TEST_F(DerivationHashTest, EqualApplicationsHashEqual) {
    EXPECT_EQ(hasher(application(2, {0, 1})), hasher(application(2, {0, 1})));
}

TEST_F(DerivationHashTest, ArgumentOrderChangesTheHash) {
    EXPECT_NE(hasher(application(2, {0, 1})), hasher(application(2, {1, 0})));
}

TEST_F(DerivationHashTest, ADifferentPrimitiveChangesTheHash) {
    EXPECT_NE(hasher(application(2, {0, 1})), hasher(application(3, {0, 1})));
}

TEST_F(DerivationHashTest, AnExtraArgumentChangesTheHash) {
    EXPECT_NE(hasher(application(2, {0})), hasher(application(2, {0, 0})));
}

TEST_F(DerivationHashTest, InputsHashByIndex) {
    EXPECT_EQ(hasher(derivation{derivation::input{.index = 1}}), hasher(derivation{derivation::input{.index = 1}}));
    EXPECT_NE(hasher(derivation{derivation::input{.index = 1}}), hasher(derivation{derivation::input{.index = 2}}));
}

TEST_F(DerivationHashTest, AConstantOperandHashesApartFromAFeatureOperandWithItsId) {
    const derivation with_constant{derivation::application{
        .prim = 2, .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_constant{.id = 1}}}}};
    EXPECT_NE(hasher(with_constant), hasher(application(2, {0, 1})));
    EXPECT_EQ(hasher(with_constant), hasher(with_constant));
}

TEST_F(DerivationHashTest, TheKindChangesTheHash) {
    EXPECT_NE(hasher(derivation{derivation::input{.index = 0}}), hasher(application(0, {})));
}
