#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_candidate_finder.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetConstants {
    MOCK_METHOD(const std::vector<constant_operand>&, constants, (), (const));
};

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature*), (const));
};

// Two constants of different types over a bin of three rows.
struct ConstantCandidateFinderTest : public ::testing::Test {
    ConstantCandidateFinderTest();
    value truth;
    value zero;
    feature true_constant;
    feature zero_constant;
    bin data;
    std::vector<constant_operand> constants;
    NiceMock<MockGetConstants> get_constants;
    NiceMock<MockSizeFeature> size_feature;
    constant_candidate_finder<NiceMock<MockGetConstants>, NiceMock<MockSizeFeature>> finder{
        get_constants, size_feature};

    void SetUp() override {
        ON_CALL(get_constants, constants()).WillByDefault(ReturnRef(constants));
        ON_CALL(size_feature, size(_)).WillByDefault(Return(1));
    }
};

ConstantCandidateFinderTest::ConstantCandidateFinderTest()
    : truth{.ctor = 1, .fields = {}}
    , zero{.ctor = 2, .fields = {}}
    , true_constant{feature::constant{.literal = &truth}}
    , zero_constant{feature::constant{.literal = &zero}}
    , data{.scope = {}, .labels = {&truth, &zero, &truth}}
    , constants{
          constant_operand{.expression = &true_constant, .literal = &truth, .type = 0},
          constant_operand{.expression = &zero_constant, .literal = &zero, .type = 1}}
    , get_constants()
    , size_feature() {}

}

TEST_F(ConstantCandidateFinderTest, EachConstantObservesItsLiteralOnEveryRowInStoreOrder) {
    EXPECT_EQ(finder.find(data),
              (std::vector<candidate>{
                  candidate{
                      .expression = &true_constant,
                      .cost = 1,
                      .observed = typed_column{.type = 0, .cells = {&truth, &truth, &truth}}},
                  candidate{
                      .expression = &zero_constant,
                      .cost = 1,
                      .observed = typed_column{.type = 1, .cells = {&zero, &zero, &zero}}}}));
}

TEST_F(ConstantCandidateFinderTest, ACandidateCostsItsFeaturesSize) {
    EXPECT_CALL(size_feature, size(_)).Times(AnyNumber());
    EXPECT_CALL(size_feature, size(&zero_constant)).WillOnce(Return(4));
    EXPECT_EQ(finder.find(data).back().cost, 4u);
}

TEST_F(ConstantCandidateFinderTest, NoConstantsGiveNoCandidates) {
    constants.clear();
    EXPECT_TRUE(finder.find(data).empty());
}
