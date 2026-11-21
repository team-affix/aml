#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mcts_option_chooser.hpp"
#include "infrastructure/option_range.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Property;
using ::testing::Return;

namespace {

struct MockChooseChoice {
    MOCK_METHOD(mcts_choice, choose, (const option_range&, const option_range&));
};

mcts_choice same_options_chosen_last(const option_range& counts, const option_range& choices) {
    EXPECT_EQ(counts.size(), choices.size());
    for (size_t position = 0; position < choices.size(); ++position)
        EXPECT_EQ(choices.at(position), counts.at(position));
    return choices.at(choices.size() - 1);
}

struct MctsOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseChoice> choose_choice;
    mcts_option_chooser<NiceMock<MockChooseChoice>> chooser{choose_choice};
};

}

TEST_F(MctsOptionChooserTest, MctsChoosesAmongTheOptionsIndices) {
    EXPECT_CALL(choose_choice, choose(_, _)).Times(AnyNumber());
    EXPECT_CALL(choose_choice, choose(Property(&option_range::size, 3u), Property(&option_range::size, 3u)))
        .WillOnce(Return(2));
    EXPECT_EQ(chooser.choose(3), 2u);
}

TEST_F(MctsOptionChooserTest, ASingleOptionIsStillOfferedToMcts) {
    EXPECT_CALL(choose_choice, choose(_, _)).Times(AnyNumber());
    EXPECT_CALL(choose_choice, choose(Property(&option_range::size, 1u), Property(&option_range::size, 1u)))
        .WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(1), 0u);
}

TEST_F(MctsOptionChooserTest, BothRangesOfferTheSameOptions) {
    EXPECT_CALL(choose_choice, choose(_, _)).WillOnce(::testing::Invoke(&same_options_chosen_last));
    EXPECT_EQ(chooser.choose(4), 3u);
}

TEST_F(MctsOptionChooserTest, EveryChoiceIsAskedOfMcts) {
    EXPECT_CALL(choose_choice, choose(_, _)).Times(3).WillRepeatedly(Return(0));
    chooser.choose(2);
    chooser.choose(5);
    chooser.choose(1);
}
