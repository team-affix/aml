#include <gtest/gtest.h>
#include "infrastructure/first_option_chooser.hpp"

namespace {

struct FirstOptionChooserTest : public ::testing::Test {
    first_option_chooser chooser;
};

}

TEST_F(FirstOptionChooserTest, TheFirstOptionIsChosen) {
    EXPECT_EQ(chooser.choose(3), 0u);
}
