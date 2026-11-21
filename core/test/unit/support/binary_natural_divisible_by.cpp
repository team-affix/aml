#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_divisible_by.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalDivisibleByTest : public ::testing::Test {
    BinaryNaturalDivisibleByTest();
    value answer(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_divisible_by primitive;
};

BinaryNaturalDivisibleByTest::BinaryNaturalDivisibleByTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 3, 7) {}

value BinaryNaturalDivisibleByTest::answer(size_t number) const {
    const value argument = writer(number);
    return primitive({&argument});
}

}

TEST_F(BinaryNaturalDivisibleByTest, MultiplesOfTheDivisorAreDivisible) {
    EXPECT_EQ(answer(0), truth);
    EXPECT_EQ(answer(3), truth);
    EXPECT_EQ(answer(6), truth);
    EXPECT_EQ(answer(999999999999), truth);
}

TEST_F(BinaryNaturalDivisibleByTest, OtherNumbersAreNot) {
    EXPECT_EQ(answer(1), falsity);
    EXPECT_EQ(answer(2), falsity);
    EXPECT_EQ(answer(4), falsity);
    EXPECT_EQ(answer(1000000000000), falsity);
}
