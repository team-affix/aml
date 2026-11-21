#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/argument_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindArgumentOptions {
    MOCK_METHOD(
        std::vector<size_t>, find, (prim_id, const std::vector<type_id>&, const std::vector<size_t>&), (const));
};

struct MockChooseArgument {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct ArgumentChooserTest : public ::testing::Test {
    NiceMock<MockFindArgumentOptions> find_argument_options;
    NiceMock<MockChooseArgument> choose_argument;
    argument_chooser<NiceMock<MockFindArgumentOptions>, NiceMock<MockChooseArgument>> chooser{
        find_argument_options, choose_argument};
};

}

TEST_F(ArgumentChooserTest, EachArgumentIsChosenFromTheOptionsLeftByTheArgumentsBeforeIt) {
    const std::vector<type_id> arg_types{0, 1};
    EXPECT_CALL(find_argument_options, find(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(choose_argument, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(find_argument_options, find(3, arg_types, std::vector<size_t>{}))
        .WillOnce(Return(std::vector<size_t>{4, 6}));
    EXPECT_CALL(choose_argument, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(find_argument_options, find(3, arg_types, std::vector<size_t>{6}))
        .WillOnce(Return(std::vector<size_t>{0, 2, 5}));
    EXPECT_CALL(choose_argument, choose(3)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(3, arg_types), (std::vector<size_t>{6, 0}));
}

TEST_F(ArgumentChooserTest, ANullaryPrimitiveHasNoArguments) {
    EXPECT_TRUE(chooser.choose(0, {}).empty());
}

TEST_F(ArgumentChooserTest, AUnaryPrimitiveTakesOneChoice) {
    EXPECT_CALL(find_argument_options, find(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(choose_argument, choose(_)).Times(AnyNumber());
    EXPECT_CALL(find_argument_options, find(2, std::vector<type_id>{4}, std::vector<size_t>{}))
        .WillOnce(Return(std::vector<size_t>{7, 8, 9}));
    EXPECT_CALL(choose_argument, choose(3)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose(2, {4}), (std::vector<size_t>{9}));
}

TEST_F(ArgumentChooserTest, AForcedArgumentIsStillChosenThrough) {
    ON_CALL(find_argument_options, find(_, _, _)).WillByDefault(Return(std::vector<size_t>{5}));
    EXPECT_CALL(choose_argument, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_argument, choose(1)).Times(3).WillRepeatedly(Return(0));
    EXPECT_EQ(chooser.choose(0, {0, 0, 0}), (std::vector<size_t>{5, 5, 5}));
}
