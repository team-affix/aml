#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_mul.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalMulTest : public ::testing::Test {
    BinaryNaturalMulTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_mul primitive;
};

BinaryNaturalMulTest::BinaryNaturalMulTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalMulTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalMulTest, TwoNumbersMultiply) {
    {
        const value left = natural(6);
        const value right = natural(7);
        EXPECT_EQ(reader(primitive({&left, &right})), 42u);
    }
    {
        const value left = natural(1);
        const value right = natural(13);
        EXPECT_EQ(reader(primitive({&left, &right})), 13u);
    }
    {
        const value left = natural(0);
        const value right = natural(9);
        EXPECT_EQ(reader(primitive({&left, &right})), 0u);
    }
}

TEST_F(BinaryNaturalMulTest, TheProductIsCappedAtTheLargestNumber) {
    {
        const value left = natural(size_t{1} << 30);
        const value right = natural(size_t{1} << 30);
        EXPECT_EQ(reader(primitive({&left, &right})), k_binary_natural_largest);
    }
}
