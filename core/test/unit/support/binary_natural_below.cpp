#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_below.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalBelowTest : public ::testing::Test {
    BinaryNaturalBelowTest();
    value answer(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_below primitive;
};

BinaryNaturalBelowTest::BinaryNaturalBelowTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 4, 3, 7) {}

value BinaryNaturalBelowTest::answer(size_t number) const {
    const value argument = writer(number);
    return primitive({&argument});
}

}

TEST_F(BinaryNaturalBelowTest, NumbersUnderTheBoundAreBelow) {
    EXPECT_EQ(answer(0), truth);
    EXPECT_EQ(answer(3), truth);
}

TEST_F(BinaryNaturalBelowTest, TheBoundAndAboveAreNot) {
    EXPECT_EQ(answer(4), falsity);
    EXPECT_EQ(answer(5), falsity);
    EXPECT_EQ(answer(1099511627776), falsity);
}
