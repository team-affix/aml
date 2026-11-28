#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_same_parity.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalSameParityTest : public ::testing::Test {
    BinaryNaturalSameParityTest();
    value answer(size_t left, size_t right) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_same_parity primitive;
};

BinaryNaturalSameParityTest::BinaryNaturalSameParityTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalSameParityTest::answer(size_t left, size_t right) const {
    const value first = writer(left);
    const value second = writer(right);
    return primitive({&first, &second});
}

}

TEST_F(BinaryNaturalSameParityTest, NumbersOfTheSameParityMatch) {
    EXPECT_EQ(answer(0, 0), truth);
    EXPECT_EQ(answer(2, 8), truth);
    EXPECT_EQ(answer(3, 7), truth);
    EXPECT_EQ(answer(1099511627776, 4), truth);
}

TEST_F(BinaryNaturalSameParityTest, NumbersOfDifferentParityDoNot) {
    EXPECT_EQ(answer(0, 1), falsity);
    EXPECT_EQ(answer(2, 3), falsity);
    EXPECT_EQ(answer(3, 2), falsity);
}
