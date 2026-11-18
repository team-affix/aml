#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/field_chooser.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindVariableCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&, var_id), (const));
};

struct MockChooseField {
    MOCK_METHOD(size_t, choose, (size_t));
};

// A bin whose variables 1 and 2 were just bound by a match.
struct FieldChooserTest : public ::testing::Test {
    FieldChooserTest();
    value falsity;
    value truth;
    feature first_field;
    feature second_field;
    bin data;
    candidate first_field_candidate;
    candidate second_field_candidate;
    NiceMock<MockFindVariableCandidates> find_variable_candidates;
    NiceMock<MockChooseField> choose_field;
    field_chooser<NiceMock<MockFindVariableCandidates>, NiceMock<MockChooseField>> chooser{
        find_variable_candidates, choose_field};
};

FieldChooserTest::FieldChooserTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , first_field{feature::var{.index = 1}}
    , second_field{feature::var{.index = 2}}
    , data{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                     typed_column{.type = 0, .cells = {&truth, &falsity}},
                     typed_column{.type = 0, .cells = {&falsity, &truth}}},
           .labels = {&falsity, &truth}}
    , first_field_candidate{.expression = &first_field, .cost = 1, .observed = data.scope[1]}
    , second_field_candidate{.expression = &second_field, .cost = 1, .observed = data.scope[2]}
    , find_variable_candidates()
    , choose_field() {}

}

TEST_F(FieldChooserTest, TheChosenHeterogeneousFieldFromTheFirstFieldOnIsTheScrutinee) {
    EXPECT_CALL(find_variable_candidates, find(data, 1))
        .WillOnce(Return(std::vector<candidate>{first_field_candidate, second_field_candidate}));
    EXPECT_CALL(choose_field, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(chooser.choose(data, 1), second_field_candidate);
}

TEST_F(FieldChooserTest, TheOnlyFieldIsStillChosenThrough) {
    ON_CALL(find_variable_candidates, find(data, 2))
        .WillByDefault(Return(std::vector<candidate>{second_field_candidate}));
    EXPECT_CALL(choose_field, choose(1)).WillOnce(Return(0));
    EXPECT_EQ(chooser.choose(data, 2), second_field_candidate);
}

TEST_F(FieldChooserTest, FieldsAreFoundFromTheGivenFirstField) {
    EXPECT_CALL(find_variable_candidates, find(data, 2))
        .WillOnce(Return(std::vector<candidate>{second_field_candidate}));
    ON_CALL(choose_field, choose(1)).WillByDefault(Return(0));
    chooser.choose(data, 2);
}
