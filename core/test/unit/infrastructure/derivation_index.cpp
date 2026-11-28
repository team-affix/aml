#include <optional>
#include <gtest/gtest.h>
#include "infrastructure/derivation_index.hpp"

using namespace AML;

namespace {

std::vector<operand> feature_args(const std::vector<feature_id>& ids) {
    std::vector<operand> args;
    for (const feature_id id : ids)
        args.push_back(operand{operand::from_feature{.id = id}});
    return args;
}

struct DerivationIndexTest : public ::testing::Test {
    DerivationIndexTest();
    derivation sum;
    derivation swapped;
    derivation_index index;
};

DerivationIndexTest::DerivationIndexTest()
    : sum{derivation::application{.prim = 0, .args = feature_args({1, 2})}}
    , swapped{derivation::application{.prim = 0, .args = feature_args({2, 1})}}
    , index() {}

}

TEST_F(DerivationIndexTest, StartsEmpty) {
    EXPECT_EQ(index.find(sum), std::nullopt);
}

TEST_F(DerivationIndexTest, AnInsertedDerivationFindsItsFeature) {
    index.insert(sum, 7);
    EXPECT_EQ(index.find(sum), std::optional<feature_id>(7));
}

TEST_F(DerivationIndexTest, AnotherDerivationIsNotFound) {
    index.insert(sum, 7);
    EXPECT_EQ(index.find(swapped), std::nullopt);
}

TEST_F(DerivationIndexTest, TwoDerivationsMayFindOneFeature) {
    index.insert(sum, 7);
    index.insert(swapped, 7);
    EXPECT_EQ(index.find(swapped), std::optional<feature_id>(7));
}

TEST_F(DerivationIndexTest, ReinsertingADerivationFindsTheLatestFeature) {
    index.insert(sum, 7);
    index.insert(sum, 9);
    EXPECT_EQ(index.find(sum), std::optional<feature_id>(9));
}
