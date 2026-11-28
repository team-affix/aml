#include <gtest/gtest.h>
#include "infrastructure/set_key_store.hpp"

using namespace AML;

namespace {

struct SetKeyStoreTest : public ::testing::Test {
    set_key_store keys;
};

}

TEST_F(SetKeyStoreTest, ANewKeyIsRecorded) {
    EXPECT_TRUE(keys.try_record(42, 5));
}

TEST_F(SetKeyStoreTest, AKeyRecordedAtTheSameSizeIsNotRecordedAgain) {
    keys.try_record(42, 5);
    EXPECT_FALSE(keys.try_record(42, 5));
}

TEST_F(SetKeyStoreTest, AKeyRecordedAtASmallerSizeIsNotRecordedAgain) {
    keys.try_record(42, 5);
    EXPECT_FALSE(keys.try_record(42, 6));
}

TEST_F(SetKeyStoreTest, AKeyIsRecordedAgainAtASmallerSize) {
    keys.try_record(42, 5);
    EXPECT_TRUE(keys.try_record(42, 4));
}

TEST_F(SetKeyStoreTest, TheSmallerSizeIsKept) {
    keys.try_record(42, 5);
    keys.try_record(42, 3);
    EXPECT_FALSE(keys.try_record(42, 4));
    EXPECT_FALSE(keys.try_record(42, 3));
    EXPECT_TRUE(keys.try_record(42, 2));
}

TEST_F(SetKeyStoreTest, ALargerSizeDoesNotReplaceTheSmaller) {
    keys.try_record(42, 3);
    keys.try_record(42, 9);
    EXPECT_FALSE(keys.try_record(42, 4));
}

TEST_F(SetKeyStoreTest, KeysAreKeptApart) {
    keys.try_record(42, 5);
    EXPECT_TRUE(keys.try_record(43, 5));
}

TEST_F(SetKeyStoreTest, ZeroIsAKeyLikeAnyOther) {
    EXPECT_TRUE(keys.try_record(0, 0));
    EXPECT_FALSE(keys.try_record(0, 0));
}
