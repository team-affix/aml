#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_candidate_chooser.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockChooseChoice {
    MOCK_METHOD(mcts_choice, choose, (const std::vector<mcts_choice>&, const std::vector<mcts_choice>&));
};

struct MctsCandidateChooserTest : public ::testing::Test {
    MctsCandidateChooserTest();
    candidate any;
    NiceMock<MockChooseChoice> choose_choice;
    mcts_candidate_chooser<NiceMock<MockChooseChoice>> chooser{choose_choice};
};

MctsCandidateChooserTest::MctsCandidateChooserTest()
    : any{.expression = nullptr, .cost = 1, .observed = typed_column{.type = 0, .cells = {}}}
    , choose_choice() {}

}

TEST_F(MctsCandidateChooserTest, MctsChoosesAmongTheCandidatesIndices) {
    const std::vector<mcts_choice> indices{0, 1, 2};
    EXPECT_CALL(choose_choice, choose(indices, indices)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({any, any, any}), 2u);
}
