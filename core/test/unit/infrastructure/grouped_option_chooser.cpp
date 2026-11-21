#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/grouped_option_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockChooseGroup {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseInGroup {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct GroupedOptionChooserTest : public ::testing::Test {
    NiceMock<MockChooseGroup> choose_group;
    NiceMock<MockChooseInGroup> choose_in_group;
    grouped_option_chooser<NiceMock<MockChooseGroup>, NiceMock<MockChooseInGroup>> chooser{
        choose_group, choose_in_group};
};

}

TEST_F(GroupedOptionChooserTest, TheGroupIsChosenAmongTheKeysPresentThenTheOptionWithinIt) {
    EXPECT_CALL(choose_group, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_group, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_group, choose(3)).WillOnce(Return(2));
    EXPECT_CALL(choose_in_group, choose(2)).WillOnce(Return(1));
    // Keys 0, 3, 0, 5: groups 5, 3, 0 in that order; group 0 holds positions 0 and 2.
    EXPECT_EQ(chooser.choose({0, 3, 0, 5}), 2u);
}

TEST_F(GroupedOptionChooserTest, TheHighestKeyIsTheFirstGroup) {
    ON_CALL(choose_group, choose(_)).WillByDefault(Return(0));
    EXPECT_EQ(chooser.choose({0, 3, 0, 5}), 3u);
}

TEST_F(GroupedOptionChooserTest, AGroupOfOneOptionMakesNoChoiceWithinIt) {
    ON_CALL(choose_group, choose(_)).WillByDefault(Return(1));
    EXPECT_CALL(choose_in_group, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({0, 3, 0, 5}), 1u);
}

TEST_F(GroupedOptionChooserTest, OneKeyMakesNoGroupChoice) {
    EXPECT_CALL(choose_group, choose(_)).Times(0);
    EXPECT_CALL(choose_in_group, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_group, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose({0, 0, 0}), 2u);
}

TEST_F(GroupedOptionChooserTest, ASingleOptionIsTakenWithoutAnyChoice) {
    EXPECT_CALL(choose_group, choose(_)).Times(0);
    EXPECT_CALL(choose_in_group, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose({4}), 0u);
}

TEST_F(GroupedOptionChooserTest, AGroupsOptionsKeepTheirGivenOrder) {
    ON_CALL(choose_group, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(choose_in_group, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_in_group, choose(3)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose({2, 7, 7, 1, 7}), 2u);
}
