#ifndef SCOUT_OPTIONS_HPP
#define SCOUT_OPTIONS_HPP

#include <cstddef>

struct scout_options {
    size_t max_applications;
    auto operator<=>(const scout_options&) const = default;
};

#endif
