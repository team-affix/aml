#include <any>
#include <memory>
#include <optional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/table_model_evaluator.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;

namespace {

struct MockEvaluateGraphRow {
    MOCK_METHOD(std::vector<std::any>, evaluate, (const model_graph&, const std::vector<std::any>&), (const));
};

struct MockFindValueClass {
    MOCK_METHOD(std::optional<size_t>, find, (const cached_column&, const std::any&), (const));
};

// The model reads two features and maps the classes (0, 1) to 5 and (1, 1) to 6. The graph gives the values a and b,
// which the first and second columns class as 1 and 1.
struct TableModelEvaluatorTest : public ::testing::Test {
    TableModelEvaluatorTest();
    std::shared_ptr<const cached_column> empty_column();
    table_model model;
    std::vector<std::any> inputs;
    NiceMock<MockEvaluateGraphRow> evaluate_graph_row;
    NiceMock<MockFindValueClass> find_value_class;
    table_model_evaluator<NiceMock<MockEvaluateGraphRow>, NiceMock<MockFindValueClass>> evaluator{
        evaluate_graph_row, find_value_class};

    void SetUp() override {
        ON_CALL(evaluate_graph_row, evaluate(model.graph, _))
            .WillByDefault(Return(std::vector<std::any>{std::any(10), std::any(20)}));
        ON_CALL(find_value_class, find(_, _)).WillByDefault(Return(std::optional<size_t>(1)));
    }
};

TableModelEvaluatorTest::TableModelEvaluatorTest()
    : model{
          .graph = model_graph{
              .nodes = {derivation{derivation::input{.index = 0}}, derivation{derivation::input{.index = 1}}},
              .types = {typeid(int), typeid(int)}, .outputs = {0, 1}},
          .columns = {empty_column(), empty_column()},
          .rows = {{{0, 1}, std::any(5)}, {{1, 1}, std::any(6)}}}
    , inputs{std::any(10), std::any(20)}
    , evaluate_graph_row()
    , find_value_class() {}

std::shared_ptr<const cached_column> TableModelEvaluatorTest::empty_column() {
    return std::make_shared<const cached_column>(
        cached_column{.type = typeid(int), .cells = std::any(std::vector<int>{}), .classes = {}, .hash = 0});
}

}

TEST_F(TableModelEvaluatorTest, TheValuesClassesLookTheLabelUp) {
    EXPECT_EQ(std::any_cast<int>(evaluator.evaluate(model, inputs).value()), 6);
}

TEST_F(TableModelEvaluatorTest, EachValueIsClassedInItsOwnFeaturesColumn) {
    EXPECT_CALL(find_value_class, find(_, _)).Times(0);
    EXPECT_CALL(find_value_class, find(Ref(*model.columns[0]), _)).WillOnce(Return(std::optional<size_t>(0)));
    EXPECT_CALL(find_value_class, find(Ref(*model.columns[1]), _)).WillOnce(Return(std::optional<size_t>(1)));
    EXPECT_EQ(std::any_cast<int>(evaluator.evaluate(model, inputs).value()), 5);
}

TEST_F(TableModelEvaluatorTest, AValueNoTrainingRowHeldGivesNoLabel) {
    ON_CALL(find_value_class, find(_, _)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(evaluator.evaluate(model, inputs).has_value());
}

TEST_F(TableModelEvaluatorTest, ATupleTheTableDoesNotHoldGivesNoLabel) {
    ON_CALL(find_value_class, find(_, _)).WillByDefault(Return(std::optional<size_t>(0)));
    EXPECT_FALSE(evaluator.evaluate(model, inputs).has_value());
}

TEST_F(TableModelEvaluatorTest, TheGraphIsEvaluatedOnTheInputs) {
    EXPECT_CALL(evaluate_graph_row, evaluate(model.graph, _))
        .WillOnce(Return(std::vector<std::any>{std::any(10), std::any(20)}));
    evaluator.evaluate(model, inputs);
}

TEST_F(TableModelEvaluatorTest, AModelWithNoFeaturesGivesItsOneLabel) {
    const table_model constant{
        .graph = model_graph{.nodes = {}, .types = {}, .outputs = {}}, .columns = {}, .rows = {{{}, std::any(4)}}};
    ON_CALL(evaluate_graph_row, evaluate(constant.graph, _)).WillByDefault(Return(std::vector<std::any>{}));
    EXPECT_EQ(std::any_cast<int>(evaluator.evaluate(constant, inputs).value()), 4);
}
