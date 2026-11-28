#include <any>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_atom_seeder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace {

struct MockInsertConstantRecord {
    MOCK_METHOD(constant_id, insert, (const constant_record&));
};

struct ConstantAtomSeederTest : public ::testing::Test {
    NiceMock<MockInsertConstantRecord> insert_constant_record;
    constant_atom_seeder<NiceMock<MockInsertConstantRecord>> seeder{insert_constant_record};
};

}

TEST_F(ConstantAtomSeederTest, EachConstantIsAnAtomByItsPositionInOrder) {
    InSequence order;
    EXPECT_CALL(insert_constant_record, insert(AllOf(
        Field(&constant_record::origin, constant_derivation{constant_derivation::atom{.index = 0}}),
        Field(&constant_record::type, std::type_index(typeid(int))))));
    EXPECT_CALL(insert_constant_record, insert(AllOf(
        Field(&constant_record::origin, constant_derivation{constant_derivation::atom{.index = 1}}),
        Field(&constant_record::type, std::type_index(typeid(bool))))));
    seeder.seed({native_constant::of(3), native_constant::of(true)});
}

TEST_F(ConstantAtomSeederTest, NoConstantsSeedNothing) {
    EXPECT_CALL(insert_constant_record, insert(_)).Times(0);
    seeder.seed({});
}
