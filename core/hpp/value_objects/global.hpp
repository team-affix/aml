#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <variant>
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"

using global = std::variant<declaration_group, definition>;

#endif
