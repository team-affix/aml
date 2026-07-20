#include "infrastructure/elaborator_runtime.hpp"

elaborator_runtime::elaborator_runtime(
        const std::vector<module_file>& module_files,
        const std::vector<statement_file>& statement_files,
        initial_goal_exprs& initial_goals)
    : manifest_(module_files, statement_files, initial_goals) {}

void elaborator_runtime::elaborate() {
    manifest_.elaborator_.elaborate();
}
