#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/keyed_argument_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindArgumentOptions {
    MOCK_METHOD(std::vector<size_t>, find, (prim_id, const std::vector<type_id>&, const std::vector<size_t>&),
                (const));
};

struct MockMeasureKey {
    MOCK_METHOD(size_t, measure, (size_t), (const));
};

struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (const std::vector<size_t>&));
};

// A binary primitive 3 over type 0; operands 4, 6 and 9 are offered, with keys 0, 2 and 1.
struct KeyedArgumentChooserTest : public ::testing::Test {
    NiceMock<MockFindArgumentOptions> find_argument_options;
    NiceMock<MockMeasureKey> measure_key;
    NiceMock<MockChooseOption> choose_option;
    keyed_argument_chooser<NiceMock<MockFindArgumentOptions>, NiceMock<MockMeasureKey>, NiceMock<MockChooseOption>>
        chooser{find_argument_options, measure_key, choose_option};

    void SetUp() override {
        ON_CALL(find_argument_options, find(_, _, _)).WillByDefault(Return(std::vector<size_t>{4, 6, 9}));
        ON_CALL(measure_key, measure(4)).WillByDefault(Return(0));
        ON_CALL(measure_key, measure(6)).WillByDefault(Return(2));
        ON_CALL(measure_key, measure(9)).WillByDefault(Return(1));
        ON_CALL(choose_option, choose(_)).WillByDefault(Return(0));
    }
};

}

TEST_F(KeyedArgumentChooserTest, EachArgumentIsChosenByItsOptionsKeys) {
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_option, choose(std::vector<size_t>{0, 2, 1})).WillOnce(Return(1)).WillOnce(Return(2));
    EXPECT_EQ(chooser.choose(3, {0, 0}), (std::vector<size_t>{6, 9}));
}

TEST_F(KeyedArgumentChooserTest, EachPositionsOptionsAreFoundAfterTheArgumentsChosenSoFar) {
    EXPECT_CALL(find_argument_options, find(_, _, _)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(find_argument_options, find(3, std::vector<type_id>{0, 0}, std::vector<size_t>{}))
        .WillOnce(Return(std::vector<size_t>{4, 6, 9}));
    EXPECT_CALL(find_argument_options, find(3, std::vector<type_id>{0, 0}, std::vector<size_t>{4}))
        .WillOnce(Return(std::vector<size_t>{6}));
    EXPECT_EQ(chooser.choose(3, {0, 0}), (std::vector<size_t>{4, 6}));
}

TEST_F(KeyedArgumentChooserTest, EveryOfferedOperandIsMeasured) {
    EXPECT_CALL(measure_key, measure(_)).Times(AnyNumber());
    EXPECT_CALL(measure_key, measure(4)).WillOnce(Return(0));
    EXPECT_CALL(measure_key, measure(6)).WillOnce(Return(2));
    EXPECT_CALL(measure_key, measure(9)).WillOnce(Return(1));
    chooser.choose(3, {0});
}

TEST_F(KeyedArgumentChooserTest, ANullaryPrimitiveTakesNoArguments) {
    EXPECT_CALL(choose_option, choose(_)).Times(0);
    EXPECT_TRUE(chooser.choose(3, {}).empty());
}
