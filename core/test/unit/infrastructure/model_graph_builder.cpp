#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_graph_builder.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

std::vector<operand> feature_args(const std::vector<feature_id>& ids) {
    std::vector<operand> args;
    for (const feature_id id : ids)
        args.push_back(operand{operand::from_feature{.id = id}});
    return args;
}

struct MockGetFeatureRecord {
    MOCK_METHOD(const feature_record&, record, (feature_id), (const));
};

// 0: bool input 2; 1: bool input 1; 2: int application p3(0, 1, constant 8); 3: int application p4(2, 2);
// 4: bool application p5(6); 5: bool input 0; 6: bool application p6(5), with a larger id than 4.
struct ModelGraphBuilderTest : public ::testing::Test {
    ModelGraphBuilderTest();
    feature_record application(prim_id prim, std::vector<feature_id> args, std::type_index type);
    std::vector<feature_record> records;
    NiceMock<MockGetFeatureRecord> get_feature_record;
    model_graph_builder<NiceMock<MockGetFeatureRecord>> builder{get_feature_record};

    void SetUp() override {
        for (feature_id feature = 0; feature < records.size(); ++feature)
            ON_CALL(get_feature_record, record(feature)).WillByDefault(ReturnRef(records[feature]));
    }
};

ModelGraphBuilderTest::ModelGraphBuilderTest()
    : records{
          feature_record{.origin = derivation{derivation::input{.index = 2}}, .type = typeid(bool), .key = 0},
          feature_record{.origin = derivation{derivation::input{.index = 1}}, .type = typeid(bool), .key = 0},
          feature_record{
              .origin = derivation{derivation::application{
                  .prim = 3,
                  .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_feature{.id = 1}},
                           operand{operand::from_constant{.id = 8}}}}},
              .type = typeid(int),
              .key = 0},
          application(4, {2, 2}, typeid(int)),
          application(5, {6}, typeid(bool)),
          feature_record{.origin = derivation{derivation::input{.index = 0}}, .type = typeid(bool), .key = 0},
          application(6, {5}, typeid(bool))}
    , get_feature_record() {}

feature_record ModelGraphBuilderTest::application(prim_id prim, std::vector<feature_id> args, std::type_index type) {
    return feature_record{
        .origin = derivation{derivation::application{.prim = prim, .args = feature_args(args)}}, .type = type,
        .key = 0};
}

}

TEST_F(ModelGraphBuilderTest, NoMembersGiveAnEmptyGraph) {
    EXPECT_EQ(builder.build({}), (model_graph{.nodes = {}, .types = {}, .outputs = {}}));
}

TEST_F(ModelGraphBuilderTest, AnInputIsOneNodeAndTheOutput) {
    EXPECT_EQ(builder.build({0}), (model_graph{
        .nodes = {derivation{derivation::input{.index = 2}}}, .types = {typeid(bool)}, .outputs = {0}}));
}

TEST_F(ModelGraphBuilderTest, FeatureArgumentsComeFirstAsNodePositionsAndConstantsKeepTheirIds) {
    EXPECT_EQ(builder.build({2}), (model_graph{
        .nodes = {derivation{derivation::input{.index = 2}}, derivation{derivation::input{.index = 1}},
                  derivation{derivation::application{
                      .prim = 3,
                      .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_feature{.id = 1}},
                               operand{operand::from_constant{.id = 8}}}}}},
        .types = {typeid(bool), typeid(bool), typeid(int)},
        .outputs = {2}}));
}

TEST_F(ModelGraphBuilderTest, ASharedAncestorIsOneNode) {
    const model_graph graph = builder.build({3, 0});
    EXPECT_EQ(graph.nodes.size(), 4u);
    EXPECT_EQ(graph.nodes[3], (derivation{derivation::application{.prim = 4, .args = feature_args({2, 2})}}));
    EXPECT_EQ(graph.outputs, (std::vector<size_t>{3, 0}));
}

TEST_F(ModelGraphBuilderTest, AnArgumentWithALargerIdStillComesFirst) {
    const model_graph graph = builder.build({4});
    EXPECT_EQ(graph.nodes, (std::vector<derivation>{
        derivation{derivation::input{.index = 0}},
        derivation{derivation::application{.prim = 6, .args = feature_args({0})}},
        derivation{derivation::application{.prim = 5, .args = feature_args({1})}}}));
    EXPECT_EQ(graph.outputs, (std::vector<size_t>{2}));
}
