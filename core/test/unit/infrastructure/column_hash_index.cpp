#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/column_hash_index.hpp"

using namespace AML;

namespace {

struct ColumnHashIndexTest : public ::testing::Test {
    column_hash_index index;
};

}

TEST_F(ColumnHashIndexTest, AnUnknownHashFindsNone) {
    EXPECT_TRUE(index.find(5).empty());
}

TEST_F(ColumnHashIndexTest, AHashFindsItsFeaturesInInsertionOrder) {
    index.insert(5, 3);
    index.insert(5, 1);
    EXPECT_EQ(index.find(5), (std::vector<feature_id>{3, 1}));
}

TEST_F(ColumnHashIndexTest, HashesKeepTheirFeaturesApart) {
    index.insert(5, 3);
    index.insert(6, 4);
    EXPECT_EQ(index.find(5), (std::vector<feature_id>{3}));
    EXPECT_EQ(index.find(6), (std::vector<feature_id>{4}));
}
