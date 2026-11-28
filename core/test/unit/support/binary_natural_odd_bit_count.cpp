#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_odd_bit_count.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalOddBitCountTest : public ::testing::Test {
    BinaryNaturalOddBitCountTest();
    value answer(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_odd_bit_count primitive;
};

BinaryNaturalOddBitCountTest::BinaryNaturalOddBitCountTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalOddBitCountTest::answer(size_t number) const {
    const value argument = writer(number);
    return primitive({&argument});
}

}

TEST_F(BinaryNaturalOddBitCountTest, AnOddNumberOfOneBitsIsOdd) {
    EXPECT_EQ(answer(1), truth);
    EXPECT_EQ(answer(2), truth);
    EXPECT_EQ(answer(7), truth);
    EXPECT_EQ(answer(1099511627776), truth);
}

TEST_F(BinaryNaturalOddBitCountTest, AnEvenNumberOfOneBitsIsNot) {
    EXPECT_EQ(answer(0), falsity);
    EXPECT_EQ(answer(3), falsity);
    EXPECT_EQ(answer(5), falsity);
    EXPECT_EQ(answer(1099511627775), falsity);
}
