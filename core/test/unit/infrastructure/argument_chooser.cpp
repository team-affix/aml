#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/argument_chooser.hpp"

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
