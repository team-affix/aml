#ifndef MODULE_FILE_HPP
#define MODULE_FILE_HPP

#include <vector>
#include "value_objects/module_item.hpp"

struct module_file {
    std::vector<module_item> items;
};

#endif
