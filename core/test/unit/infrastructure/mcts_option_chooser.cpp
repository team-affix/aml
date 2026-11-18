#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_option_chooser.hpp"
#include "infrastructure/option_range.hpp"

using ::testing::NiceMock;
using ::testing::Property;
using ::testing::Return;

namespace {

struct MockChooseChoice {
    MOCK_METHOD(mcts_choice, choose, (const option_range&, const option_range&));
};

struct MctsOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseChoice> choose_choice;
    mcts_option_chooser<NiceMock<MockChooseChoice>> chooser{choose_choice};
};

}

TEST_F(MctsOptionChooserTest, MctsChoosesAmongTheOptionsIndices) {
    EXPECT_CALL(choose_choice, choose(Property(&option_range::size, 3u), Property(&option_range::size, 3u)))
        .WillOnce(Return(2));
    EXPECT_EQ(chooser.choose(3), 2u);
}

TEST_F(MctsOptionChooserTest, ASingleOptionIsStillOfferedToMcts) {
    EXPECT_CALL(choose_choice, choose(Property(&option_range::size, 1u), Property(&option_range::size, 1u)))
        .WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(1), 0u);
}
