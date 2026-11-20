#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/value_sizer.hpp"

using namespace AML;

namespace {

struct ValueSizerTest : public ::testing::Test {
    ValueSizerTest();
    value zero;
    value one;
    value two;
    value_sizer sizer;
};

ValueSizerTest::ValueSizerTest()
    : zero{.ctor = 0, .args = {}}
    , one{.ctor = 1, .args = {zero}}
    , two{.ctor = 1, .args = {one}}
    , sizer() {}

}

TEST_F(ValueSizerTest, ANullaryValueHasSizeOne) {
    EXPECT_EQ(sizer.size(zero), 1u);
}

TEST_F(ValueSizerTest, AValueCountsEveryConstructorInIt) {
    EXPECT_EQ(sizer.size(two), 3u);
    const value pair{.ctor = 2, .args = {two, one}};
    EXPECT_EQ(sizer.size(pair), 6u);
}

TEST_F(ValueSizerTest, ARepeatedArgumentIsCountedEachTimeItAppears) {
    const value pair{.ctor = 2, .args = {one, one}};
    EXPECT_EQ(sizer.size(pair), 5u);
}

TEST_F(ValueSizerTest, ADeepValueIsSizedFully) {
    value chain{.ctor = 0, .args = {}};
    for (size_t depth = 1; depth < 50; ++depth)
        chain = value{.ctor = 1, .args = {chain}};
    EXPECT_EQ(sizer.size(chain), 50u);
}
