#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/application_chooser.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
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

struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (size_t));
};

using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

operand feature_operand(feature_id id) {
    return operand{operand::from_feature{.id = id}};
}

operand constant_operand(constant_id id) {
    return operand{operand::from_constant{.id = id}};
}

// Primitive 4 takes (int, int), primitive 6 takes (int, bool). The int constants are 20 and 21; bool has none.
struct ApplicationChooserTest : public ::testing::Test {
    ApplicationChooserTest();
    native_primitive int_int;
    native_primitive int_bool;
    std::vector<constant_id> ints;
    std::vector<constant_id> none;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    NiceMock<MockGetConstantsOfType> get_constants_of_type;
    NiceMock<MockChooseOption> choose_option;
    application_chooser<
        NiceMock<MockGetNativePrimitive>, NiceMock<MockGetConstantsOfType>, NiceMock<MockChooseOption>> chooser{
        get_native_primitive, get_constants_of_type, choose_option};

    void SetUp() override {
        ON_CALL(get_native_primitive, primitive(4)).WillByDefault(ReturnRef(int_int));
        ON_CALL(get_native_primitive, primitive(6)).WillByDefault(ReturnRef(int_bool));
        ON_CALL(get_constants_of_type, of_type(_)).WillByDefault(ReturnRef(none));
        ON_CALL(get_constants_of_type, of_type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(ints));
    }
};

ApplicationChooserTest::ApplicationChooserTest()
    : int_int{.arg_types = {typeid(int), typeid(int)}, .result_type = typeid(int), .apply = nullptr}
    , int_bool{.arg_types = {typeid(int), typeid(bool)}, .result_type = typeid(int), .apply = nullptr}
    , ints{20, 21}
    , none()
    , get_native_primitive()
    , get_constants_of_type()
    , choose_option() {}

}

TEST_F(ApplicationChooserTest, ThePrimitiveIsChosenAmongTheFillableOnesFirst) {
    const pool_t pool{{typeid(int), {1}}, {typeid(bool), {2}}};
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_option, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(choose_option, choose(3)).WillOnce(Return(0));
    EXPECT_CALL(choose_option, choose(1)).WillOnce(Return(0));
    const derivation chosen = chooser.choose(pool, {4, 6});
    EXPECT_EQ(
        chosen, (derivation{derivation::application{.prim = 6, .args = {feature_operand(1), feature_operand(2)}}}));
}

TEST_F(ApplicationChooserTest, AnArgumentOffersItsTypesFeaturesThenItsConstants) {
    const pool_t pool{{typeid(int), {1, 3}}};
    ON_CALL(choose_option, choose(1)).WillByDefault(Return(0));
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_option, choose(4)).WillOnce(Return(3));
    EXPECT_CALL(choose_option, choose(2)).WillOnce(Return(1));
    const derivation chosen = chooser.choose(pool, {4});
    EXPECT_EQ(
        chosen, (derivation{derivation::application{.prim = 4, .args = {constant_operand(21), feature_operand(3)}}}));
}

TEST_F(ApplicationChooserTest, AfterAFeatureTheLastArgumentStillOffersConstants) {
    const pool_t pool{{typeid(int), {1, 3}}};
    ON_CALL(choose_option, choose(1)).WillByDefault(Return(0));
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_option, choose(4)).WillOnce(Return(0));
    EXPECT_CALL(choose_option, choose(4)).WillOnce(Return(2));
    const derivation chosen = chooser.choose(pool, {4});
    EXPECT_EQ(
        chosen, (derivation{derivation::application{.prim = 4, .args = {feature_operand(1), constant_operand(20)}}}));
}

TEST_F(ApplicationChooserTest, WhileNoFeatureIsChosenTheLastFeedableArgumentOffersFeaturesAlone) {
    const pool_t pool{{typeid(int), {1}}};
    ON_CALL(choose_option, choose(1)).WillByDefault(Return(0));
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_option, choose(3)).WillOnce(Return(2));
    EXPECT_CALL(choose_option, choose(1)).WillOnce(Return(0));
    const derivation chosen = chooser.choose(pool, {4});
    EXPECT_EQ(
        chosen, (derivation{derivation::application{.prim = 4, .args = {constant_operand(21), feature_operand(1)}}}));
}

TEST_F(ApplicationChooserTest, AFirstArgumentThatIsTheLastFeedableOneTakesAFeature) {
    // Primitive 8 takes (int, double); the pool holds no doubles, so the first argument is the last feedable one.
    const pool_t pool{{typeid(int), {1}}, {typeid(bool), {}}};
    native_primitive int_double{
        .arg_types = {typeid(int), typeid(double)}, .result_type = typeid(int), .apply = nullptr};
    const std::vector<constant_id> doubles{30};
    ON_CALL(get_native_primitive, primitive(8)).WillByDefault(ReturnRef(int_double));
    ON_CALL(get_constants_of_type, of_type(std::type_index(typeid(double)))).WillByDefault(ReturnRef(doubles));
    EXPECT_CALL(choose_option, choose(_)).WillRepeatedly(Return(0));
    const derivation chosen = chooser.choose(pool, {8});
    EXPECT_EQ(
        chosen, (derivation{derivation::application{.prim = 8, .args = {feature_operand(1), constant_operand(30)}}}));
}
