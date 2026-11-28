#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_column_evaluator.hpp"
#include "value_objects/primitive_type.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetPrimitiveType {
    MOCK_METHOD(const primitive_type&, type, (prim_id), (const));
};

struct MockCallPrimitive {
    MOCK_METHOD(value, call, (prim_id, const std::vector<const value*>&), (const));
};

// Values are bare constructors 0..5; type 3 is the result type of primitives 0 and 1.
struct FeatureColumnEvaluatorTest : public ::testing::Test {
    FeatureColumnEvaluatorTest();
    static value leaf(ctor_id ctor);
    static column_view view(type_id type, column cells);
    feature x0;
    feature x1;
    primitive_type result;
    bin_view data;
    NiceMock<MockGetPrimitiveType> get_primitive_type;
    NiceMock<MockCallPrimitive> call_primitive;
    feature_column_evaluator<NiceMock<MockGetPrimitiveType>, NiceMock<MockCallPrimitive>> evaluator{
        get_primitive_type, call_primitive};

    void SetUp() override {
        ON_CALL(get_primitive_type, type(_)).WillByDefault(ReturnRef(result));
    }
};

FeatureColumnEvaluatorTest::FeatureColumnEvaluatorTest()
    : x0{feature::var{.index = 0}}
    , x1{feature::var{.index = 1}}
    , result{.arg_types = {0, 0}, .result_type = 3}
    , data(bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {leaf(0), leaf(1)}},
                                       typed_column{.type = 0, .cells = {leaf(2), leaf(3)}}},
                            .labels = typed_column{.type = 0, .cells = {leaf(0), leaf(0)}}}))
    , get_primitive_type()
    , call_primitive() {}

value FeatureColumnEvaluatorTest::leaf(ctor_id ctor) {
    return value{.ctor = ctor, .args = {}};
}

column_view FeatureColumnEvaluatorTest::view(type_id type, column cells) {
    return column_view::of(typed_column{.type = type, .cells = std::move(cells)});
}

}

TEST_F(FeatureColumnEvaluatorTest, AnApplicationIsCalledRowByRowOnItsArgumentsValues) {
    EXPECT_CALL(call_primitive, call(_, _)).Times(AnyNumber());
    EXPECT_CALL(call_primitive, call(0, ElementsAre(Pointee(leaf(0)), Pointee(leaf(2))))).WillOnce(Return(leaf(4)));
    EXPECT_CALL(call_primitive, call(0, ElementsAre(Pointee(leaf(1)), Pointee(leaf(3))))).WillOnce(Return(leaf(5)));
    const feature application{feature::app{.prim = 0, .args = {x0, x1}}};
    EXPECT_EQ(evaluator.evaluate(application, data), view(3, {leaf(4), leaf(5)}));
}

TEST_F(FeatureColumnEvaluatorTest, TheColumnTakesThePrimitivesResultType) {
    ON_CALL(call_primitive, call(_, _)).WillByDefault(Return(leaf(4)));
    EXPECT_CALL(get_primitive_type, type(_)).Times(AnyNumber());
    EXPECT_CALL(get_primitive_type, type(1)).WillRepeatedly(ReturnRef(result));
    const feature application{feature::app{.prim = 1, .args = {x0}}};
    EXPECT_EQ(evaluator.evaluate(application, data).type, 3u);
}

TEST_F(FeatureColumnEvaluatorTest, AConstantIsItsLiteralOnEveryRow) {
    EXPECT_CALL(call_primitive, call(_, _)).Times(AnyNumber());
    EXPECT_CALL(call_primitive, call(0, ElementsAre(Pointee(leaf(0)), Pointee(leaf(5))))).WillOnce(Return(leaf(1)));
    EXPECT_CALL(call_primitive, call(0, ElementsAre(Pointee(leaf(1)), Pointee(leaf(5))))).WillOnce(Return(leaf(2)));
    const feature application{feature::app{.prim = 0, .args = {x0, feature{feature::constant{.literal = leaf(5)}}}}};
    EXPECT_EQ(evaluator.evaluate(application, data), view(3, {leaf(1), leaf(2)}));
}

TEST_F(FeatureColumnEvaluatorTest, AnInnerApplicationIsEvaluatedForItsOuterOne) {
    EXPECT_CALL(call_primitive, call(_, _)).Times(AnyNumber());
    EXPECT_CALL(call_primitive, call(1, ElementsAre(Pointee(leaf(0))))).WillOnce(Return(leaf(2)));
    EXPECT_CALL(call_primitive, call(1, ElementsAre(Pointee(leaf(1))))).WillOnce(Return(leaf(3)));
    EXPECT_CALL(call_primitive, call(0, ElementsAre(Pointee(leaf(2))))).WillOnce(Return(leaf(4)));
    EXPECT_CALL(call_primitive, call(0, ElementsAre(Pointee(leaf(3))))).WillOnce(Return(leaf(5)));
    const feature inner{feature::app{.prim = 1, .args = {x0}}};
    const feature outer{feature::app{.prim = 0, .args = {inner}}};
    EXPECT_EQ(evaluator.evaluate(outer, data), view(3, {leaf(4), leaf(5)}));
}

TEST_F(FeatureColumnEvaluatorTest, AVariableIsReadThroughTheBinsSelectionOfRows) {
    const auto rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{2, 0});
    const auto cells = std::make_shared<const column>(column{leaf(0), leaf(1), leaf(2)});
    const bin_view selected{.inputs = {column_view{.type = 0, .cells = cells, .rows = rows}},
                            .labels = column_view{.type = 0, .cells = cells, .rows = rows}};
    EXPECT_CALL(call_primitive, call(_, _)).Times(AnyNumber());
    EXPECT_CALL(call_primitive, call(1, ElementsAre(Pointee(leaf(2))))).WillOnce(Return(leaf(4)));
    EXPECT_CALL(call_primitive, call(1, ElementsAre(Pointee(leaf(0))))).WillOnce(Return(leaf(5)));
    const feature application{feature::app{.prim = 1, .args = {x0}}};
    EXPECT_EQ(evaluator.evaluate(application, selected), view(3, {leaf(4), leaf(5)}));
}

TEST_F(FeatureColumnEvaluatorTest, ABinWithoutRowsGivesAnEmptyColumn) {
    const bin_view empty = bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {}}},
                                            .labels = typed_column{.type = 0, .cells = {}}});
    EXPECT_CALL(call_primitive, call(_, _)).Times(0);
    const feature application{feature::app{.prim = 0, .args = {x0}}};
    EXPECT_EQ(evaluator.evaluate(application, empty), view(3, {}));
}
