#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_half.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalHalfTest : public ::testing::Test {
    BinaryNaturalHalfTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_half primitive;
};

BinaryNaturalHalfTest::BinaryNaturalHalfTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalHalfTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalHalfTest, ANumberIsHalvedRoundingDown) {
    {
        const value number = natural(0);
        EXPECT_EQ(reader(primitive({&number})), 0u);
    }
    {
        const value number = natural(1);
        EXPECT_EQ(reader(primitive({&number})), 0u);
    }
    {
        const value number = natural(7);
        EXPECT_EQ(reader(primitive({&number})), 3u);
    }
    {
        const value number = natural(8);
        EXPECT_EQ(reader(primitive({&number})), 4u);
    }
}
