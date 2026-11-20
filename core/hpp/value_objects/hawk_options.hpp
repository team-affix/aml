#ifndef HAWK_OPTIONS_HPP
#define HAWK_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

struct hawk_options {
    size_t max_applications;
    double exploration_constant;
    uint32_t seed;
    auto operator<=>(const hawk_options&) const = default;
};

}

#endif
