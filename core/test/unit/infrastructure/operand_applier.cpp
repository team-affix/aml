#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/operand_applier.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetConstants {
    MOCK_METHOD(const std::vector<constant_operand>&, constants, (), (const));
};

struct MockGetFeatureCandidates {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockApplyPrimitive {
    MOCK_METHOD(const value*, apply, (prim_id, const std::vector<const value*>&), (const));
};

// Operands: the constant true (0), then features x0 (1) and x1 (2) over three rows.
struct OperandApplierTest : public ::testing::Test {
    OperandApplierTest();
    value falsity;
    value truth;
    std::vector<constant_operand> constants;
    std::vector<candidate> features;
    NiceMock<MockGetConstants> get_constants;
    NiceMock<MockGetFeatureCandidates> get_feature_candidates;
    NiceMock<MockApplyPrimitive> apply_primitive;
    operand_applier<NiceMock<MockGetConstants>, NiceMock<MockGetFeatureCandidates>, NiceMock<MockApplyPrimitive>>
        applier{get_constants, get_feature_candidates, apply_primitive};

    void SetUp() override {
        ON_CALL(get_constants, constants()).WillByDefault(ReturnRef(constants));
        ON_CALL(get_feature_candidates, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(apply_primitive, apply(_, _)).WillByDefault(Return(&falsity));
    }
};

OperandApplierTest::OperandApplierTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , constants{constant_operand{.expression = nullptr, .literal = &truth, .type = 0}}
    , features{
          candidate{.expression = nullptr, .cost = 1,
                    .observed = typed_column{.type = 0, .cells = {&falsity, &truth, &truth}}},
          candidate{.expression = nullptr, .cost = 1,
                    .observed = typed_column{.type = 0, .cells = {&truth, &truth, &falsity}}}}
    , get_constants()
    , get_feature_candidates()
    , apply_primitive() {}

}

TEST_F(OperandApplierTest, ThePrimitiveIsAppliedToEachRowsFeatureValues) {
    InSequence order;
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&falsity, &truth})).WillOnce(Return(&falsity));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &truth})).WillOnce(Return(&truth));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &falsity})).WillOnce(Return(&falsity));
    EXPECT_EQ(applier.apply(2, {1, 2}, 3), (column{&falsity, &truth, &falsity}));
}

TEST_F(OperandApplierTest, AConstantGivesItsLiteralOnEveryRow) {
    InSequence order;
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &falsity}));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &truth}));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &truth}));
    applier.apply(2, {0, 1}, 3);
}

TEST_F(OperandApplierTest, NoRowsGiveAnEmptyColumn) {
    EXPECT_TRUE(applier.apply(0, {1}, 0).empty());
}
