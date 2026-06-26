#ifndef CONSTRUCTOR_GROUP_HPP
#define CONSTRUCTOR_GROUP_HPP

#include <vector>
#include "constructor_decl.hpp"

struct constructor_group {
    std::vector<constructor_decl> constructors;
    auto operator<=>(const constructor_group&) const = default;
};

#endif
