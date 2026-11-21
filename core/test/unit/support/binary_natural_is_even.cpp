#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "support/binary_natural_is_even.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), and the boolean ones 3
// (false) and 7 (true), so nothing depends on them being 0, 1 and 2.
struct BinaryNaturalIsEvenTest : public ::testing::Test {
    BinaryNaturalIsEvenTest();
    value natural(size_t number) const;
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
    value falsity;
    value truth;
    binary_natural_is_even primitive;
};

BinaryNaturalIsEvenTest::BinaryNaturalIsEvenTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals)
    , falsity{.ctor = 3, .args = {}}
    , truth{.ctor = 7, .args = {}}
    , primitive(naturals, 3, 7) {}

value BinaryNaturalIsEvenTest::natural(size_t number) const {
    return writer(number);
}

}

TEST_F(BinaryNaturalIsEvenTest, EvenNumbersAreEvenAndOddOnesAreNot) {
    {
        const value number = natural(0);
        EXPECT_EQ(primitive({&number}), truth);
    }
    {
        const value number = natural(1);
        EXPECT_EQ(primitive({&number}), falsity);
    }
    {
        const value number = natural(2);
        EXPECT_EQ(primitive({&number}), truth);
    }
    {
        const value number = natural(1023);
        EXPECT_EQ(primitive({&number}), falsity);
    }
    {
        const value number = natural(1024);
        EXPECT_EQ(primitive({&number}), truth);
    }
}
