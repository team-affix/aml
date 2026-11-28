#include <optional>
#include <random>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/derivation_sampler.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNativePrimitive {
    MOCK_METHOD(const native_primitive&, primitive, (prim_id), (const));
};

struct MockGetConstantsOfType {
    MOCK_METHOD(const std::vector<constant_id>&, of_type, (std::type_index), (const));
};

using test_derivation_sampler_t =
    derivation_sampler<NiceMock<MockGetNativePrimitive>, NiceMock<MockGetConstantsOfType>, std::mt19937_64>;
using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

// Primitive 0 takes (bool, int); primitive 1 takes int. The int constants are constants 20 and 21. Both primitives
// are eligible unless a test says otherwise.
struct DerivationSamplerTest : public ::testing::Test {
    DerivationSamplerTest();
    derivation::application sampled(const pool_t& pool);
    std::vector<prim_id> both;
    native_primitive pair;
    native_primitive single;
    std::vector<constant_id> constants;
    std::vector<constant_id> none;
    std::mt19937_64 random_engine;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    NiceMock<MockGetConstantsOfType> get_constants_of_type;
    test_derivation_sampler_t sampler{get_native_primitive, get_constants_of_type, random_engine};

    void SetUp() override {
        ON_CALL(get_native_primitive, primitive(0)).WillByDefault(ReturnRef(pair));
        ON_CALL(get_native_primitive, primitive(1)).WillByDefault(ReturnRef(single));
        ON_CALL(get_constants_of_type, of_type(_)).WillByDefault(ReturnRef(none));
        ON_CALL(get_constants_of_type, of_type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(constants));
    }
};

DerivationSamplerTest::DerivationSamplerTest()
    : both{0, 1}
    , pair{.arg_types = {typeid(bool), typeid(int)}, .result_type = typeid(bool), .apply = nullptr}
    , single{.arg_types = {typeid(int)}, .result_type = typeid(int), .apply = nullptr}
    , constants{20, 21}
    , none()
    , random_engine(5)
    , get_native_primitive()
    , get_constants_of_type() {}

derivation::application DerivationSamplerTest::sampled(const pool_t& pool) {
    return std::get<derivation::application>(sampler.sample(pool, both).value().content);
}

}

namespace {

bool is_feature(const operand& arg, feature_id id) {
    return arg == operand{operand::from_feature{.id = id}};
}

bool is_constant(const operand& arg, constant_id id) {
    return arg == operand{operand::from_constant{.id = id}};
}

}

TEST_F(DerivationSamplerTest, NoEligiblePrimitivesSampleNothing) {
    EXPECT_EQ(sampler.sample(pool_t{{typeid(int), {2}}}, {}), std::nullopt);
}

TEST_F(DerivationSamplerTest, OnlyEligiblePrimitivesAreSampled) {
    for (int draw = 0; draw < 100; ++draw) {
        const std::optional<derivation> origin = sampler.sample(pool_t{{typeid(int), {3}}}, {1});
        EXPECT_EQ(std::get<derivation::application>(origin.value().content).prim, 1u);
    }
}

TEST_F(DerivationSamplerTest, EachArgumentIsAPoolFeatureOrAConstantOfItsType) {
    const pool_t pool{{typeid(bool), {1, 2}}, {typeid(int), {3}}};
    for (int draw = 0; draw < 200; ++draw) {
        const derivation::application application = sampled(pool);
        const operand& last = application.args.back();
        EXPECT_TRUE(is_feature(last, 3) || is_constant(last, 20) || is_constant(last, 21));
        if (application.prim == 1) {
            EXPECT_EQ(application.args.size(), 1u);
            continue;
        }
        ASSERT_EQ(application.args.size(), 2u);
        EXPECT_TRUE(is_feature(application.args[0], 1) || is_feature(application.args[0], 2));
    }
}

TEST_F(DerivationSamplerTest, EveryApplicationHasAFeatureOperand) {
    const pool_t pool{{typeid(int), {3}}};
    for (int draw = 0; draw < 300; ++draw) {
        const std::optional<derivation> origin = sampler.sample(pool, {1});
        EXPECT_TRUE(is_feature(std::get<derivation::application>(origin.value().content).args[0], 3));
    }
}

TEST_F(DerivationSamplerTest, EveryPrimitiveAndEveryCandidateIsSampled) {
    const pool_t pool{{typeid(bool), {1, 2}}, {typeid(int), {3}}};
    std::set<prim_id> primitives;
    std::set<size_t> last_kinds;
    for (int draw = 0; draw < 500; ++draw) {
        const derivation::application application = sampled(pool);
        primitives.insert(application.prim);
        last_kinds.insert(application.args.back().source.index());
    }
    EXPECT_EQ(primitives, (std::set<prim_id>{0, 1}));
    EXPECT_EQ(last_kinds, (std::set<size_t>{0, 1}));
}

TEST_F(DerivationSamplerTest, AnArgumentTypeWithNoCandidateSamplesNothing) {
    EXPECT_EQ(sampler.sample(pool_t{{typeid(int), {3}}}, {0}), std::nullopt);
}

TEST_F(DerivationSamplerTest, PrimitivesAreChosenUniformlyAmongTheEligible) {
    const pool_t pool{{typeid(bool), {1}}, {typeid(int), {3}}};
    int first = 0;
    for (int draw = 0; draw < 4000; ++draw)
        first += sampled(pool).prim == 0 ? 1 : 0;
    EXPECT_NEAR(first, 2000, 200);
}
