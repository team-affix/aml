#include <any>
#include <functional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/lifted_primitive.hpp"
#include "support/double_vector_length.hpp"

using namespace AML;

namespace {

operand_data column_of(const std::any& cells) {
    return operand_data{.data = &cells, .scalar = false};
}

operand_data scalar_of(const std::any& value) {
    return operand_data{.data = &value, .scalar = true};
}

}

TEST(LiftedPrimitiveTest, TheFunctionIsAppliedToEveryRowsCells) {
    const lifted_primitive<int, int, int> plus{std::function<int(int, int)>(std::plus<int>())};
    const std::any left(std::vector<int>{1, 2, 3});
    const std::any right(std::vector<int>{10, 20, 30});
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(plus({column_of(left), column_of(right)}, 3)),
              (std::vector<int>{11, 22, 33}));
}

TEST(LiftedPrimitiveTest, AScalarIsReadAtEveryRow) {
    const lifted_primitive<int, int, int> minus{std::function<int(int, int)>(std::minus<int>())};
    const std::any numbers(std::vector<int>{5, 6, 7});
    const std::any five(5);
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(minus({column_of(numbers), scalar_of(five)}, 3)),
              (std::vector<int>{0, 1, 2}));
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(minus({scalar_of(five), column_of(numbers)}, 3)),
              (std::vector<int>{0, -1, -2}));
}

TEST(LiftedPrimitiveTest, ArgumentsOfDifferentTypesAreUnwrappedEach) {
    const lifted_primitive<bool, int, bool> differs{std::function<bool(int, bool)>(std::not_equal_to<bool>())};
    const std::any numbers(std::vector<int>{0, 1, 2});
    const std::any flags(std::vector<bool>{false, false, true});
    EXPECT_EQ(std::any_cast<const std::vector<bool>&>(differs({column_of(numbers), column_of(flags)}, 3)),
              (std::vector<bool>{false, true, false}));
}

TEST(LiftedPrimitiveTest, ABooleanScalarIsReadAtEveryRow) {
    const lifted_primitive<bool, bool, bool> exclusive{std::function<bool(bool, bool)>(std::bit_xor<bool>())};
    const std::any flags(std::vector<bool>{false, true});
    const std::any truth(true);
    EXPECT_EQ(std::any_cast<const std::vector<bool>&>(exclusive({column_of(flags), scalar_of(truth)}, 2)),
              (std::vector<bool>{true, false}));
}

TEST(LiftedPrimitiveTest, ConstReferenceArgumentsReadTheirColumns) {
    const lifted_primitive<int, const std::vector<double>&> length{
        std::function<int(const std::vector<double>&)>(double_vector_length())};
    const std::any vectors(std::vector<std::vector<double>>{{}, {1.0, 2.0}});
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(length({column_of(vectors)}, 2)), (std::vector<int>{0, 2}));
}

TEST(LiftedPrimitiveTest, NoRowsGiveAnEmptyColumn) {
    const lifted_primitive<int, int> negate{std::function<int(int)>(std::negate<int>())};
    const std::any numbers(std::vector<int>{});
    EXPECT_TRUE(std::any_cast<const std::vector<int>&>(negate({column_of(numbers)}, 0)).empty());
}
