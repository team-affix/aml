#include <any>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/uniform_column_detector.hpp"

using namespace AML;

namespace {

struct UniformColumnDetectorTest : public ::testing::Test {
    cached_column classed(std::vector<size_t> classes);
    uniform_column_detector detector;
};

cached_column UniformColumnDetectorTest::classed(std::vector<size_t> classes) {
    return cached_column{.type = typeid(int), .cells = std::any(), .classes = std::move(classes), .hash = 0};
}

}

TEST_F(UniformColumnDetectorTest, OneClassEverywhereIsUniform) {
    EXPECT_TRUE(detector.detect(classed({0, 0, 0})));
}

TEST_F(UniformColumnDetectorTest, ASecondClassIsNotUniform) {
    EXPECT_FALSE(detector.detect(classed({0, 0, 1})));
}

TEST_F(UniformColumnDetectorTest, NoRowsAreNotUniform) {
    EXPECT_FALSE(detector.detect(classed({})));
}

TEST_F(UniformColumnDetectorTest, OneRowIsUniform) {
    EXPECT_TRUE(detector.detect(classed({0})));
}
