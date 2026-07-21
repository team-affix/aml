#include "infrastructure/elaborate_command_handler.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include "infrastructure/aml_expr_pool.hpp"
#include "infrastructure/elaborator_runtime.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/var_names.hpp"
#include "parser/hpp/import_module_file.hpp"
#include "parser/hpp/import_statement_file.hpp"
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/lc_functor_ids.hpp"
#include "value_objects/module_file.hpp"
#include "value_objects/statement_file.hpp"

elaborate_command_handler::elaborate_command_handler(
        std::vector<std::string> module_paths,
        std::vector<std::string> statement_paths,
        std::string output_path)
    : module_paths_(std::move(module_paths))
    , statement_paths_(std::move(statement_paths))
    , output_path_(std::move(output_path)) {}

void elaborate_command_handler::operator()() {
    aml_expr_pool pool;

    std::vector<module_file> mods;
    for (const std::string& path : module_paths_) {
        mods.push_back(import_module_file(
            path, pool, pool, pool, pool, pool, pool, pool, pool));
    }

    std::vector<statement_file> stmts;
    for (const std::string& path : statement_paths_) {
        stmts.push_back(import_statement_file(
            path, pool, pool, pool, pool, pool, pool, pool, pool));
    }

    initial_goal_exprs goals;
    elaborator_runtime rt{mods, stmts, goals};
    rt.elaborate();

    functor_names fn;
    var_names vn;
    fn.set_name(k_abs_functor_id, "abs");
    fn.set_name(k_app_functor_id, "app");
    fn.set_name(k_var_functor_id, "var");
    fn.set_name(k_zero_functor_id, "zero");
    fn.set_name(k_suc_functor_id, "suc");
    fn.set_name(k_normalize_functor_id, "normalize");
    fn.set_name(k_eq_functor_id, "eq");
    vn.set_name(k_main_function_var_id, "Main");
    vn.set_name(k_model_var_id, "Model");

    std::ostringstream out;
    expr_printer printer{out, vn, fn};
    for (size_t i = 0; i < goals.count(); ++i) {
        if (i > 0)
            out << ", ";
        printer.print(goals.get(i));
    }

    std::ofstream file(output_path_);
    if (!file)
        throw std::runtime_error("cannot open output file: " + output_path_);
    file << out.str();
    if (!file)
        throw std::runtime_error("failed writing output file: " + output_path_);
}
