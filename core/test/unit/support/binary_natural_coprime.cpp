#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_coprime.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalCoprimeTest : public ::testing::Test {
    BinaryNaturalCoprimeTest();
    value answer(size_t left, size_t right) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_coprime primitive;
};

BinaryNaturalCoprimeTest::BinaryNaturalCoprimeTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalCoprimeTest::answer(size_t left, size_t right) const {
    const value first = writer(left);
    const value second = writer(right);
    return primitive({&first, &second});
}

}

TEST_F(BinaryNaturalCoprimeTest, NumbersWithNoCommonFactorAreCoprime) {
    EXPECT_EQ(answer(1, 0), truth);
    EXPECT_EQ(answer(0, 1), truth);
    EXPECT_EQ(answer(8, 9), truth);
    EXPECT_EQ(answer(1099511627689, 1099511627776), truth);
}

TEST_F(BinaryNaturalCoprimeTest, NumbersWithACommonFactorAreNot) {
    EXPECT_EQ(answer(0, 0), falsity);
    EXPECT_EQ(answer(0, 2), falsity);
    EXPECT_EQ(answer(6, 9), falsity);
    EXPECT_EQ(answer(9, 6), falsity);
    EXPECT_EQ(answer(1000003, 1000006000009), falsity);
}
