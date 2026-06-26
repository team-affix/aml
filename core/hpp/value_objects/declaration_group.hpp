#ifndef DECLARATION_GROUP_HPP
#define DECLARATION_GROUP_HPP

#include <vector>
#include "declaration_decl.hpp"

struct declaration_group {
    std::vector<declaration_decl> declarations;
    auto operator<=>(const declaration_group&) const = default;
};

#endif
