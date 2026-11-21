#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_divides.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalDividesTest : public ::testing::Test {
    BinaryNaturalDividesTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_divides primitive;
};

BinaryNaturalDividesTest::BinaryNaturalDividesTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalDividesTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalDividesTest, TheFirstDividesTheSecondWhenItLeavesNoRemainder) {
    {
        const value left = natural(3);
        const value right = natural(12);
        EXPECT_EQ(primitive({&left, &right}), truth);
    }
    {
        const value left = natural(5);
        const value right = natural(12);
        EXPECT_EQ(primitive({&left, &right}), falsity);
    }
    {
        const value left = natural(1);
        const value right = natural(7);
        EXPECT_EQ(primitive({&left, &right}), truth);
    }
    {
        const value left = natural(4);
        const value right = natural(0);
        EXPECT_EQ(primitive({&left, &right}), truth);
    }
}

TEST_F(BinaryNaturalDividesTest, ZeroDividesOnlyZero) {
    {
        const value left = natural(0);
        const value right = natural(0);
        EXPECT_EQ(primitive({&left, &right}), truth);
    }
    {
        const value left = natural(0);
        const value right = natural(4);
        EXPECT_EQ(primitive({&left, &right}), falsity);
    }
}
