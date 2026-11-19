#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/output_finder.hpp"

namespace {

struct OutputFinderTest : public ::testing::Test {
    OutputFinderTest();
    candidate make(const feature* expression, size_t cost, const column& cells);
    value falsity;
    value truth;
    feature first;
    feature second;
    feature third;
    typed_column labels;
    output_finder finder;
};

OutputFinderTest::OutputFinderTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , first{feature::var{.index = 0}}
    , second{feature::var{.index = 1}}
    , third{feature::var{.index = 2}}
    , labels{.type = 0, .cells = {&falsity, &truth}}
    , finder() {}

candidate OutputFinderTest::make(const feature* expression, size_t cost, const column& cells) {
    return candidate{.expression = expression, .cost = cost, .observed = typed_column{.type = 0, .cells = cells}};
}

}

TEST_F(OutputFinderTest, NoCandidatesGiveNoOutput) {
    EXPECT_FALSE(finder.find({}, labels).has_value());
}

TEST_F(OutputFinderTest, ACandidateWhoseColumnIsTheLabelsIsTheOutput) {
    EXPECT_EQ(finder.find({make(&first, 1, {&truth, &falsity}), make(&second, 3, labels.cells)}, labels),
              std::optional<const feature*>(&second));
}

TEST_F(OutputFinderTest, CandidatesThatDifferFromTheLabelsGiveNoOutput) {
    EXPECT_FALSE(finder.find({make(&first, 1, {&truth, &falsity}), make(&second, 1, {&truth, &truth})}, labels)
                     .has_value());
}

TEST_F(OutputFinderTest, TheCheapestMatchingCandidateWins) {
    const std::vector<candidate> matching{
        make(&first, 5, labels.cells), make(&second, 2, labels.cells), make(&third, 4, labels.cells)};
    EXPECT_EQ(finder.find(matching, labels),
              std::optional<const feature*>(&second));
}

TEST_F(OutputFinderTest, TheFirstCandidateWinsATie) {
    EXPECT_EQ(finder.find({make(&first, 2, labels.cells), make(&second, 2, labels.cells)}, labels),
              std::optional<const feature*>(&first));
}

TEST_F(OutputFinderTest, AColumnOfAnotherTypeIsNotTheOutputEvenWithTheSameCells) {
    const candidate other_type{
        .expression = &first, .cost = 1, .observed = typed_column{.type = 3, .cells = labels.cells}};
    EXPECT_FALSE(finder.find({other_type}, labels).has_value());
}
