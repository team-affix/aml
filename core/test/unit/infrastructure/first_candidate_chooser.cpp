#include <gtest/gtest.h>
#include "infrastructure/first_candidate_chooser.hpp"

namespace {

struct FirstCandidateChooserTest : public ::testing::Test {
    first_candidate_chooser chooser;
};

}

TEST_F(FirstCandidateChooserTest, TheFirstCandidateIsChosen) {
    const candidate any{.expression = nullptr, .cost = 1, .observed = typed_column{.type = 0, .cells = {}}};
    EXPECT_EQ(chooser.choose({any, any, any}), 0u);
}
