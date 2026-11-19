#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/primitive_applier.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetPrimitiveType {
    MOCK_METHOD(const primitive_type&, type, (prim_id), (const));
};

struct MockChooseArguments {
    MOCK_METHOD(std::vector<size_t>, choose, (prim_id, const std::vector<type_id>&));
};

struct MockRecordApplication {
    MOCK_METHOD(void, record, (const prim_application&));
};

struct MockFindAppCandidate {
    MOCK_METHOD(std::optional<candidate>, find, (prim_id, type_id, const std::vector<size_t>&, size_t));
};

struct MockTryInsertFeature {
    MOCK_METHOD(void, try_insert, (candidate));
};

// Primitive 5 is binary over type 0 with result type 4; its chosen arguments are operands 0 and 1.
struct PrimitiveApplierTest : public ::testing::Test {
    PrimitiveApplierTest();
    value falsity;
    value truth;
    feature application;
    candidate application_candidate;
    primitive_type type;
    NiceMock<MockGetPrimitiveType> get_primitive_type;
    NiceMock<MockChooseArguments> choose_arguments;
    NiceMock<MockRecordApplication> record_application;
    NiceMock<MockFindAppCandidate> find_app_candidate;
    NiceMock<MockTryInsertFeature> try_insert_feature;
    primitive_applier<
        NiceMock<MockGetPrimitiveType>, NiceMock<MockChooseArguments>, NiceMock<MockRecordApplication>,
        NiceMock<MockFindAppCandidate>, NiceMock<MockTryInsertFeature>>
        applier{get_primitive_type, choose_arguments, record_application, find_app_candidate, try_insert_feature};

    void SetUp() override {
        ON_CALL(get_primitive_type, type(5)).WillByDefault(ReturnRef(type));
        ON_CALL(choose_arguments, choose(5, type.arg_types)).WillByDefault(Return(std::vector<size_t>{0, 1}));
        ON_CALL(find_app_candidate, find(5, 4, _, _)).WillByDefault(Return(application_candidate));
    }
};

PrimitiveApplierTest::PrimitiveApplierTest()
    : falsity{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , truth{.ctor = {.type = 0, .tag = 1}, .args = {}}
    , application{feature::app{.prim = 5, .args = {}}}
    , application_candidate{
          .expression = &application, .cost = 3, .observed = typed_column{.type = 4, .cells = {&truth, &falsity}}}
    , type{.arg_types = {0, 0}, .result_type = 4}
    , get_primitive_type()
    , choose_arguments()
    , record_application()
    , find_app_candidate()
    , try_insert_feature() {}

}

TEST_F(PrimitiveApplierTest, TheArgumentsAreChosenForTheSignatureThenLoggedThenApplied) {
    InSequence order;
    EXPECT_CALL(choose_arguments, choose(5, type.arg_types)).WillOnce(Return(std::vector<size_t>{0, 1}));
    EXPECT_CALL(record_application, record(prim_application{.prim = 5, .args = {0, 1}}));
    EXPECT_CALL(find_app_candidate, find(5, 4, std::vector<size_t>{0, 1}, 2)).WillOnce(Return(application_candidate));
    applier.apply(5, 2);
}

TEST_F(PrimitiveApplierTest, AFoundApplicationJoinsTheFeatures) {
    EXPECT_CALL(try_insert_feature, try_insert(application_candidate));
    applier.apply(5, 2);
}

TEST_F(PrimitiveApplierTest, AnApplicationThatIsNotFoundIsStillLoggedButDoesNotJoinTheFeatures) {
    ON_CALL(find_app_candidate, find(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(record_application, record(_));
    EXPECT_CALL(try_insert_feature, try_insert(_)).Times(0);
    applier.apply(5, 2);
}

TEST_F(PrimitiveApplierTest, TheTypeOfTheChosenPrimitiveIsUsed) {
    EXPECT_CALL(get_primitive_type, type(5)).WillOnce(ReturnRef(type));
    applier.apply(5, 2);
}

TEST_F(PrimitiveApplierTest, EveryApplicationIsLoggedExactlyOnce) {
    EXPECT_CALL(record_application, record(_)).Times(1);
    applier.apply(5, 2);
}
