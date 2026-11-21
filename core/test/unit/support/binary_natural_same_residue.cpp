#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_same_residue.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalSameResidueTest : public ::testing::Test {
    BinaryNaturalSameResidueTest();
    value answer(size_t left, size_t right) const;
    binary_natural_ctors naturals;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_same_residue primitive;
};

BinaryNaturalSameResidueTest::BinaryNaturalSameResidueTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 3, 7) {}

value BinaryNaturalSameResidueTest::answer(size_t left, size_t right) const {
    const value first = writer(left);
    const value second = writer(right);
    return primitive({&first, &second});
}

}

TEST_F(BinaryNaturalSameResidueTest, NumbersWithTheSameRemainderMatch) {
    EXPECT_EQ(answer(0, 0), truth);
    EXPECT_EQ(answer(1, 4), truth);
    EXPECT_EQ(answer(4, 1), truth);
    EXPECT_EQ(answer(3, 1099511627775), truth);
}

TEST_F(BinaryNaturalSameResidueTest, NumbersWithDifferentRemaindersDoNot) {
    EXPECT_EQ(answer(0, 1), falsity);
    EXPECT_EQ(answer(1, 2), falsity);
    EXPECT_EQ(answer(5, 3), falsity);
}
