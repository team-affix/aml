#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_square.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalSquareTest : public ::testing::Test {
    BinaryNaturalSquareTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_square primitive;
};

BinaryNaturalSquareTest::BinaryNaturalSquareTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalSquareTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalSquareTest, ANumberIsMultipliedByItself) {
    {
        const value number = natural(0);
        EXPECT_EQ(reader(primitive({&number})), 0u);
    }
    {
        const value number = natural(1);
        EXPECT_EQ(reader(primitive({&number})), 1u);
    }
    {
        const value number = natural(12);
        EXPECT_EQ(reader(primitive({&number})), 144u);
    }
}

TEST_F(BinaryNaturalSquareTest, TheSquareIsCappedAtTheLargestNumber) {
    {
        const value number = natural(size_t{1} << 30);
        EXPECT_EQ(reader(primitive({&number})), k_binary_natural_largest);
    }
}
