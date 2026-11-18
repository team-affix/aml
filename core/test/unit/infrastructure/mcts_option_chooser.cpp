#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_option_chooser.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockChooseChoice {
    MOCK_METHOD(mcts_choice, choose, (const std::vector<mcts_choice>&, const std::vector<mcts_choice>&));
};

struct MctsOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseChoice> choose_choice;
    mcts_option_chooser<NiceMock<MockChooseChoice>> chooser{choose_choice};
};

}

TEST_F(MctsOptionChooserTest, MctsChoosesAmongTheOptionsIndices) {
    const std::vector<mcts_choice> indices{0, 1, 2};
    EXPECT_CALL(choose_choice, choose(indices, indices)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose(3), 2u);
}

TEST_F(MctsOptionChooserTest, ASingleOptionIsStillOfferedToMcts) {
    const std::vector<mcts_choice> indices{0};
    EXPECT_CALL(choose_choice, choose(indices, indices)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(1), 0u);
}
