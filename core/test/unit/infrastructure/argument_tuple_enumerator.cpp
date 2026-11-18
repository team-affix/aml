#include <gtest/gtest.h>
#include "infrastructure/argument_tuple_enumerator.hpp"

namespace {

struct ArgumentTupleEnumeratorTest : public ::testing::Test {
    ArgumentTupleEnumeratorTest();
    candidate typed(type_id type);
    argument_tuple_enumerator enumerator;
};

ArgumentTupleEnumeratorTest::ArgumentTupleEnumeratorTest()
    : enumerator() {}

candidate ArgumentTupleEnumeratorTest::typed(type_id type) {
    return candidate{.expression = nullptr, .cost = 1, .observed = typed_column{.type = type, .cells = {}}};
}

}

TEST_F(ArgumentTupleEnumeratorTest, EveryOperandOfTheRightTypeInTheFrontierIsATuple) {
    const std::vector<candidate> operands{typed(0), typed(1), typed(0)};
    EXPECT_EQ(enumerator.enumerate({0}, operands, 0, 3), (std::vector<std::vector<size_t>>{{0}, {2}}));
}

TEST_F(ArgumentTupleEnumeratorTest, TuplesWithoutAFrontierOperandAreSkipped) {
    const std::vector<candidate> operands{typed(0), typed(0), typed(0)};
    EXPECT_EQ(enumerator.enumerate({0, 0}, operands, 2, 3),
              (std::vector<std::vector<size_t>>{{0, 2}, {1, 2}, {2, 0}, {2, 1}, {2, 2}}));
}

TEST_F(ArgumentTupleEnumeratorTest, EachPositionTakesOperandsOfItsOwnType) {
    const std::vector<candidate> operands{typed(0), typed(1), typed(1)};
    EXPECT_EQ(enumerator.enumerate({1, 0}, operands, 0, 3), (std::vector<std::vector<size_t>>{{1, 0}, {2, 0}}));
}

TEST_F(ArgumentTupleEnumeratorTest, APositionNoOperandFitsGivesNoTuples) {
    const std::vector<candidate> operands{typed(0)};
    EXPECT_TRUE(enumerator.enumerate({0, 1}, operands, 0, 1).empty());
}
