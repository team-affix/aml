#include <any>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/operand_cell_reader.hpp"

using namespace AML;

TEST(OperandCellReaderTest, AColumnIsReadAtEachRow) {
    const std::any column(std::vector<int>{4, 5, 6});
    const operand_cell_reader<int> reader(operand_data{.data = &column, .scalar = false});
    EXPECT_EQ(reader.read(0), 4);
    EXPECT_EQ(reader.read(2), 6);
}

TEST(OperandCellReaderTest, AScalarIsReadAtEveryRow) {
    const std::any scalar(9);
    const operand_cell_reader<int> reader(operand_data{.data = &scalar, .scalar = true});
    EXPECT_EQ(reader.read(0), 9);
    EXPECT_EQ(reader.read(100), 9);
}

TEST(OperandCellReaderTest, AColumnCellIsReadInPlace) {
    const std::any column(std::vector<int>{4, 5});
    const operand_cell_reader<int> reader(operand_data{.data = &column, .scalar = false});
    EXPECT_EQ(&reader.read(1), &std::any_cast<const std::vector<int>&>(column)[1]);
}

TEST(OperandCellReaderTest, BooleanColumnsAndScalarsAreRead) {
    const std::any column(std::vector<bool>{false, true});
    const std::any scalar(true);
    EXPECT_TRUE(operand_cell_reader<bool>(operand_data{.data = &column, .scalar = false}).read(1));
    EXPECT_TRUE(operand_cell_reader<bool>(operand_data{.data = &scalar, .scalar = true}).read(0));
}
