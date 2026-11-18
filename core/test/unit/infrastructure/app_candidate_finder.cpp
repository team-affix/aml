#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/app_candidate_finder.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;

namespace {

struct MockApplyPrimitiveToColumns {
    MOCK_METHOD(column, apply, (prim_id, const std::vector<const column*>&), (const));
};

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column&), (const));
};

struct MockMakeAppFeature {
    MOCK_METHOD(const feature*, make_app, (prim_id, const std::vector<const feature*>&));
};

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature*), (const));
};

using finder_t = app_candidate_finder<
    NiceMock<MockApplyPrimitiveToColumns>,
    NiceMock<MockFindUniformValue>,
    NiceMock<MockMakeAppFeature>,
    NiceMock<MockSizeFeature>>;

// Primitive 2 applied to two boolean variables over two rows, giving a heterogeneous result of type 5.
struct AppCandidateFinderTest : public ::testing::Test {
    AppCandidateFinderTest();
    value falsity;
    value truth;
    feature left_variable;
    feature right_variable;
    feature application;
    candidate left;
    candidate right;
    column result;
    NiceMock<MockApplyPrimitiveToColumns> apply_primitive_to_columns;
    NiceMock<MockFindUniformValue> find_uniform_value;
    NiceMock<MockMakeAppFeature> make_app_feature;
    NiceMock<MockSizeFeature> size_feature;
    finder_t finder{apply_primitive_to_columns, find_uniform_value, make_app_feature, size_feature};

    void SetUp() override {
        ON_CALL(apply_primitive_to_columns, apply(2, _)).WillByDefault(Return(result));
        ON_CALL(find_uniform_value, find(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(make_app_feature, make_app(2, std::vector<const feature*>{&left_variable, &right_variable}))
            .WillByDefault(Return(&application));
        ON_CALL(size_feature, size(&application)).WillByDefault(Return(3));
    }
};

AppCandidateFinderTest::AppCandidateFinderTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , left_variable{feature::var{.index = 0}}
    , right_variable{feature::var{.index = 1}}
    , application{feature::app{.prim = 2, .args = {&left_variable, &right_variable}}}
    , left{.expression = &left_variable, .cost = 1, .observed = typed_column{.type = 0, .cells = {&falsity, &truth}}}
    , right{.expression = &right_variable, .cost = 1, .observed = typed_column{.type = 0, .cells = {&truth, &truth}}}
    , result{&falsity, &truth}
    , apply_primitive_to_columns()
    , find_uniform_value()
    , make_app_feature()
    , size_feature() {}

}

TEST_F(AppCandidateFinderTest, ThePrimitiveIsAppliedToTheArgumentsColumnsInOrder) {
    EXPECT_CALL(apply_primitive_to_columns, apply(2, ElementsAre(Pointee(left.observed.cells),
                                                                 Pointee(right.observed.cells))))
        .WillOnce(Return(result));
    finder.find(2, 5, {&left, &right});
}

TEST_F(AppCandidateFinderTest, AHeterogeneousApplicationIsACandidateWithItsCostAndResultType) {
    EXPECT_EQ(finder.find(2, 5, {&left, &right}),
              (candidate{.expression = &application, .cost = 3, .observed = typed_column{.type = 5, .cells = result}}));
}

TEST_F(AppCandidateFinderTest, AHomogeneousApplicationGivesNoCandidateAndMakesNoFeature) {
    ON_CALL(find_uniform_value, find(result)).WillByDefault(Return(&truth));
    EXPECT_CALL(make_app_feature, make_app(_, _)).Times(0);
    EXPECT_FALSE(finder.find(2, 5, {&left, &right}).has_value());
}
