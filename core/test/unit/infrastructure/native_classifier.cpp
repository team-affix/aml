#include <any>
#include <functional>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/native_classifier.hpp"

using namespace AML;

namespace {

struct NativeClassifierTest : public ::testing::Test {
    native_classifier<int, std::hash<int>> classifier;
};

}

TEST_F(NativeClassifierTest, AnEmptyColumnHasNoClasses) {
    EXPECT_TRUE(classifier(std::any(std::vector<int>{})).empty());
}

TEST_F(NativeClassifierTest, ClassesAreNumberedInOrderOfFirstAppearance) {
    EXPECT_EQ(classifier(std::any(std::vector<int>{9, 2, 9, 5, 2})), (std::vector<size_t>{0, 1, 0, 2, 1}));
}

TEST_F(NativeClassifierTest, AUniformColumnHasOneClass) {
    EXPECT_EQ(classifier(std::any(std::vector<int>{4, 4, 4})), (std::vector<size_t>{0, 0, 0}));
}

TEST_F(NativeClassifierTest, BooleanColumnsAreClassedThroughTheirPackedVector) {
    const native_classifier<bool, std::hash<bool>> booleans;
    EXPECT_EQ(booleans(std::any(std::vector<bool>{true, false, true})), (std::vector<size_t>{0, 1, 0}));
}

TEST_F(NativeClassifierTest, ValuesAreComparedByEquality) {
    const native_classifier<std::string, std::hash<std::string>> strings;
    EXPECT_EQ(strings(std::any(std::vector<std::string>{"ab", std::string("a") + "b"})), (std::vector<size_t>{0, 0}));
}
