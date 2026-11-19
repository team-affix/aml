#include <variant>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/primitive_signature_reader.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockPrimitive {
    MOCK_METHOD(primitive_signature, signature, (), (const));
};

struct OtherMockPrimitive {
    MOCK_METHOD(primitive_signature, signature, (), (const));
};

using test_primitive = std::variant<const NiceMock<MockPrimitive>*, const NiceMock<OtherMockPrimitive>*>;

struct MockGetPrimitive {
    MOCK_METHOD(const test_primitive&, primitive_at, (prim_id), (const));
};

struct PrimitiveSignatureReaderTest : public ::testing::Test {
    PrimitiveSignatureReaderTest();
    NiceMock<MockPrimitive> first;
    NiceMock<OtherMockPrimitive> second;
    test_primitive first_entry;
    test_primitive second_entry;
    NiceMock<MockGetPrimitive> get_primitive;
    primitive_signature_reader<NiceMock<MockGetPrimitive>> reader{get_primitive};

    void SetUp() override {
        ON_CALL(get_primitive, primitive_at(0)).WillByDefault(ReturnRef(first_entry));
        ON_CALL(get_primitive, primitive_at(1)).WillByDefault(ReturnRef(second_entry));
    }
};

PrimitiveSignatureReaderTest::PrimitiveSignatureReaderTest()
    : first()
    , second()
    , first_entry(&first)
    , second_entry(&second)
    , get_primitive() {}

}

TEST_F(PrimitiveSignatureReaderTest, TheSignatureComesFromThePrimitiveAtThatId) {
    const primitive_signature signature{.arg_types = {3}, .result_type = 4};
    EXPECT_CALL(second, signature()).WillOnce(Return(signature));
    EXPECT_EQ(reader.signature(1), signature);
}

TEST_F(PrimitiveSignatureReaderTest, OnlyThePrimitiveAtThatIdIsAsked) {
    EXPECT_CALL(second, signature()).Times(0);
    EXPECT_CALL(first, signature()).WillOnce(Return(primitive_signature{.arg_types = {}, .result_type = 0}));
    reader.signature(0);
}

TEST_F(PrimitiveSignatureReaderTest, ThePrimitiveIsLookedUpByItsId) {
    EXPECT_CALL(get_primitive, primitive_at(1)).WillOnce(ReturnRef(second_entry));
    reader.signature(1);
}
