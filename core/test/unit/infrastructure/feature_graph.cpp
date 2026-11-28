#include <typeindex>
#include <gtest/gtest.h>
#include "infrastructure/feature_graph.hpp"

using namespace AML;

namespace {

std::vector<operand> feature_args(const std::vector<feature_id>& ids) {
    std::vector<operand> args;
    for (const feature_id id : ids)
        args.push_back(operand{operand::from_feature{.id = id}});
    return args;
}

struct FeatureGraphTest : public ::testing::Test {
    FeatureGraphTest();
    feature_record input;
    feature_record application;
    feature_graph graph;
};

FeatureGraphTest::FeatureGraphTest()
    : input{.origin = derivation{derivation::input{.index = 0}}, .type = std::type_index(typeid(bool)), .key = 11}
    , application{
          .origin = derivation{derivation::application{.prim = 1, .args = feature_args({0})}},
          .type = std::type_index(typeid(int)),
          .key = 22}
    , graph() {}

}

TEST_F(FeatureGraphTest, InsertedFeaturesAreNumberedInOrder) {
    EXPECT_EQ(graph.insert(input), 0u);
    EXPECT_EQ(graph.insert(application), 1u);
}

TEST_F(FeatureGraphTest, AFeaturesRecordIsKept) {
    graph.insert(input);
    graph.insert(application);
    EXPECT_EQ(graph.record(0), input);
    EXPECT_EQ(graph.record(1), application);
}

TEST_F(FeatureGraphTest, ReplacingAnOriginKeepsTheTypeAndKey) {
    graph.insert(input);
    graph.insert(application);
    const derivation other{derivation::application{.prim = 4, .args = feature_args({0, 0})}};
    graph.replace_origin(1, other);
    EXPECT_EQ(graph.record(1), (feature_record{.origin = other, .type = std::type_index(typeid(int)), .key = 22}));
}

TEST_F(FeatureGraphTest, ReplacingAnOriginLeavesOtherFeaturesAlone) {
    graph.insert(input);
    graph.insert(application);
    graph.replace_origin(1, derivation{derivation::application{.prim = 4, .args = feature_args({0, 0})}});
    EXPECT_EQ(graph.record(0), input);
}
