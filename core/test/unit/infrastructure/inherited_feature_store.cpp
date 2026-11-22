#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/inherited_feature_store.hpp"

using namespace AML;

namespace {

struct InheritedFeatureStoreTest : public ::testing::Test {
    InheritedFeatureStoreTest();
    std::vector<size_t> first_rows;
    std::vector<size_t> second_rows;
    feature sum;
    feature product;
    inherited_feature_store store;
};

InheritedFeatureStoreTest::InheritedFeatureStoreTest()
    : first_rows{0, 1}
    , second_rows{2}
    , sum{feature::app{.prim = 0, .args = {feature{feature::var{.index = 0}}, feature{feature::var{.index = 1}}}}}
    , product{feature::app{.prim = 1, .args = {feature{feature::var{.index = 0}}, feature{feature::var{.index = 1}}}}}
    , store() {}

}

TEST_F(InheritedFeatureStoreTest, ABinWithoutARecordInheritsNothing) {
    EXPECT_FALSE(store.has(&first_rows));
    EXPECT_TRUE(store.inherited(&first_rows).empty());
}

TEST_F(InheritedFeatureStoreTest, ARecordedBinInheritsItsFeaturesInOrder) {
    store.record(&first_rows, {sum, product});
    EXPECT_TRUE(store.has(&first_rows));
    EXPECT_EQ(store.inherited(&first_rows), (std::vector<feature>{sum, product}));
}

TEST_F(InheritedFeatureStoreTest, ABinRecordedWithNothingHasARecord) {
    store.record(&first_rows, {});
    EXPECT_TRUE(store.has(&first_rows));
    EXPECT_TRUE(store.inherited(&first_rows).empty());
}

TEST_F(InheritedFeatureStoreTest, RecordsAreKeptPerBin) {
    store.record(&first_rows, {sum});
    store.record(&second_rows, {product});
    EXPECT_EQ(store.inherited(&first_rows), (std::vector<feature>{sum}));
    EXPECT_EQ(store.inherited(&second_rows), (std::vector<feature>{product}));
}

TEST_F(InheritedFeatureStoreTest, RecordingABinAgainReplacesItsRecord) {
    store.record(&first_rows, {sum});
    store.record(&first_rows, {product});
    EXPECT_EQ(store.inherited(&first_rows), (std::vector<feature>{product}));
}

TEST_F(InheritedFeatureStoreTest, ClearForgetsEveryRecord) {
    store.record(&first_rows, {sum});
    store.record(&second_rows, {product});
    store.clear();
    EXPECT_FALSE(store.has(&first_rows));
    EXPECT_FALSE(store.has(&second_rows));
    EXPECT_TRUE(store.inherited(&first_rows).empty());
}
