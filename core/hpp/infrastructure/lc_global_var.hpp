#ifndef LC_GLOBAL_VAR_HPP
#define LC_GLOBAL_VAR_HPP

#include <stdexcept>
#include <string>
#include "infrastructure/global_env.hpp"
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcVar>
const lc_expr* lc_global_var(IMakeLcVar& make_var, const global_env& global,
                             const std::string& name, uint32_t local_depth) {
    auto k = global.lookup_global(name);
    if (!k)
        throw std::runtime_error("unbound global name: " + name);
    return make_var.make_var(local_depth + *k);
}

#endif
