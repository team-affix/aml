#include <gtest/gtest.h>
#include "infrastructure/cheapest_feature_index.hpp"

using namespace AML;

namespace {

struct CheapestFeatureIndexTest : public ::testing::Test {
    CheapestFeatureIndexTest();
    candidate make(const feature& expression, size_t cost, const column& cells);
    candidate make_typed(const feature& expression, type_id type, const column& cells);
    value falsity;
    value truth;
    feature first;
    feature second;
    cheapest_feature_index index;
};

CheapestFeatureIndexTest::CheapestFeatureIndexTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , first{feature::var{.index = 0}}
    , second{feature::var{.index = 1}}
    , index() {}

candidate CheapestFeatureIndexTest::make(const feature& expression, size_t cost, const column& cells) {
    return candidate{
        .expression = expression, .cost = cost, .observed = column_view::of(typed_column{.type = 0, .cells = cells})};
}

candidate CheapestFeatureIndexTest::make_typed(const feature& expression, type_id type, const column& cells) {
    return candidate{
        .expression = expression, .cost = 1, .observed = column_view::of(typed_column{.type = type, .cells = cells})};
}

}

TEST_F(CheapestFeatureIndexTest, CandidatesWithDifferentColumnsAreAllKeptInOrder) {
    index.try_insert(make(first, 1, {falsity, truth}));
    index.try_insert(make(second, 1, {truth, falsity}));
    EXPECT_EQ(index.candidates(),
              (std::vector<candidate>{make(first, 1, {falsity, truth}), make(second, 1, {truth, falsity})}));
}

TEST_F(CheapestFeatureIndexTest, ANoCheaperCandidateForAHeldColumnIsDropped) {
    index.try_insert(make(first, 2, {falsity, truth}));
    index.try_insert(make(second, 2, {falsity, truth}));
    EXPECT_EQ(index.candidates(), (std::vector<candidate>{make(first, 2, {falsity, truth})}));
}

TEST_F(CheapestFeatureIndexTest, ACheaperCandidateForAHeldColumnReplacesItInPlace) {
    index.try_insert(make(first, 3, {falsity, truth}));
    index.try_insert(make(first, 3, {truth, truth}));
    index.try_insert(make(second, 1, {falsity, truth}));
    EXPECT_EQ(index.candidates(),
              (std::vector<candidate>{make(second, 1, {falsity, truth}), make(first, 3, {truth, truth})}));
}

TEST_F(CheapestFeatureIndexTest, ClearForgetsEveryCandidate) {
    index.try_insert(make(first, 3, {falsity, truth}));
    index.clear();
    index.try_insert(make(second, 5, {falsity, truth}));
    EXPECT_EQ(index.candidates(), (std::vector<candidate>{make(second, 5, {falsity, truth})}));
}

TEST_F(CheapestFeatureIndexTest, EachTypeListsItsCandidatesPositionsInOrder) {
    index.try_insert(make_typed(first, 2, {falsity, truth}));
    index.try_insert(make_typed(second, 0, {truth, falsity}));
    index.try_insert(make_typed(first, 2, {truth, truth}));
    EXPECT_EQ(index.features_of_type(0), (std::vector<size_t>{1}));
    EXPECT_EQ(index.features_of_type(2), (std::vector<size_t>{0, 2}));
    EXPECT_TRUE(index.features_of_type(1).empty());
}

TEST_F(CheapestFeatureIndexTest, AReplacementKeepsItsPositionUnderItsType) {
    index.try_insert(make(first, 3, {falsity, truth}));
    index.try_insert(make(second, 1, {falsity, truth}));
    EXPECT_EQ(index.features_of_type(0), (std::vector<size_t>{0}));
}

TEST_F(CheapestFeatureIndexTest, ClearForgetsEveryTypesPositions) {
    index.try_insert(make(first, 3, {falsity, truth}));
    index.clear();
    EXPECT_TRUE(index.features_of_type(0).empty());
}

TEST_F(CheapestFeatureIndexTest, ANewIndexIsEmpty) {
    EXPECT_TRUE(index.candidates().empty());
    EXPECT_TRUE(index.features_of_type(0).empty());
}

TEST_F(CheapestFeatureIndexTest, ColumnsOfDifferentLengthAreDifferent) {
    index.try_insert(make(first, 1, {falsity}));
    index.try_insert(make(second, 1, {falsity, falsity}));
    EXPECT_EQ(index.candidates().size(), 2u);
}

TEST_F(CheapestFeatureIndexTest, AReplacementDoesNotAddAPosition) {
    index.try_insert(make(first, 5, {falsity, truth}));
    index.try_insert(make(second, 2, {falsity, truth}));
    index.try_insert(make(first, 1, {falsity, truth}));
    EXPECT_EQ(index.candidates().size(), 1u);
    EXPECT_EQ(index.features_of_type(0).size(), 1u);
    EXPECT_EQ(index.candidates()[0].cost, 1u);
}

TEST_F(CheapestFeatureIndexTest, IdenticalCellsOfDifferentTypesAreDifferentColumns) {
    index.try_insert(make_typed(first, 0, {falsity, truth}));
    index.try_insert(make_typed(second, 1, {falsity, truth}));
    EXPECT_EQ(index.candidates().size(), 2u);
}
