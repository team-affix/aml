#ifndef GLOBAL_ENV_FACTORY_HPP
#define GLOBAL_ENV_FACTORY_HPP

#include <string>
#include <vector>
#include "infrastructure/builtin_declarations.hpp"
#include "infrastructure/global_env.hpp"

inline std::vector<std::string> builtin_global_names() {
    std::vector<std::string> names;
    for (const declaration_group* group : builtin_declaration_groups)
        for (const declaration_decl& decl : group->declarations)
            names.push_back(decl.name);
    return names;
}

inline global_env global_env_from_builtin_names() {
    return global_env(builtin_global_names());
}

inline global_env global_env_from_builtin_names_and(std::vector<std::string> extra) {
    std::vector<std::string> names = builtin_global_names();
    for (std::string& name : extra)
        names.push_back(std::move(name));
    return global_env(std::move(names));
}

#endif
