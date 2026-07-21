#ifndef ELABORATE_TEST_HELPERS_HPP
#define ELABORATE_TEST_HELPERS_HPP

#include <cstdint>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "import_goals_from_string.hpp"
#include "infrastructure/elaborate_command_handler.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"

inline std::string read_file(const std::string& path) {
    std::ifstream in(path);
    EXPECT_TRUE(in.good()) << "cannot read " << path;
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}

inline std::string run_elaborate(const std::vector<std::string>& modules,
                                 const std::vector<std::string>& statements,
                                 const std::string& out_path) {
    elaborate_command_handler h(modules, statements, out_path);
    h();
    return read_file(out_path);
}

inline size_t round_trip_goal_count(const std::string& goals_str) {
    expr_pool pool;
    initial_goal_exprs goals;
    non_backtracking_var_sequencer var_seq(0);
    std::map<std::string, uint32_t> functor_map;
    uint32_t next_functor_id = k_first_user_functor_id;
    import_goals_from_string(
        goals_str, pool, pool, var_seq, goals, functor_map, next_functor_id);
    return goals.count();
}

inline size_t count_substr(const std::string& haystack, const std::string& needle) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

inline void expect_no_list_sugar(const std::string& got) {
    EXPECT_EQ(got.find('['), std::string::npos);
}

inline void expect_named_print_invariants(const std::string& got) {
    EXPECT_NE(got.find("eq("), std::string::npos);
    EXPECT_NE(got.find("abs("), std::string::npos);
    EXPECT_NE(got.find("app("), std::string::npos);
    EXPECT_EQ(got.find("!2"), std::string::npos);
    EXPECT_EQ(got.find("!3"), std::string::npos);
    EXPECT_EQ(got.find("!8"), std::string::npos);
    EXPECT_EQ(got.find("?0"), std::string::npos);
    EXPECT_EQ(got.find("?1"), std::string::npos);
    expect_no_list_sugar(got);
}

#endif
