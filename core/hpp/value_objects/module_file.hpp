#ifndef MODULE_FILE_HPP
#define MODULE_FILE_HPP

#include <vector>
#include "value_objects/global.hpp"

struct module_file {
    std::vector<global> items;
    auto operator<=>(const module_file&) const = default;
};

#endif
