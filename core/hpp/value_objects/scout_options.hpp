#ifndef SCOUT_OPTIONS_HPP
#define SCOUT_OPTIONS_HPP

#include <cstddef>

struct scout_options {
    size_t max_synthesis_depth;
    size_t max_constant_size;
    auto operator<=>(const scout_options&) const = default;
};

#endif
