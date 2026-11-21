#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_mod.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalModTest : public ::testing::Test {
    BinaryNaturalModTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_mod primitive;
};

BinaryNaturalModTest::BinaryNaturalModTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalModTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalModTest, TheRemainderIsWhatDivisionLeaves) {
    {
        const value left = natural(17);
        const value right = natural(5);
        EXPECT_EQ(reader(primitive({&left, &right})), 2u);
    }
    {
        const value left = natural(15);
        const value right = natural(5);
        EXPECT_EQ(reader(primitive({&left, &right})), 0u);
    }
    {
        const value left = natural(0);
        const value right = natural(4);
        EXPECT_EQ(reader(primitive({&left, &right})), 0u);
    }
    {
        const value left = natural(4);
        const value right = natural(9);
        EXPECT_EQ(reader(primitive({&left, &right})), 4u);
    }
}

TEST_F(BinaryNaturalModTest, TheRemainderModuloZeroIsTheNumber) {
    {
        const value left = natural(9);
        const value right = natural(0);
        EXPECT_EQ(reader(primitive({&left, &right})), 9u);
    }
    {
        const value left = natural(0);
        const value right = natural(0);
        EXPECT_EQ(reader(primitive({&left, &right})), 0u);
    }
}
