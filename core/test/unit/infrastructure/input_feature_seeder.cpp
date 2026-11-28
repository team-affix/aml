#include <any>
#include <memory>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/input_feature_seeder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockComputeColumn {
    MOCK_METHOD(std::any, compute, (const derivation&, const std::vector<const std::any*>&), (const));
};

struct MockMakeCachedColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, make, (std::type_index, std::any), (const));
};

struct MockRecordFeature {
    MOCK_METHOD(feature_id, record, (const derivation&, std::shared_ptr<const cached_column>));
};

struct MockDetectUniformColumn {
    MOCK_METHOD(bool, detect, (const cached_column&), (const));
};

using test_input_feature_seeder_t = input_feature_seeder<
    NiceMock<MockComputeColumn>, NiceMock<MockMakeCachedColumn>, NiceMock<MockRecordFeature>,
    NiceMock<MockDetectUniformColumn>>;

// Input 0 (bool) varies, input 1 (int) is uniform, input 2 (double) varies; each is recorded as 10 + its index.
struct InputFeatureSeederTest : public ::testing::Test {
    InputFeatureSeederTest();
    std::shared_ptr<const cached_column> cached_of(std::type_index type);
    std::vector<native_column> inputs;
    std::vector<std::shared_ptr<const cached_column>> made;
    NiceMock<MockComputeColumn> compute_column;
    NiceMock<MockMakeCachedColumn> make_cached_column;
    NiceMock<MockRecordFeature> record_feature;
    NiceMock<MockDetectUniformColumn> detect_uniform_column;
    test_input_feature_seeder_t seeder{compute_column, make_cached_column, record_feature, detect_uniform_column};

    void SetUp() override {
        for (size_t index = 0; index < inputs.size(); ++index) {
            const derivation origin{derivation::input{.index = static_cast<var_id>(index)}};
            ON_CALL(make_cached_column, make(inputs[index].type, _)).WillByDefault(Return(made[index]));
            ON_CALL(record_feature, record(origin, made[index])).WillByDefault(Return(10 + index));
        }
        ON_CALL(detect_uniform_column, detect(::testing::Ref(*made[1]))).WillByDefault(Return(true));
    }
};

InputFeatureSeederTest::InputFeatureSeederTest()
    : inputs{native_column::of(std::vector<bool>{false, true}), native_column::of(std::vector<int>{4, 4}),
             native_column::of(std::vector<double>{1.0, 2.0})}
    , made{cached_of(typeid(bool)), cached_of(typeid(int)), cached_of(typeid(double))}
    , compute_column()
    , make_cached_column()
    , record_feature()
    , detect_uniform_column() {}

std::shared_ptr<const cached_column> InputFeatureSeederTest::cached_of(std::type_index type) {
    return std::make_shared<const cached_column>(
        cached_column{.type = type, .cells = std::any(), .classes = {}, .hash = 0});
}

}

TEST_F(InputFeatureSeederTest, EveryInputIsRecordedInOrder) {
    EXPECT_CALL(record_feature, record(_, _)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(record_feature, record(derivation{derivation::input{.index = 0}}, made[0])).WillOnce(Return(10));
    EXPECT_CALL(record_feature, record(derivation{derivation::input{.index = 1}}, made[1])).WillOnce(Return(11));
    EXPECT_CALL(record_feature, record(derivation{derivation::input{.index = 2}}, made[2])).WillOnce(Return(12));
    seeder.seed(inputs);
}

TEST_F(InputFeatureSeederTest, TheVariedInputsAreTheStartingSet) {
    EXPECT_EQ(seeder.seed(inputs), (std::vector<feature_id>{10, 12}));
}

TEST_F(InputFeatureSeederTest, EachInputsColumnIsComputedFromItsOrigin) {
    EXPECT_CALL(compute_column, compute(_, _)).Times(AnyNumber());
    EXPECT_CALL(compute_column, compute(derivation{derivation::input{.index = 1}}, ElementsAre()));
    seeder.seed(inputs);
}

TEST_F(InputFeatureSeederTest, EachColumnIsMadeWithItsInputsType) {
    EXPECT_CALL(make_cached_column, make(_, _)).Times(AnyNumber());
    EXPECT_CALL(make_cached_column, make(std::type_index(typeid(int)), _)).WillOnce(Return(made[1]));
    seeder.seed(inputs);
}

TEST_F(InputFeatureSeederTest, NoInputsSeedNothing) {
    EXPECT_CALL(record_feature, record(_, _)).Times(0);
    EXPECT_TRUE(seeder.seed({}).empty());
}
