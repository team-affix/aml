#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/scrutinee_piece_finder.hpp"

using namespace AML;

namespace {

struct ScrutineePieceFinderTest : public ::testing::Test {
    ScrutineePieceFinderTest();
    static feature app(prim_id prim, std::vector<feature> args);
    feature x0;
    feature x1;
    feature literal;
    scrutinee_piece_finder finder;
};

ScrutineePieceFinderTest::ScrutineePieceFinderTest()
    : x0{feature::var{.index = 0}}
    , x1{feature::var{.index = 1}}
    , literal{feature::constant{.literal = value{.ctor = 0, .args = {}}}}
    , finder() {}

feature ScrutineePieceFinderTest::app(prim_id prim, std::vector<feature> args) {
    return feature{feature::app{.prim = prim, .args = std::move(args)}};
}

}

TEST_F(ScrutineePieceFinderTest, AVariableHasNoPieces) {
    EXPECT_TRUE(finder.find(x0).empty());
}

TEST_F(ScrutineePieceFinderTest, AConstantHasNoPieces) {
    EXPECT_TRUE(finder.find(literal).empty());
}

TEST_F(ScrutineePieceFinderTest, AnApplicationOfVariablesIsItsOnlyPiece) {
    const feature sum = app(0, {x0, x1});
    EXPECT_EQ(finder.find(sum), (std::vector<feature>{sum}));
}

TEST_F(ScrutineePieceFinderTest, PiecesComeInPreorderLeftToRight) {
    const feature left = app(1, {x0});
    const feature right = app(2, {x1});
    const feature inner = app(3, {right});
    const feature whole = app(0, {left, inner});
    EXPECT_EQ(finder.find(whole), (std::vector<feature>{whole, left, inner, right}));
}

TEST_F(ScrutineePieceFinderTest, ARepeatedApplicationIsAPieceOnce) {
    const feature twice = app(1, {x0});
    const feature whole = app(0, {twice, twice});
    EXPECT_EQ(finder.find(whole), (std::vector<feature>{whole, twice}));
}

TEST_F(ScrutineePieceFinderTest, VariablesAndConstantsInsideAreNotPieces) {
    const feature whole = app(0, {x0, literal});
    EXPECT_EQ(finder.find(whole), (std::vector<feature>{whole}));
}
