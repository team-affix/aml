#ifndef DECLARATION_GROUP_HPP
#define DECLARATION_GROUP_HPP

#include <vector>
#include "value_objects/declaration.hpp"

struct declaration_group {
    std::vector<declaration> declarations;
    auto operator<=>(const declaration_group&) const = default;
};

#endif
