#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_add.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalAddTest : public ::testing::Test {
    BinaryNaturalAddTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_add primitive;
};

BinaryNaturalAddTest::BinaryNaturalAddTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalAddTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalAddTest, TwoNumbersAddUp) {
    {
        const value left = natural(3);
        const value right = natural(4);
        EXPECT_EQ(reader(primitive({&left, &right})), 7u);
    }
    {
        const value left = natural(0);
        const value right = natural(0);
        EXPECT_EQ(reader(primitive({&left, &right})), 0u);
    }
    {
        const value left = natural(1);
        const value right = natural(1);
        EXPECT_EQ(reader(primitive({&left, &right})), 2u);
    }
    {
        const value left = natural(0);
        const value right = natural(9);
        EXPECT_EQ(reader(primitive({&left, &right})), 9u);
    }
}

TEST_F(BinaryNaturalAddTest, TheSumIsCappedAtTheLargestNumber) {
    {
        const value left = natural(k_binary_natural_largest);
        const value right = natural(5);
        EXPECT_EQ(reader(primitive({&left, &right})), k_binary_natural_largest);
    }
}
