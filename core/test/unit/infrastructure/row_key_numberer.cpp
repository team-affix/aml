#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/row_key_numberer.hpp"

using namespace AML;

namespace {

struct RowKeyNumbererTest : public ::testing::Test {
    RowKeyNumbererTest();
    std::vector<size_t> alternating;
    std::vector<size_t> halves;
    row_key_numberer numberer;
};

RowKeyNumbererTest::RowKeyNumbererTest()
    : alternating{0, 1, 0, 1}
    , halves{0, 0, 1, 1}
    , numberer() {}

}

TEST_F(RowKeyNumbererTest, NoColumnsGiveTheEmptyPrefixAlone) {
    EXPECT_EQ(numberer.number({}, 3), (std::vector<std::vector<size_t>>{{0, 0, 0}}));
}

TEST_F(RowKeyNumbererTest, OneColumnIsNumberedInOrderOfFirstAppearance) {
    const std::vector<size_t> classes{2, 0, 2, 1};
    EXPECT_EQ(numberer.number({&classes}, 4), (std::vector<std::vector<size_t>>{{0, 0, 0, 0}, {0, 1, 0, 2}}));
}

TEST_F(RowKeyNumbererTest, EachLayerNumbersThePrefixSoFar) {
    EXPECT_EQ(numberer.number({&halves, &alternating}, 4),
              (std::vector<std::vector<size_t>>{{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 2, 3}}));
}

TEST_F(RowKeyNumbererTest, RowsAgreeingOnEveryColumnShareANumber) {
    const std::vector<size_t> first{0, 1, 0, 1};
    const std::vector<size_t> second{3, 2, 3, 2};
    EXPECT_EQ(numberer.number({&first, &second}, 4).back(), (std::vector<size_t>{0, 1, 0, 1}));
}

TEST_F(RowKeyNumbererTest, AColumnRepeatingAPrefixAddsNoDistinctions) {
    EXPECT_EQ(numberer.number({&alternating, &alternating}, 4).back(), (std::vector<size_t>{0, 1, 0, 1}));
}

TEST_F(RowKeyNumbererTest, NoRowsGiveEmptyLayers) {
    const std::vector<size_t> none;
    EXPECT_EQ(numberer.number({&none}, 0), (std::vector<std::vector<size_t>>{{}, {}}));
}
