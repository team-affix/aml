#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_succ.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalSuccTest : public ::testing::Test {
    BinaryNaturalSuccTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_succ primitive;
};

BinaryNaturalSuccTest::BinaryNaturalSuccTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalSuccTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalSuccTest, OneIsAdded) {
    {
        const value number = natural(0);
        EXPECT_EQ(reader(primitive({&number})), 1u);
    }
    {
        const value number = natural(7);
        EXPECT_EQ(reader(primitive({&number})), 8u);
    }
    {
        const value number = natural(255);
        EXPECT_EQ(reader(primitive({&number})), 256u);
    }
}

TEST_F(BinaryNaturalSuccTest, TheLargestNumberStaysTheLargest) {
    {
        const value number = natural(k_binary_natural_largest);
        EXPECT_EQ(reader(primitive({&number})), k_binary_natural_largest);
    }
}
