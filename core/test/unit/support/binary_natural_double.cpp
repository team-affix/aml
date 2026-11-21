#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_double.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalDoubleTest : public ::testing::Test {
    BinaryNaturalDoubleTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_double primitive;
};

BinaryNaturalDoubleTest::BinaryNaturalDoubleTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalDoubleTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalDoubleTest, ANumberIsDoubled) {
    {
        const value number = natural(0);
        EXPECT_EQ(reader(primitive({&number})), 0u);
    }
    {
        const value number = natural(5);
        EXPECT_EQ(reader(primitive({&number})), 10u);
    }
    {
        const value number = natural(64);
        EXPECT_EQ(reader(primitive({&number})), 128u);
    }
}

TEST_F(BinaryNaturalDoubleTest, TheDoubleIsCappedAtTheLargestNumber) {
    {
        const value number = natural(k_binary_natural_largest);
        EXPECT_EQ(reader(primitive({&number})), k_binary_natural_largest);
    }
}
