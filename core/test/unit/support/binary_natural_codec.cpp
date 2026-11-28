#include <cstddef>
#include <gtest/gtest.h>
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"

using namespace AML;

namespace {

// The binary natural constructors are ids 4 (zero), 5 (twice) and 6 (twice_plus_one), so nothing depends on them
// being 0, 1 and 2.
struct BinaryNaturalCodecTest : public ::testing::Test {
    BinaryNaturalCodecTest();
    binary_natural_ctors naturals;
    binary_natural_reader reader;
    binary_natural_writer writer;
};

BinaryNaturalCodecTest::BinaryNaturalCodecTest()
    : naturals{.zero = 4, .twice = 5, .twice_plus_one = 6}
    , reader(naturals)
    , writer(naturals) {}

}

TEST_F(BinaryNaturalCodecTest, ZeroIsTheZeroConstructor) {
    EXPECT_EQ(writer(0), (value{.ctor = 4, .args = {}}));
    EXPECT_EQ(reader(value{.ctor = 4, .args = {}}), 0u);
}

TEST_F(BinaryNaturalCodecTest, ANumberIsWrittenLowestBitOutermost) {
    const value one{.ctor = 6, .args = {value{.ctor = 4, .args = {}}}};
    const value two{.ctor = 5, .args = {one}};
    const value five{.ctor = 6, .args = {two}};
    EXPECT_EQ(writer(1), one);
    EXPECT_EQ(writer(2), two);
    EXPECT_EQ(writer(5), five);
}

TEST_F(BinaryNaturalCodecTest, ReadingAWrittenNumberGivesItBack) {
    for (size_t number = 0; number < 300; ++number)
        EXPECT_EQ(reader(writer(number)), number);
    EXPECT_EQ(reader(writer(size_t{1} << 39)), size_t{1} << 39);
}

TEST_F(BinaryNaturalCodecTest, ANumberHasOneForm) {
    for (size_t number = 1; number < 300; ++number) {
        const value* rest = nullptr;
        const value written = writer(number);
        for (rest = &written; !rest->args.empty(); rest = &rest->args[0])
            EXPECT_FALSE(rest->ctor == 5 && rest->args[0].ctor == 4);
    }
}

TEST_F(BinaryNaturalCodecTest, WritingCapsAtTheLargestNumber) {
    EXPECT_EQ(reader(writer(k_binary_natural_largest)), k_binary_natural_largest);
    EXPECT_EQ(reader(writer(static_cast<unsigned __int128>(k_binary_natural_largest) + 1)), k_binary_natural_largest);
    EXPECT_EQ(reader(writer(static_cast<unsigned __int128>(1) << 100)), k_binary_natural_largest);
}
