#ifndef TRANSPILLED_PROGRAM_HPP
#define TRANSPILLED_PROGRAM_HPP

#include <map>
#include <string>
#include <vector>
#include "infrastructure/lc_expr_pool.hpp"
#include "transpiled_function.hpp"

struct transpiled_program {
    lc_expr_pool                       pool;
    std::map<std::string, const lc_expr*> globals;
    std::vector<transpiled_function>   functions;

    transpiled_program() = default;
    transpiled_program(const transpiled_program&) = delete;
    transpiled_program& operator=(const transpiled_program&) = delete;
    transpiled_program(transpiled_program&&) = default;
    transpiled_program& operator=(transpiled_program&&) = default;
};

#endif
