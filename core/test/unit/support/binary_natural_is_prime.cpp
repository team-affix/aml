#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_is_prime.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalIsPrimeTest : public ::testing::Test {
    BinaryNaturalIsPrimeTest();
    value answer(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_is_prime primitive;
};

BinaryNaturalIsPrimeTest::BinaryNaturalIsPrimeTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalIsPrimeTest::answer(size_t number) const {
    const value argument = writer(number);
    return primitive({&argument});
}

}

TEST_F(BinaryNaturalIsPrimeTest, PrimesArePrime) {
    EXPECT_EQ(answer(2), truth);
    EXPECT_EQ(answer(3), truth);
    EXPECT_EQ(answer(5), truth);
    EXPECT_EQ(answer(37), truth);
    EXPECT_EQ(answer(41), truth);
    EXPECT_EQ(answer(1000003), truth);
    EXPECT_EQ(answer(1099511627689), truth);
}

TEST_F(BinaryNaturalIsPrimeTest, ZeroOneAndCompositesAreNot) {
    EXPECT_EQ(answer(0), falsity);
    EXPECT_EQ(answer(1), falsity);
    EXPECT_EQ(answer(4), falsity);
    EXPECT_EQ(answer(9), falsity);
    EXPECT_EQ(answer(561), falsity);
    EXPECT_EQ(answer(2047), falsity);
    EXPECT_EQ(answer(1000006000009), falsity);
}
