#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/output_finder.hpp"

using namespace AML;

namespace {

struct OutputFinderTest : public ::testing::Test {
    OutputFinderTest();
    candidate make(const feature& expression, size_t cost, const column& cells);
    value falsity;
    value truth;
    feature first;
    feature second;
    feature third;
    column label_cells;
    column_view labels;
    output_finder finder;
};

OutputFinderTest::OutputFinderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , first{feature::var{.index = 0}}
    , second{feature::var{.index = 1}}
    , third{feature::var{.index = 2}}
    , label_cells{falsity, truth}
    , labels(column_view::of(typed_column{.type = 0, .cells = label_cells}))
    , finder() {}

candidate OutputFinderTest::make(const feature& expression, size_t cost, const column& cells) {
    return candidate{
        .expression = expression, .cost = cost, .observed = column_view::of(typed_column{.type = 0, .cells = cells})};
}

}

TEST_F(OutputFinderTest, NoCandidatesGiveNoOutput) {
    EXPECT_FALSE(finder.find({}, labels).has_value());
}

TEST_F(OutputFinderTest, ACandidateWhoseColumnIsTheLabelsIsTheOutput) {
    EXPECT_EQ(finder.find({make(first, 1, {truth, falsity}), make(second, 3, label_cells)}, labels),
              std::optional<feature>(second));
}

TEST_F(OutputFinderTest, CandidatesThatDifferFromTheLabelsGiveNoOutput) {
    EXPECT_FALSE(finder.find({make(first, 1, {truth, falsity}), make(second, 1, {truth, truth})}, labels)
                     .has_value());
}

TEST_F(OutputFinderTest, TheCheapestMatchingCandidateWins) {
    const std::vector<candidate> matching{
        make(first, 5, label_cells), make(second, 2, label_cells), make(third, 4, label_cells)};
    EXPECT_EQ(finder.find(matching, labels),
              std::optional<feature>(second));
}

TEST_F(OutputFinderTest, TheFirstCandidateWinsATie) {
    EXPECT_EQ(finder.find({make(first, 2, label_cells), make(second, 2, label_cells)}, labels),
              std::optional<feature>(first));
}

TEST_F(OutputFinderTest, AColumnOfAnotherTypeIsNotTheOutputEvenWithTheSameCells) {
    const candidate other_type{
        .expression = first, .cost = 1, .observed = column_view::of(typed_column{.type = 3, .cells = label_cells})};
    EXPECT_FALSE(finder.find({other_type}, labels).has_value());
}
