#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/inheriting_bin_splitter.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockSplitBin {
    MOCK_METHOD(std::vector<bin_view>, split, (const bin_view&, const column_view&));
};

struct MockGetFeatureCandidates {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockCheckInherited {
    MOCK_METHOD(bool, has, (const std::vector<size_t>*), (const));
};

struct MockGetInheritedFeatures {
    MOCK_METHOD(const std::vector<feature>&, inherited, (const std::vector<size_t>*), (const));
};

struct MockClearInheritedFeatures {
    MOCK_METHOD(void, clear, ());
};

struct MockRecordInheritedFeatures {
    MOCK_METHOD(void, record, (const std::vector<size_t>*, std::vector<feature>));
};

// The scrutinee is the application sum(twice(x0), x0); a variable candidate observes another column.
struct InheritingBinSplitterTest : public ::testing::Test {
    InheritingBinSplitterTest();
    static bin_view one_row_bin(ctor_id ctor);
    value zero;
    feature x0;
    feature twice;
    feature sum;
    feature earlier;
    bin_view parent;
    bin_view first_child;
    bin_view second_child;
    column_view scrutinee_column;
    std::vector<candidate> candidates;
    std::vector<feature> parent_inherited;
    NiceMock<MockSplitBin> split_bin;
    NiceMock<MockGetFeatureCandidates> get_feature_candidates;
    NiceMock<MockCheckInherited> check_inherited;
    NiceMock<MockGetInheritedFeatures> get_inherited_features;
    NiceMock<MockClearInheritedFeatures> clear_inherited_features;
    NiceMock<MockRecordInheritedFeatures> record_inherited_features;
    inheriting_bin_splitter<
        NiceMock<MockSplitBin>, NiceMock<MockGetFeatureCandidates>, NiceMock<MockCheckInherited>,
        NiceMock<MockGetInheritedFeatures>, NiceMock<MockClearInheritedFeatures>,
        NiceMock<MockRecordInheritedFeatures>>
        splitter{split_bin, get_feature_candidates, check_inherited, get_inherited_features, clear_inherited_features,
                 record_inherited_features};

    void SetUp() override {
        ON_CALL(split_bin, split(_, _)).WillByDefault(Return(std::vector<bin_view>{first_child, second_child}));
        ON_CALL(get_feature_candidates, candidates()).WillByDefault(ReturnRef(candidates));
        ON_CALL(check_inherited, has(_)).WillByDefault(Return(true));
        ON_CALL(get_inherited_features, inherited(_)).WillByDefault(ReturnRef(parent_inherited));
    }
};

InheritingBinSplitterTest::InheritingBinSplitterTest()
    : zero{.ctor = 0, .args = {}}
    , x0{feature::var{.index = 0}}
    , twice{feature::app{.prim = 1, .args = {x0}}}
    , sum{feature::app{.prim = 0, .args = {twice, x0}}}
    , earlier{feature::app{.prim = 2, .args = {x0}}}
    , parent(one_row_bin(0))
    , first_child(one_row_bin(1))
    , second_child(one_row_bin(2))
    , scrutinee_column(column_view::of(typed_column{.type = 0, .cells = {zero}}))
    , candidates{candidate{.expression = x0, .cost = 1, .observed = parent.inputs[0]},
                 candidate{.expression = sum, .cost = 4, .observed = scrutinee_column}}
    , parent_inherited{}
    , split_bin()
    , get_feature_candidates()
    , check_inherited()
    , get_inherited_features()
    , clear_inherited_features()
    , record_inherited_features() {}

bin_view InheritingBinSplitterTest::one_row_bin(ctor_id ctor) {
    const value cell{.ctor = ctor, .args = {}};
    return bin_view::of(
        bin{.inputs = {typed_column{.type = 0, .cells = {cell}}}, .labels = typed_column{.type = 0, .cells = {cell}}});
}

}

TEST_F(InheritingBinSplitterTest, TheChildrenAreTheWrappedSplittersChildren) {
    EXPECT_CALL(split_bin, split(_, _)).Times(AnyNumber());
    EXPECT_CALL(split_bin, split(parent, scrutinee_column))
        .WillOnce(Return(std::vector<bin_view>{first_child, second_child}));
    EXPECT_EQ(splitter.split(parent, scrutinee_column), (std::vector<bin_view>{first_child, second_child}));
}

TEST_F(InheritingBinSplitterTest, EachChildInheritsTheScrutineeThenTheApplicationsInsideIt) {
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features, record(first_child.labels.rows.get(), std::vector<feature>{sum, twice}));
    EXPECT_CALL(record_inherited_features, record(second_child.labels.rows.get(), std::vector<feature>{sum, twice}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, WhatTheParentInheritedComesFirst) {
    parent_inherited = {earlier};
    EXPECT_CALL(get_inherited_features, inherited(_)).Times(AnyNumber());
    EXPECT_CALL(get_inherited_features, inherited(parent.labels.rows.get())).WillOnce(ReturnRef(parent_inherited));
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features,
                record(first_child.labels.rows.get(), std::vector<feature>{earlier, sum, twice}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, AnApplicationAlreadyInheritedIsPassedOnce) {
    parent_inherited = {twice};
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features, record(first_child.labels.rows.get(), std::vector<feature>{twice, sum}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, AnApplicationRepeatedInsideTheScrutineeIsPassedOnce) {
    const feature doubled{feature::app{.prim = 0, .args = {twice, twice}}};
    candidates[1].expression = doubled;
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features,
                record(first_child.labels.rows.get(), std::vector<feature>{doubled, twice}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, AVariableScrutineePassesOnlyWhatTheParentInherited) {
    parent_inherited = {earlier};
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features, record(first_child.labels.rows.get(), std::vector<feature>{earlier}));
    splitter.split(parent, parent.inputs[0]);
}

TEST_F(InheritingBinSplitterTest, AColumnNoCandidateObservedPassesOnlyWhatTheParentInherited) {
    parent_inherited = {earlier};
    const column_view bound_argument = column_view::of(typed_column{.type = 0, .cells = {zero}});
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features, record(first_child.labels.rows.get(), std::vector<feature>{earlier}));
    splitter.split(parent, bound_argument);
}

TEST_F(InheritingBinSplitterTest, ACandidateReadingEqualValuesFromAnotherColumnIsNotTheScrutinee) {
    const column_view equal_values = column_view::of(typed_column{.type = 0, .cells = {zero}});
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features, record(first_child.labels.rows.get(), std::vector<feature>{}));
    splitter.split(parent, equal_values);
}

TEST_F(InheritingBinSplitterTest, AParentWithoutARecordStartsANewModel) {
    ON_CALL(check_inherited, has(parent.labels.rows.get())).WillByDefault(Return(false));
    EXPECT_CALL(clear_inherited_features, clear());
    EXPECT_CALL(get_inherited_features, inherited(_)).Times(0);
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(AnyNumber());
    EXPECT_CALL(record_inherited_features, record(first_child.labels.rows.get(), std::vector<feature>{sum, twice}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, AParentWithARecordKeepsTheRecords) {
    EXPECT_CALL(clear_inherited_features, clear()).Times(0);
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, TheRecordsAreClearedBeforeTheChildrenAreRecorded) {
    ON_CALL(check_inherited, has(_)).WillByDefault(Return(false));
    ::testing::InSequence order;
    EXPECT_CALL(clear_inherited_features, clear());
    EXPECT_CALL(record_inherited_features, record(_, _)).Times(2);
    splitter.split(parent, scrutinee_column);
}
