#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_pred.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on
// them being 0, 1 and 2.
struct BinaryNaturalPredTest : public ::testing::Test {
    BinaryNaturalPredTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    binary_natural_pred primitive;
};

BinaryNaturalPredTest::BinaryNaturalPredTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , primitive(naturals) {}

value BinaryNaturalPredTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalPredTest, OneIsTakenAway) {
    {
        const value number = natural(1);
        EXPECT_EQ(reader(primitive({&number})), 0u);
    }
    {
        const value number = natural(8);
        EXPECT_EQ(reader(primitive({&number})), 7u);
    }
    {
        const value number = natural(256);
        EXPECT_EQ(reader(primitive({&number})), 255u);
    }
}

TEST_F(BinaryNaturalPredTest, ZeroStaysZero) {
    {
        const value number = natural(0);
        EXPECT_EQ(reader(primitive({&number})), 0u);
    }
}
