#include <any>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/native_cell_reader.hpp"

using namespace AML;

TEST(NativeCellReaderTest, TheRowsValueIsGivenInItsOwnAny) {
    const native_cell_reader<std::string> reader;
    EXPECT_EQ(std::any_cast<std::string>(reader(std::any(std::vector<std::string>{"a", "b"}), 1)), "b");
}

TEST(NativeCellReaderTest, ABooleanIsReadAsABool) {
    const native_cell_reader<bool> reader;
    EXPECT_TRUE(std::any_cast<bool>(reader(std::any(std::vector<bool>{false, true}), 1)));
}
