#ifndef SCOUT_OPTIONS_HPP
#define SCOUT_OPTIONS_HPP

#include <cstddef>

namespace AML {

struct scout_options {
    size_t max_applications;
    auto operator<=>(const scout_options&) const = default;
};

}

#endif
