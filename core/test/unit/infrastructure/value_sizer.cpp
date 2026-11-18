#include <gtest/gtest.h>
#include "infrastructure/value_sizer.hpp"

namespace {

struct ValueSizerTest : public ::testing::Test {
    ValueSizerTest();
    value zero;
    value one;
    value two;
    value_sizer sizer;
};

ValueSizerTest::ValueSizerTest()
    : zero{.ctor = 0, .fields = {}}
    , one{.ctor = 1, .fields = {&zero}}
    , two{.ctor = 1, .fields = {&one}}
    , sizer() {}

}

TEST_F(ValueSizerTest, ANullaryValueHasSizeOne) {
    EXPECT_EQ(sizer.size(&zero), 1u);
}

TEST_F(ValueSizerTest, AValueCountsEveryConstructorInIt) {
    EXPECT_EQ(sizer.size(&two), 3u);
    const value pair{.ctor = 2, .fields = {&two, &one}};
    EXPECT_EQ(sizer.size(&pair), 6u);
}
