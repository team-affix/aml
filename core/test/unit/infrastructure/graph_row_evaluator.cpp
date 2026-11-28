#include <any>
#include <functional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "value_objects/cached_constant.hpp"
#include "infrastructure/graph_row_evaluator.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::MockFunction;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

MATCHER_P(HoldsIntegers, expected, "") {
    return std::any_cast<const std::vector<int>&>(arg) == expected;
}

struct MockGetNativePrimitive {
    MOCK_METHOD(const native_primitive&, primitive, (prim_id), (const));
};

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

struct MockReadConstant {
    MOCK_METHOD(std::shared_ptr<const cached_constant>, read, (constant_id));
};

// Constant 9 is 5. Primitive 2 is a mock function; the graph is x1 and p2(x1, constant 9), output last.
struct GraphRowEvaluatorTest : public ::testing::Test {
    GraphRowEvaluatorTest();
    std::shared_ptr<const cached_constant> five;
    native_type integer;
    MockFunction<std::any(const std::vector<operand_data>&, size_t)> apply;
    native_primitive applied;
    model_graph graph;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    NiceMock<MockGetNativeType> get_native_type;
    NiceMock<MockReadConstant> read_constant;
    graph_row_evaluator<NiceMock<MockGetNativePrimitive>, NiceMock<MockGetNativeType>, NiceMock<MockReadConstant>>
        evaluator{get_native_primitive, get_native_type, read_constant};

    void SetUp() override {
        ON_CALL(get_native_primitive, primitive(2)).WillByDefault(ReturnRef(applied));
        ON_CALL(get_native_type, type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(integer));
        ON_CALL(read_constant, read(9)).WillByDefault(Return(five));
        ON_CALL(apply, Call(_, 1)).WillByDefault(Return(std::any(std::vector<int>{40})));
    }
};

GraphRowEvaluatorTest::GraphRowEvaluatorTest()
    : five(std::make_shared<const cached_constant>(cached_constant{.type = typeid(int), .cell = std::any(5)}))
    , integer(native_type::of<int, std::hash<int>>())
    , apply()
    , applied{.arg_types = {typeid(int), typeid(int)}, .result_type = typeid(int), .apply = apply.AsStdFunction()}
    , graph{
          .nodes = {derivation{derivation::input{.index = 1}},
                    derivation{derivation::application{
                        .prim = 2,
                        .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_constant{.id = 9}}}}}},
          .types = {typeid(int), typeid(int)},
          .outputs = {1}}
    , get_native_primitive()
    , get_native_type()
    , read_constant() {}

}

TEST_F(GraphRowEvaluatorTest, AnOutputIsItsNodesOneRowColumn) {
    const std::vector<std::any> outputs = evaluator.evaluate(graph, {std::any(3), std::any(7)});
    ASSERT_EQ(outputs.size(), 1u);
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(outputs[0]), (std::vector<int>{40}));
}

TEST_F(GraphRowEvaluatorTest, AnApplicationReadsFeatureColumnsAndConstantScalars) {
    EXPECT_CALL(apply, Call(ElementsAre(
        AllOf(Field(&operand_data::data, Pointee(HoldsIntegers(std::vector<int>{7}))),
              Field(&operand_data::scalar, false)),
        operand_data{.data = &five->cell, .scalar = true}), 1))
        .WillOnce(Return(std::any(std::vector<int>{0})));
    evaluator.evaluate(graph, {std::any(3), std::any(7)});
}

TEST_F(GraphRowEvaluatorTest, AnInputCanBeAnOutput) {
    graph.outputs = {0};
    const std::vector<std::any> outputs = evaluator.evaluate(graph, {std::any(3), std::any(7)});
    EXPECT_EQ(std::any_cast<const std::vector<int>&>(outputs[0]), (std::vector<int>{7}));
}

TEST_F(GraphRowEvaluatorTest, AnEmptyGraphGivesNoOutputs) {
    EXPECT_TRUE(evaluator.evaluate(model_graph{.nodes = {}, .types = {}, .outputs = {}}, {}).empty());
}
