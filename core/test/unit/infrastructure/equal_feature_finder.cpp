#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/equal_feature_finder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockFindHashedFeatures {
    MOCK_METHOD(const std::vector<feature_id>&, find, (size_t), (const));
};

struct MockReadColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, read, (feature_id));
};

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

// The made column holds the integers 3, 4, 3 and hashes to 77.
struct EqualFeatureFinderTest : public ::testing::Test {
    EqualFeatureFinderTest();
    std::shared_ptr<const cached_column> integers_of(std::vector<int> cells, std::vector<size_t> classes);
    native_type integer;
    cached_column made;
    std::vector<feature_id> hashed;
    std::vector<feature_id> none;
    NiceMock<MockFindHashedFeatures> find_hashed_features;
    NiceMock<MockReadColumn> read_column;
    NiceMock<MockGetNativeType> get_native_type;
    equal_feature_finder<NiceMock<MockFindHashedFeatures>, NiceMock<MockReadColumn>, NiceMock<MockGetNativeType>>
        finder{find_hashed_features, read_column, get_native_type};

    void SetUp() override {
        ON_CALL(find_hashed_features, find(_)).WillByDefault(ReturnRef(none));
        ON_CALL(find_hashed_features, find(77)).WillByDefault(ReturnRef(hashed));
        ON_CALL(get_native_type, type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(integer));
    }
};

EqualFeatureFinderTest::EqualFeatureFinderTest()
    : integer(native_type::of<int, std::hash<int>>())
    , made{.type = typeid(int), .cells = std::any(std::vector<int>{3, 4, 3}), .classes = {0, 1, 0}, .hash = 77}
    , hashed()
    , none()
    , find_hashed_features()
    , read_column()
    , get_native_type() {}

std::shared_ptr<const cached_column> EqualFeatureFinderTest::integers_of(
    std::vector<int> cells, std::vector<size_t> classes) {
    return std::make_shared<const cached_column>(
        cached_column{.type = typeid(int), .cells = std::any(std::move(cells)), .classes = classes, .hash = 77});
}

}

TEST_F(EqualFeatureFinderTest, NoFeatureWithTheHashFindsNone) {
    EXPECT_EQ(finder.find(made), std::nullopt);
}

TEST_F(EqualFeatureFinderTest, AFeatureWithAnEqualColumnIsFound) {
    hashed = {4};
    ON_CALL(read_column, read(4)).WillByDefault(Return(integers_of({3, 4, 3}, {0, 1, 0})));
    EXPECT_EQ(finder.find(made), std::optional<feature_id>(4));
}

TEST_F(EqualFeatureFinderTest, EqualClassesWithOtherValuesAreNotEqual) {
    hashed = {4};
    ON_CALL(read_column, read(4)).WillByDefault(Return(integers_of({5, 6, 5}, {0, 1, 0})));
    EXPECT_EQ(finder.find(made), std::nullopt);
}

TEST_F(EqualFeatureFinderTest, OtherClassesAreNotEqualAndNoValueIsCompared) {
    hashed = {4};
    ON_CALL(read_column, read(4)).WillByDefault(Return(integers_of({3, 3, 3}, {0, 0, 0})));
    EXPECT_CALL(get_native_type, type(_)).Times(0);
    EXPECT_EQ(finder.find(made), std::nullopt);
}

TEST_F(EqualFeatureFinderTest, AColumnOfAnotherTypeIsNotEqual) {
    hashed = {4};
    ON_CALL(read_column, read(4)).WillByDefault(Return(std::make_shared<const cached_column>(cached_column{
        .type = typeid(long), .cells = std::any(std::vector<long>{3, 4, 3}), .classes = {0, 1, 0}, .hash = 77})));
    EXPECT_EQ(finder.find(made), std::nullopt);
}

TEST_F(EqualFeatureFinderTest, TheFirstEqualFeatureInIndexOrderIsFound) {
    hashed = {6, 4, 5};
    ON_CALL(read_column, read(6)).WillByDefault(Return(integers_of({5, 6, 5}, {0, 1, 0})));
    ON_CALL(read_column, read(4)).WillByDefault(Return(integers_of({3, 4, 3}, {0, 1, 0})));
    ON_CALL(read_column, read(5)).WillByDefault(Return(integers_of({3, 4, 3}, {0, 1, 0})));
    EXPECT_EQ(finder.find(made), std::optional<feature_id>(4));
}

TEST_F(EqualFeatureFinderTest, ComparisonStopsAtTheFirstEqualFeature) {
    hashed = {4, 5};
    ON_CALL(read_column, read(4)).WillByDefault(Return(integers_of({3, 4, 3}, {0, 1, 0})));
    EXPECT_CALL(read_column, read(_)).Times(AnyNumber());
    EXPECT_CALL(read_column, read(5)).Times(0);
    finder.find(made);
}
