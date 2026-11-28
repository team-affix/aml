#include <any>
#include <functional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include "value_objects/native_column.hpp"
#include "value_objects/native_constant.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"

using namespace AML;

TEST(NativeTypeTest, ATypeCarriesItsCppTypeAndCellSize) {
    const native_type real = native_type::of<double, std::hash<double>>();
    EXPECT_EQ(real.type, std::type_index(typeid(double)));
    EXPECT_EQ(real.cell_bytes, sizeof(double));
}

TEST(NativeTypeTest, TheOperationsWorkOnColumnsOfTheType) {
    const native_type integer = native_type::of<int, std::hash<int>>();
    const std::any cells(std::vector<int>{2, 5, 2});
    EXPECT_EQ(integer.classify(cells), (std::vector<size_t>{0, 1, 0}));
    EXPECT_EQ(integer.hash(cells), integer.hash(std::any(std::vector<int>{2, 5, 2})));
    EXPECT_TRUE(integer.equal_at(cells, 0, cells, 2));
    EXPECT_EQ(std::any_cast<int>(integer.cell(cells, 1)), 5);
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(integer.fill(std::any(9), 2)), (std::vector<int>{9, 9}));
}

TEST(NativePrimitiveTest, APrimitiveRecordsItsArgumentAndResultTypes) {
    const native_primitive differs =
        native_primitive::of(std::function<bool(int, double)>(std::not_equal_to<double>()));
    EXPECT_EQ(differs.arg_types, (std::vector<std::type_index>{typeid(int), typeid(double)}));
    EXPECT_EQ(differs.result_type, std::type_index(typeid(bool)));
}

TEST(NativePrimitiveTest, ReferenceArgumentsAreRecordedByTheirValueType) {
    const native_primitive size_of = native_primitive::of(
        std::function<size_t(const std::vector<int>&)>(std::mem_fn(&std::vector<int>::size)));
    EXPECT_EQ(size_of.arg_types, (std::vector<std::type_index>{typeid(std::vector<int>)}));
}

TEST(NativePrimitiveTest, ThePrimitiveAppliesToColumns) {
    const native_primitive plus = native_primitive::of(std::function<int(int, int)>(std::plus<int>()));
    const std::any left(std::vector<int>{1, 2});
    const std::any right(std::vector<int>{3, 4});
    const std::vector<operand_data> operands{
        operand_data{.data = &left, .scalar = false}, operand_data{.data = &right, .scalar = false}};
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(plus.apply(operands, 2)), (std::vector<int>{4, 6}));
}

TEST(NativeColumnTest, AColumnHoldsItsCellsAndTheirType) {
    const native_column column = native_column::of(std::vector<double>{1.5, 2.5});
    EXPECT_EQ(column.type, std::type_index(typeid(double)));
    EXPECT_EQ(std::any_cast<const std::vector<double>&>(column.cells), (std::vector<double>{1.5, 2.5}));
}

TEST(NativeConstantTest, AConstantHoldsItsValueAndType) {
    const native_constant constant = native_constant::of(7L);
    EXPECT_EQ(constant.type, std::type_index(typeid(long)));
    EXPECT_EQ(std::any_cast<long>(constant.literal), 7L);
}
