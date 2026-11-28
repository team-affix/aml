#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_absdiff.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalAbsdiffTest : public ::testing::Test {
    BinaryNaturalAbsdiffTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_absdiff primitive;
};

BinaryNaturalAbsdiffTest::BinaryNaturalAbsdiffTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalAbsdiffTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalAbsdiffTest, TheDistanceIsTheSameInEitherOrder) {
    {
        const value left = natural(3);
        const value right = natural(8);
        EXPECT_EQ(reader(primitive({&left, &right})), 5u);
    }
    {
        const value left = natural(8);
        const value right = natural(3);
        EXPECT_EQ(reader(primitive({&left, &right})), 5u);
    }
    {
        const value left = natural(4);
        const value right = natural(4);
        EXPECT_EQ(reader(primitive({&left, &right})), 0u);
    }
    {
        const value left = natural(0);
        const value right = natural(7);
        EXPECT_EQ(reader(primitive({&left, &right})), 7u);
    }
}
