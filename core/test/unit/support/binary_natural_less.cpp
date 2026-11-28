#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_less.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalLessTest : public ::testing::Test {
    BinaryNaturalLessTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_less primitive;
};

BinaryNaturalLessTest::BinaryNaturalLessTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalLessTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalLessTest, OnlyAStrictlySmallerFirstIsLess) {
    {
        const value left = natural(2);
        const value right = natural(3);
        EXPECT_EQ(primitive({&left, &right}), truth);
    }
    {
        const value left = natural(3);
        const value right = natural(2);
        EXPECT_EQ(primitive({&left, &right}), falsity);
    }
    {
        const value left = natural(3);
        const value right = natural(3);
        EXPECT_EQ(primitive({&left, &right}), falsity);
    }
    {
        const value left = natural(0);
        const value right = natural(1);
        EXPECT_EQ(primitive({&left, &right}), truth);
    }
}
