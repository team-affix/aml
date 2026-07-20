#ifndef DEFINITION_HPP
#define DEFINITION_HPP

#include <string>
#include "aml_expr.hpp"

struct definition {
    std::string     name;
    const aml_expr* body;
    auto operator<=>(const definition&) const = default;
};

#endif
