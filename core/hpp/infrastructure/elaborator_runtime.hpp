#ifndef ELABORATOR_RUNTIME_HPP
#define ELABORATOR_RUNTIME_HPP

#include <vector>
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/elaborator_manifest.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement_file.hpp"

struct elaborator_runtime {
    elaborator_runtime(const std::vector<module_file>& module_files,
                       const std::vector<statement_file>& statement_files,
                       initial_goal_exprs& initial_goals);

    void elaborate();

private:
    elaborator_manifest manifest_;
};

#endif
