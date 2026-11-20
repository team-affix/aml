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
    : zero{.tag = 0, .args = {}}
    , one{.tag = 1, .args = {&zero}}
    , two{.tag = 1, .args = {&one}}
    , sizer() {}

}

TEST_F(ValueSizerTest, ANullaryValueHasSizeOne) {
    EXPECT_EQ(sizer.size(&zero), 1u);
}

TEST_F(ValueSizerTest, AValueCountsEveryConstructorInIt) {
    EXPECT_EQ(sizer.size(&two), 3u);
    const value pair{.tag = 2, .args = {&two, &one}};
    EXPECT_EQ(sizer.size(&pair), 6u);
}

TEST_F(ValueSizerTest, ASharedArgumentIsCountedEachTimeItAppears) {
    const value pair{.tag = 2, .args = {&one, &one}};
    EXPECT_EQ(sizer.size(&pair), 5u);
}

TEST_F(ValueSizerTest, ADeepValueIsSizedFully) {
    std::vector<value> chain;
    chain.reserve(50);
    chain.push_back(value{.tag = 0, .args = {}});
    for (size_t depth = 1; depth < 50; ++depth)
        chain.push_back(value{.tag = 1, .args = {&chain.back()}});
    EXPECT_EQ(sizer.size(&chain.back()), 50u);
}
