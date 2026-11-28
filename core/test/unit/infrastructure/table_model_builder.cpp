#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/table_model_builder.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockBuildModelGraph {
    MOCK_METHOD(model_graph, build, (const std::vector<feature_id>&), (const));
};

struct MockReadColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, read, (feature_id));
};

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

using test_table_model_builder_t =
    table_model_builder<NiceMock<MockBuildModelGraph>, NiceMock<MockReadColumn>, NiceMock<MockGetNativeType>>;

// Four training rows labelled 7, 8, 9, 9. Feature 3 has the classes 0, 0, 1, 1; feature 5 has 0, 1, 0, 0.
struct TableModelBuilderTest : public ::testing::Test {
    TableModelBuilderTest();
    std::shared_ptr<const cached_column> classes_of(std::vector<size_t> classes);
    native_column labels;
    native_type integer;
    model_graph graph;
    std::shared_ptr<const cached_column> third;
    std::shared_ptr<const cached_column> fifth;
    NiceMock<MockBuildModelGraph> build_model_graph;
    NiceMock<MockReadColumn> read_column;
    NiceMock<MockGetNativeType> get_native_type;
    test_table_model_builder_t builder{build_model_graph, read_column, get_native_type, labels, 4};

    void SetUp() override {
        ON_CALL(read_column, read(3)).WillByDefault(Return(third));
        ON_CALL(read_column, read(5)).WillByDefault(Return(fifth));
        ON_CALL(get_native_type, type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(integer));
    }
};

TableModelBuilderTest::TableModelBuilderTest()
    : labels(native_column::of(std::vector<int>{7, 8, 9, 9}))
    , integer(native_type::of<int, std::hash<int>>())
    , graph{.nodes = {derivation{derivation::input{.index = 0}}}, .types = {typeid(int)}, .outputs = {0}}
    , third(classes_of({0, 0, 1, 1}))
    , fifth(classes_of({0, 1, 0, 0}))
    , build_model_graph()
    , read_column()
    , get_native_type() {}

std::shared_ptr<const cached_column> TableModelBuilderTest::classes_of(std::vector<size_t> classes) {
    return std::make_shared<const cached_column>(cached_column{
        .type = typeid(int), .cells = std::any(std::vector<int>(classes.size(), 0)), .classes = classes, .hash = 0});
}

}

TEST_F(TableModelBuilderTest, TheGraphIsTheMembersGraph) {
    EXPECT_CALL(build_model_graph, build(std::vector<feature_id>{5, 3})).WillOnce(Return(graph));
    EXPECT_EQ(builder.build({5, 3}).graph, graph);
}

TEST_F(TableModelBuilderTest, TheColumnsAreTheMembersInOrder) {
    const table_model model = builder.build({5, 3});
    EXPECT_EQ(model.columns, (std::vector<std::shared_ptr<const cached_column>>{fifth, third}));
}

TEST_F(TableModelBuilderTest, EachDistinctTupleOfClassesMapsToItsFirstRowsLabel) {
    const table_model model = builder.build({3, 5});
    EXPECT_EQ(model.rows.size(), 3u);
    EXPECT_EQ(std::any_cast<int>(model.rows.at({0, 0})), 7);
    EXPECT_EQ(std::any_cast<int>(model.rows.at({0, 1})), 8);
    EXPECT_EQ(std::any_cast<int>(model.rows.at({1, 0})), 9);
}

TEST_F(TableModelBuilderTest, TuplesFollowTheMembersOrder) {
    const table_model model = builder.build({5, 3});
    EXPECT_EQ(std::any_cast<int>(model.rows.at({1, 0})), 8);
}

TEST_F(TableModelBuilderTest, NoMembersMapTheEmptyTupleToTheFirstLabel) {
    const table_model model = builder.build({});
    EXPECT_TRUE(model.columns.empty());
    EXPECT_EQ(model.rows.size(), 1u);
    EXPECT_EQ(std::any_cast<int>(model.rows.at({})), 7);
}
