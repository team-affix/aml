// elaborate_command_handler: modules/statements → printed CHC goal string.

#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "elaborate_test_helpers.hpp"

struct ElaborateCommandHandlerTest : public ::testing::Test {
    std::string out_path(const char* name) {
        return std::string("build/") + name;
    }
};

// ---------------------------------------------------------------------------
// Happy path / shape
// ---------------------------------------------------------------------------

TEST_F(ElaborateCommandHandlerTest, EmptyInputsEqModelMainOnly) {
    const std::string got = run_elaborate({}, {}, out_path("elab_empty.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 1u);
    EXPECT_NE(got.find("eq(Model,"), std::string::npos);
    EXPECT_NE(got.find("Main"), std::string::npos);
    EXPECT_EQ(got.find("normalize("), std::string::npos);
    expect_no_list_sugar(got);
}

TEST_F(ElaborateCommandHandlerTest, LogicDefModulesOnly) {
    const std::string got = run_elaborate(
        {"parser/fixtures/logic.def"}, {}, out_path("elab_logic.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 1u);
    EXPECT_NE(got.find("eq(Model,"), std::string::npos);
    EXPECT_NE(got.find("abs("), std::string::npos);
    EXPECT_NE(got.find("app("), std::string::npos);
    EXPECT_NE(got.find("var("), std::string::npos);
    EXPECT_EQ(got.find("normalize("), std::string::npos);
    expect_no_list_sugar(got);
}

TEST_F(ElaborateCommandHandlerTest, IdDefOneStatement) {
    const std::string got = run_elaborate(
        {"cli/test/fixtures/id.def"},
        {"cli/test/fixtures/id_id.stmts"},
        out_path("elab_id_one.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 2u);
    EXPECT_EQ(got.find("eq(Model,"), 0u);
    EXPECT_NE(got.find("normalize("), std::string::npos);
    EXPECT_LT(got.find("eq(Model,"), got.find("normalize("));
    EXPECT_NE(got.find("Model"), std::string::npos);
    expect_named_print_invariants(got);
}

TEST_F(ElaborateCommandHandlerTest, ThreeStatementsPreserveOrder) {
    const std::string got = run_elaborate(
        {"cli/test/fixtures/id.def"},
        {"cli/test/fixtures/three.stmts"},
        out_path("elab_three.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 4u);
    EXPECT_EQ(count_substr(got, "normalize("), 3u);
    const size_t n0 = got.find("normalize(");
    const size_t n1 = got.find("normalize(", n0 + 1);
    const size_t n2 = got.find("normalize(", n1 + 1);
    ASSERT_NE(n0, std::string::npos);
    ASSERT_NE(n1, std::string::npos);
    ASSERT_NE(n2, std::string::npos);
    EXPECT_LT(n0, n1);
    EXPECT_LT(n1, n2);
}

TEST_F(ElaborateCommandHandlerTest, TwoModuleFilesWithNotTrue) {
    const std::string got = run_elaborate(
        {"parser/fixtures/bool_nat.decl", "parser/fixtures/logic.def"},
        {"cli/test/fixtures/not_true.stmts"},
        out_path("elab_two_mods.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 2u);
    EXPECT_NE(got.find("normalize("), std::string::npos);
    expect_no_list_sugar(got);
}

TEST_F(ElaborateCommandHandlerTest, StatementsOnlyChurchNats) {
    const std::string got = run_elaborate(
        {},
        {"cli/test/fixtures/church_nats.stmts"},
        out_path("elab_church.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 2u);
    EXPECT_NE(got.find("eq(Model,"), std::string::npos);
    EXPECT_NE(got.find("normalize("), std::string::npos);
    EXPECT_NE(got.find("abs(abs("), std::string::npos);
    expect_no_list_sugar(got);
}

TEST_F(ElaborateCommandHandlerTest, MultiModuleMultiStmtGoalCount) {
    const std::string got = run_elaborate(
        {"parser/fixtures/bool_nat.decl", "cli/test/fixtures/id.def"},
        {"cli/test/fixtures/three.stmts"},
        out_path("elab_multi.txt"));
    EXPECT_EQ(round_trip_goal_count(got), 4u);
    EXPECT_EQ(count_substr(got, "normalize("), 3u);
}

// ---------------------------------------------------------------------------
// Print / naming invariants
// ---------------------------------------------------------------------------

TEST_F(ElaborateCommandHandlerTest, PrintNamesRegisteredNotNumeric) {
    const std::string got = run_elaborate(
        {"cli/test/fixtures/id.def"},
        {"cli/test/fixtures/id_id.stmts"},
        out_path("elab_names.txt"));
    expect_named_print_invariants(got);
    EXPECT_NE(got.find("normalize("), std::string::npos);
    EXPECT_NE(got.find("var("), std::string::npos);
    EXPECT_NE(got.find("Model"), std::string::npos);
    // Peano zero appears under abs(var(zero)) for id body
    EXPECT_NE(got.find("zero"), std::string::npos);
}

TEST_F(ElaborateCommandHandlerTest, EmptyProgramPrintsMainHole) {
    const std::string got = run_elaborate({}, {}, out_path("elab_main_hole.txt"));
    EXPECT_NE(got.find("Main"), std::string::npos);
    EXPECT_EQ(got.find("?0"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Failure paths
// ---------------------------------------------------------------------------

TEST_F(ElaborateCommandHandlerTest, MissingModulePathThrows) {
    EXPECT_THROW(
        run_elaborate(
            {"cli/test/fixtures/does_not_exist.def"},
            {},
            out_path("elab_missing_mod.txt")),
        std::runtime_error);
}

TEST_F(ElaborateCommandHandlerTest, MissingStatementPathThrows) {
    EXPECT_THROW(
        run_elaborate(
            {"cli/test/fixtures/id.def"},
            {"cli/test/fixtures/does_not_exist.stmts"},
            out_path("elab_missing_stmt.txt")),
        std::runtime_error);
}

TEST_F(ElaborateCommandHandlerTest, UnboundStatementThrows) {
    EXPECT_THROW(
        run_elaborate(
            {},
            {"cli/test/fixtures/unbound.stmts"},
            out_path("elab_unbound.txt")),
        std::out_of_range);
}

TEST_F(ElaborateCommandHandlerTest, UnwritableOutputThrows) {
    EXPECT_THROW(
        run_elaborate(
            {},
            {},
            "build/no_such_dir/elab_out.txt"),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// Round-trip (atlas goal body parse)
// ---------------------------------------------------------------------------

TEST_F(ElaborateCommandHandlerTest, RoundTripEmptyModulesOnlyAndOneStmt) {
    const std::string empty = run_elaborate({}, {}, out_path("elab_rt_empty.txt"));
    EXPECT_EQ(round_trip_goal_count(empty), 1u);

    const std::string mods = run_elaborate(
        {"parser/fixtures/logic.def"}, {}, out_path("elab_rt_mods.txt"));
    EXPECT_EQ(round_trip_goal_count(mods), 1u);

    const std::string one = run_elaborate(
        {"cli/test/fixtures/id.def"},
        {"cli/test/fixtures/id_id.stmts"},
        out_path("elab_rt_one.txt"));
    EXPECT_EQ(round_trip_goal_count(one), 2u);
}
