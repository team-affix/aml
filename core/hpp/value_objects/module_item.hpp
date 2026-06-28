#ifndef MODULE_ITEM_HPP
#define MODULE_ITEM_HPP

#include <variant>
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"

struct module_item {
    std::variant<declaration_group, definition> content;
};

#endif
