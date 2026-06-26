#ifndef DECLARATION_FILE_HPP
#define DECLARATION_FILE_HPP

#include <vector>
#include "declaration_group.hpp"

struct declaration_file {
    std::vector<declaration_group> groups;
};

#endif
