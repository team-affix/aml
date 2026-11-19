#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/bound_argument_chooser.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindVariableCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&, var_id), (const));
};

struct MockChooseBoundArgument {
    MOCK_METHOD(size_t, choose, (size_t));
};

// A bin whose variables 1 and 2 were just bound by a match.
struct BoundArgumentChooserTest : public ::testing::Test {
    BoundArgumentChooserTest();
    value falsity;
    value truth;
    feature first_bound_argument;
    feature second_argument;
    bin data;
    candidate first_bound_argument_candidate;
    candidate second_argument_candidate;
    NiceMock<MockFindVariableCandidates> find_variable_candidates;
    NiceMock<MockChooseBoundArgument> choose_bound_argument;
    bound_argument_chooser<NiceMock<MockFindVariableCandidates>, NiceMock<MockChooseBoundArgument>> chooser{
        find_variable_candidates, choose_bound_argument};
};

BoundArgumentChooserTest::BoundArgumentChooserTest()
    : falsity{.tag = 0, .args = {}}
    , truth{.tag = 1, .args = {}}
    , first_bound_argument{feature::var{.index = 1}}
    , second_argument{feature::var{.index = 2}}
    , data{.inputs = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                     typed_column{.type = 0, .cells = {&truth, &falsity}},
                     typed_column{.type = 0, .cells = {&falsity, &truth}}},
           .labels = typed_column{.type = 0, .cells = {&falsity, &truth}}}
    , first_bound_argument_candidate{.expression = first_bound_argument, .cost = 1, .observed = data.inputs[1]}
    , second_argument_candidate{.expression = second_argument, .cost = 1, .observed = data.inputs[2]}
    , find_variable_candidates()
    , choose_bound_argument() {}

}

TEST_F(BoundArgumentChooserTest, TheChosenHeterogeneousArgumentFromTheFirstArgumentOnIsTheScrutinee) {
    EXPECT_CALL(find_variable_candidates, find(data, 1))
        .WillOnce(Return(std::vector<candidate>{first_bound_argument_candidate, second_argument_candidate}));
    EXPECT_CALL(choose_bound_argument, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(data, 1), second_argument_candidate);
}

TEST_F(BoundArgumentChooserTest, TheOnlyArgumentIsStillChosenThrough) {
    ON_CALL(find_variable_candidates, find(data, 2))
        .WillByDefault(Return(std::vector<candidate>{second_argument_candidate}));
    EXPECT_CALL(choose_bound_argument, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(data, 2), second_argument_candidate);
}

TEST_F(BoundArgumentChooserTest, ArgumentsAreFoundFromTheGivenFirstArgument) {
    EXPECT_CALL(find_variable_candidates, find(data, 2))
        .WillOnce(Return(std::vector<candidate>{second_argument_candidate}));
    ON_CALL(choose_bound_argument, choose(1)).WillByDefault(Return(0));
    chooser.choose(data, 2);
}
